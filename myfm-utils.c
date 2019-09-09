//
// Created by f35 on 24.08.19.
//

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-utils.h"
#include "myfm-file.h"

/* welcome to callback hell. function order of execution:
*     children_to_store_async --> g_file_enumerate_children_async
* --> enum_files_callback --> g_file_enumerator_next_files_async
* --> next_files_callback */

/* simple struct for passing multiple variables through our callbacks */
struct GFileAndListStore {
    GFile *g_file;
    GtkListStore *store;
};

static int num_files = 32; // TODO: HOW MANY?????

static void add_children (GtkListStore *store, GFile *parent_file, GList *children_list)
{
    GtkTreeIter iter;
    GFileInfo *child_info; /* doesn't need autoptr */
    GError *error = NULL; /* doesn't need autoptr */
    GList *current_node = children_list;

    while (current_node) {
        child_info = current_node->data;
        const char *child_name = g_file_info_get_display_name (child_info);
        GFile *child_g_file = g_file_get_child_for_display_name (parent_file, child_name, &error);

        if (error) {
            g_object_unref (child_info);
            g_object_unref (child_g_file);
            g_error_free (error);
            error = NULL; /* TODO: KEEP TABS */
        }
        else {
            MyFMFile *child_myfm_file = myfm_file_new_without_io (child_g_file, g_strdup(child_name));
            gtk_list_store_append (store, &iter); /* out iter */
            gtk_list_store_set (store, &iter, 0, (gpointer) child_myfm_file, -1);
            g_object_unref (child_info);
        }
        current_node = current_node->next;
    }
}

/* https://stackoverflow.com/questions/35036909/c-glib-gio-how-to-list-files-asynchronously
 * function calls itself until all children have been listed */
static void next_files_callback (GObject *file_enumerator, GAsyncResult *result, gpointer file_and_store)
{
    GError_autoptr error = NULL; /* auto_ptr or not? at least we avoid g_error_free everywhere */
    GList *directory_list = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (file_enumerator),
                                                                result, &error);
    if (error) {
        g_critical ("Unable to add files to list, error: %s", error->message); // TODO: read error handling docs
        g_object_unref (file_enumerator);
        g_object_unref ((((struct GFileAndListStore*) file_and_store)->g_file));
        free (file_and_store);
        return;
    }
    else if (directory_list == NULL) {
        /* done listing, nothing left to add to store */
        g_object_unref (file_enumerator);
        g_object_unref ((((struct GFileAndListStore*) file_and_store)->g_file)); /* decrement refcount once we're done */
        free (file_and_store);

        g_signal_emit_by_name (((struct GFileAndListStore*) file_and_store)->store, "children_added");

        return;
    }
    else {
        /* enumerator returned successfully */
        add_children (((struct GFileAndListStore*) file_and_store)->store,
                       ((struct GFileAndListStore*) file_and_store)->g_file,  directory_list);

        g_file_enumerator_next_files_async (G_FILE_ENUMERATOR (file_enumerator),
                                           num_files, G_PRIORITY_HIGH, NULL,
                                           next_files_callback, file_and_store);
    }
    g_list_free (directory_list);
}

static void enum_finished_callback (GObject *directory, GAsyncResult *result, gpointer file_and_store)
{
    GFileEnumerator_autoptr file_enumerator = NULL;
    GError_autoptr error = NULL;

    file_enumerator = g_file_enumerate_children_finish (G_FILE (directory), result, &error);
    if (error) {
        g_object_unref ((((struct GFileAndListStore*) file_and_store)->g_file)); /* decrement refcount */
        free (file_and_store);
        /* TODO: g_critical? */
        return;
    }
    else {
        g_file_enumerator_next_files_async (file_enumerator, num_files,
                                           G_PRIORITY_HIGH, NULL,
                                           next_files_callback, file_and_store);
        file_enumerator = NULL; /* set local pointer to NULL to prevent auto_unref before callback is invoked */
    }
}

void children_to_store_async (GFile *directory, GtkListStore *store)
{
    struct GFileAndListStore *file_and_store = malloc (sizeof (struct GFileAndListStore));
    file_and_store->g_file = directory;
    file_and_store->store = store;

    /* in some situations we might unref the g_file passed to this function shortly
     * after it is called. in the case that our IO takes long to complete, this
     * means there is a possibility that our g_file is freed before our callbacks
     * are invoked. to prevent this, we increment our g_file's refcount to
     * keep it alive at least temporarily */
    g_object_ref (directory);

    g_file_enumerate_children_async (directory,
                                     "*",
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     G_PRIORITY_HIGH, NULL,
                                     enum_finished_callback,
                                     file_and_store);
}