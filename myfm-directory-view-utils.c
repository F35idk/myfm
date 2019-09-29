//
// Created by f35 on 24.08.19.
//

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-directory-view-utils.h"
#include "myfm-file.h"
#include "myfm-directory-view.h"

/* welcome to callback hell. function order of execution:
*     files_to_store_async --> g_file_enumerate_children_async
* --> enum_files_callback --> g_file_enumerator_next_files_async
* --> next_files_callback */

static gint num_files = 32; // TODO: HOW MANY?????

static void add_files (GtkListStore *store, GFile *parent_file, GList *file_list)
{
    GtkTreeIter iter;
    GFileInfo *child_info; /* doesn't need autoptr */
    GError *error = NULL; /* doesn't need autoptr */
    GList *current_node = file_list;

    while (current_node) {
        child_info = current_node->data;
        const char *child_name = g_file_info_get_display_name (child_info); // TODO: no free
        GFile *child_g_file = g_file_get_child_for_display_name (parent_file, child_name, &error);

        if (error) {
            g_object_unref (child_info);
            g_object_unref (child_g_file);
            g_error_free (error);
            error = NULL; /* TODO: KEEP TABS */
        }
        else {
            MyFMFile *child_myfm_file = myfm_file_new_without_io (child_g_file, g_strdup (child_name));
            gtk_list_store_append (store, &iter); /* out iter */
            gtk_list_store_set (store, &iter, 0, (gpointer) child_myfm_file, -1);
            g_object_unref (child_info);
        }
        current_node = current_node->next;
    }
}

/* https://stackoverflow.com/questions/35036909/c-glib-gio-how-to-list-files-asynchronously
 * function calls itself until all files have been listed */
static void next_files_callback (GObject *file_enumerator, GAsyncResult *result, gpointer dirview)
{
    GError_autoptr error = NULL; /* auto_ptr or not? at least we avoid g_error_free everywhere */
    GList *directory_list = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (file_enumerator),
                                                                result, &error);
    if (error) {
        g_critical ("Unable to add files to list, error: %s", error->message); // TODO: read error handling docs
        g_object_unref (file_enumerator);
        return;
    }
    else if (directory_list == NULL) { /* done listing, nothing left to add to store */
        g_object_unref (file_enumerator);
        g_signal_emit_by_name (MYFM_DIRECTORY_VIEW (dirview), "filled"); // TODO: just show the dirview instead?
        return;
    }
    else { /* enumerator returned successfully */
        GFile *dir = myfm_directory_view_get_directory (MYFM_DIRECTORY_VIEW (dirview))->g_file;
        GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dirview)));
        add_files (store, dir, directory_list);
        g_file_enumerator_next_files_async (G_FILE_ENUMERATOR (file_enumerator),
                                           num_files, G_PRIORITY_HIGH, NULL,
                                           next_files_callback, dirview);
    }
    g_list_free (directory_list);
}

static void enum_finished_callback (GObject *directory, GAsyncResult *result, gpointer file_and_store)
{
    GFileEnumerator_autoptr file_enumerator = NULL;
    GError_autoptr error = NULL;

    file_enumerator = g_file_enumerate_children_finish (G_FILE (directory), result, &error);
    if (error) {
        // g_object_unref ((((struct GFileAndListStore*) file_and_store)->g_file)); /* decrement refcount */
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

/* adds the files in a directory to the dirview's list store */
void files_to_store_async (MyFMDirectoryView *dirview)
{
    g_file_enumerate_children_async (myfm_directory_view_get_directory (dirview)->g_file,
                                     "*",
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     G_PRIORITY_HIGH, NULL,
                                     enum_finished_callback,
                                     dirview);
}

gboolean clear_file_in_store (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    gpointer myfm_file;

    if (!model)
        return TRUE; /* stop iterating */

    gtk_tree_model_get (model, iter, 0, &myfm_file, -1); /* out myfm_file, pass by value */
    if (myfm_file) {
        // myfm_file_free ((MyFMFile *) myfm_file);
        myfm_file_unref ((MyFMFile*) myfm_file);
        gtk_list_store_set (GTK_LIST_STORE (model), iter, 0, NULL, -1);
    }

    return FALSE;
}
