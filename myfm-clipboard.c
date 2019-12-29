/*
 * Created by f35 on 23.12.19.
*/

#include <glib.h>
#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-window.h"
#include "myfm-file-operations.h"
#include "myfm-clipboard.h"

G_DEFINE_BOXED_TYPE (MyFMClipBoard, myfm_clipboard, myfm_clipboard_copy, myfm_clipboard_free)

/* for iterating over cut_files through callbacks.
 * since there is only ever one instance of myfm_clipboard,
 * static vars like this should be fine */
static GHashTableIter iter;

static void
paste_cut_files_callback (gpointer dest_dir_path,
                          gpointer win, gpointer data)
{
    gpointer key;
    MyFMFile *file;

    if (g_hash_table_iter_next (&iter, &key, &file))  {
        if (file) {
            // myfm_file_operations_copy_async (file, dest_dir_path,
            //                                  win, FALSE,
            //                                  paste_cut_files_callback,
            //                                  dest_dir_path, win, NULL);
            g_hash_table_iter_remove (&iter);
        }
    }
    else {
        g_free (dest_dir_path);
    }
}

static void
paste_copied_files_callback (gpointer dest_dir_path,
                             gpointer win, gpointer copied)
{
    MyFMFile *file;

    file = g_ptr_array_steal_index_fast (copied, 0);

    if (file) {
        // myfm_file_operations_copy_async (file, dest_dir_path,
        //                                  MYFM_WINDOW (win),
        //                                  FALSE, paste_copied_files_callback,
        //                                  dest_dir_path, win, copied); /* user data */
    }
    else {
        g_free (dest_dir_path);
    }
}

void
myfm_clipboard_paste (MyFMClipBoard *self, MyFMFile *dest_dir,
                      GtkWindow *active_win)
{
    gchar *dest_dir_path;

    /* start asynchronously iterating over cut_files or
     * copied_files and copy them into dest */
    if (g_hash_table_size (self->cut_files)) {
        gpointer key;
        MyFMFile *file;

        dest_dir_path = g_file_get_path (myfm_file_get_g_file (dest_dir));
        g_hash_table_iter_init (&iter, self->cut_files);

        if (g_hash_table_iter_next (&iter, &key, &file))  {
            if (file) {
                // myfm_file_operations_copy_async (file, dest_dir_path,
                //                                  MYFM_WINDOW (active_win),
                //                                  FALSE, paste_cut_files_callback,
                //                                  dest_dir_path, active_win, NULL); /* user data */
                g_hash_table_iter_remove (&iter);
            }
        }
        else {
            g_free (dest_dir_path);
        }
    }
    else if (self->copied_files->len) {
        MyFMFile *file;

        dest_dir_path = g_file_get_path (myfm_file_get_g_file (dest_dir));
        file = g_ptr_array_steal_index_fast (self->copied_files, 0);

        if (file) {
            // myfm_file_operations_copy_async (file, dest_dir_path,
            //                                  MYFM_WINDOW (active_win),
            //                                  FALSE, paste_copied_files_callback,
            //                                  dest_dir_path, active_win,
            //                                  self->copied_files);
        }
        else {
            g_free (dest_dir_path);
        }
    }
}

static void
myfm_clipboard_on_cut_file_updated (MyFMFile *myfm_file,
                                    GFile *old_g_file,
                                    GError *error,
                                    gpointer myfm_clipboard)
{
    MyFMClipBoard *self;

    self = (MyFMClipBoard *) myfm_clipboard;
    /* if updated file is cut, replace its old g_file key */
    if (g_hash_table_contains (self->cut_files, old_g_file)) {
        g_debug ("file in clipboard updated");
        g_hash_table_replace (self->cut_files,
                              myfm_file_get_g_file (myfm_file),
                              myfm_file);
        return;
    }
}

void
myfm_clipboard_add_to_copied (MyFMClipBoard *self,
                              MyFMFile **files, gint n_files)
{
    g_ptr_array_remove_range (self->copied_files, 0,
                              self->copied_files->len);
    g_hash_table_remove_all (self->cut_files);

    for (int i = 0; i < n_files; i ++) {
        myfm_file_ref (files[i]);
        g_ptr_array_add  (self->copied_files, files[i]);
    }

}

void
myfm_clipboard_add_to_cut (MyFMClipBoard *self,
                           MyFMFile **files, gint n_files)
{
    g_ptr_array_remove_range (self->copied_files, 0,
                              self->copied_files->len);
    g_hash_table_remove_all (self->cut_files);

    for (int i = 0; i < n_files; i ++) {
        myfm_file_ref (files[i]);
        g_object_ref (myfm_file_get_g_file (files[i]));

        g_hash_table_insert (self->cut_files,
                             myfm_file_get_g_file (files[i]),
                             files[i]);

        myfm_file_connect_update_callback (files[i],
                                           myfm_clipboard_on_cut_file_updated,
                                           self);
    }
}

gboolean
myfm_clipboard_file_is_cut (MyFMClipBoard *self,
                            MyFMFile *file)
{
    return g_hash_table_contains (self->cut_files,
                                  myfm_file_get_g_file (file));
}

gboolean
myfm_clipboard_is_empty (MyFMClipBoard *self)
{
    gboolean empty1;
    gboolean empty2;

    empty1 = !g_hash_table_size (self->cut_files);
    empty2 = !self->copied_files->len;

    return empty1 && empty2;
}

void
myfm_clipboard_free (MyFMClipBoard *self)
{
}

/* dummy func (needed to define boxed type) */
MyFMClipBoard *
myfm_clipboard_copy (MyFMClipBoard *self)
{
    return self;
}

/* wrapper around myfm_file_unref so we
 * can set it as a valid destroy func */
static void
myfm_file_destroy_func (gpointer myfm_file)
{
    myfm_file_unref (myfm_file);
}

MyFMClipBoard *
myfm_clipboard_new (void)
{
    MyFMClipBoard *clipboard;

    clipboard = g_malloc (sizeof (MyFMClipBoard));
    clipboard->copied_files = g_ptr_array_new_with_free_func (myfm_file_destroy_func);
    /* we keep a hash table of cut myfm_files, with their g_files as keys */
    clipboard->cut_files = g_hash_table_new_full ((GHashFunc) g_file_hash,
                                                  (GEqualFunc) g_file_equal,
                                                  g_object_unref,
                                                  myfm_file_destroy_func);

    return clipboard;
}
