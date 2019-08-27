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
* --> next_files_callback --> children_to_store_finish
*/

static int num_files = 32; // TODO: HOW MANY?????

static void children_to_store_finish (GPtrArray* directory_array)
{
    GtkTreeIter iter; // is initialized by append
    GtkListStore *store;

    /* second element  in directory_array is the list store */
    store = g_ptr_array_index (directory_array, 1);

    for (int i = 2; i < directory_array->len; i++) {
        gtk_list_store_append(store, &iter); // out iter
        gtk_list_store_set(store, &iter, 0, (gpointer) g_ptr_array_index (directory_array, i), -1);
    }
    // TODO: EMIT SIGNAL ON STORE??
}

// https://stackoverflow.com/questions/35036909/c-glib-gio-how-to-list-files-asynchronously
static void next_files_callback (GObject *file_enumerator, GAsyncResult *result, gpointer directory_array)
{
    GError *error = NULL;
    GList *directory_list = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (file_enumerator),
                                                                result, &error);
    if (error) {
        g_critical ("Unable to add files to list, error: %s", error->message); // TODO: programmer error or? read error handling docs
        g_object_unref (file_enumerator);
        g_error_free (error);
        return;
    }
    else if (directory_list == NULL) {
        /* done listing, nothing left to add to directory_array */
        // TODO: emit some signal? to tell our application that a directory has been opened
        children_to_store_finish (directory_array);
        g_object_unref (file_enumerator);
        return;
    }
    else {
        GList *current_node = directory_list;
        GFileInfo *child_info;
        GFile *parent_file;

        /* first element in directory_array is the parent directory */
        parent_file = g_ptr_array_index ((GPtrArray*) directory_array, 0);
        while (current_node) {
            child_info = current_node->data;
            const char *child_name = g_file_info_get_display_name (child_info);
            GFile *child_g_file = g_file_get_child_for_display_name (parent_file, child_name, &error);
            MyFMFile *child_myfm_file = malloc (sizeof (MyFMFile));
            myfm_file_from_g_file_async (child_myfm_file, child_g_file);
            g_ptr_array_add ((GPtrArray*) directory_array, child_myfm_file);
            current_node = current_node->next;
            g_object_unref (child_info); // TODO ????????????????????
        }
        g_file_enumerator_next_files_async (G_FILE_ENUMERATOR (file_enumerator),
                                           num_files, G_PRIORITY_DEFAULT, NULL,
                                           next_files_callback, directory_array);
    }
    g_list_free(directory_list);
}

static void enum_finished_callback (GObject *directory, GAsyncResult *result, gpointer directory_array)
{
    GFileEnumerator *file_enumerator;
    GError *error = NULL; // what is this &error double pointer magic

    file_enumerator = g_file_enumerate_children_finish (G_FILE (directory), result, &error);
    if (error)
        return; // do something more
    else {
        g_file_enumerator_next_files_async (file_enumerator, num_files,
                                           G_PRIORITY_DEFAULT, NULL,
                                           next_files_callback, directory_array);
    }
}

// TODO: ROW_ACTIVATED
void children_to_store_async (GFile *directory, GtkListStore *store)
{
    GFileType filetype = g_file_query_file_type (directory, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (filetype != G_FILE_TYPE_DIRECTORY)
        return; // do something more
    else {
        GPtrArray *directory_array;

        /* a little unintuitive, but we store the parent g_file and list_store in
         * our file array so we can access them later in all our callback functions. so long
         * as we remember the order and type of these objects, it shouldn't be too dangerous */
        directory_array = g_ptr_array_new();
        g_ptr_array_add (directory_array, directory);
        g_ptr_array_add (directory_array, store);

        g_file_enumerate_children_async (directory,
                                         "*",
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         G_PRIORITY_DEFAULT, NULL,
                                         enum_finished_callback,
                                         directory_array);
        puts ("hello!");
    }
}