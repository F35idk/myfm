/*
 * Created by f35 on 23.12.19.
*/

#include <glib.h>
#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-window.h"
#include "myfm-clipboard.h"
#define G_LOG_DOMAIN "myfm-clipboard"

struct _MyFMClipBoard {
    GObject parent_instance;
    GPtrArray *copied_files;
    GPtrArray *cut_files;
};

G_DEFINE_TYPE (MyFMClipBoard, myfm_clipboard, G_TYPE_OBJECT)

static void
myfm_clipboard_add (MyFMClipBoard *self,
                    MyFMFile **files,
                    gint n_files,
                    GPtrArray *arr)
{
    myfm_clipboard_clear (self);

    for (int i = 0; i < n_files; i ++) {
        myfm_file_ref (files[i]);
        g_ptr_array_add  (arr, files[i]);
    }
}

void
myfm_clipboard_add_to_copied (MyFMClipBoard *self,
                              MyFMFile **files, gint n_files)
{
    myfm_clipboard_add (self, files, n_files, self->copied_files);
}

void
myfm_clipboard_add_to_cut (MyFMClipBoard *self,
                           MyFMFile **files, gint n_files)
{
    myfm_clipboard_add (self, files, n_files, self->cut_files);
    g_signal_emit_by_name (self, "cut-uncut",
                           files, n_files, TRUE);
}

/* gets the contents of the clipboard.
 * refs the files before returning them */
MyFMFile **
myfm_clipboard_get_contents (MyFMClipBoard *self, gint *out_n_files,
                             gboolean *out_copied)
{
    MyFMFile **files = NULL;

    if (self->copied_files->len) {
        *out_n_files = self->copied_files->len;
        *out_copied = TRUE;
    }
    else if (self->cut_files->len) {
        *out_n_files = self->cut_files->len;
        *out_copied = FALSE;
    }
    else {
        *out_n_files = 0;
        *out_copied = FALSE;
        return files;
    }

    files = g_malloc (sizeof (MyFMFile *) * (*out_n_files));

    for (int i = 0; i < (*out_n_files); i ++) {
        if (*out_copied)
            files[i] = g_ptr_array_index (self->copied_files, 0);
        else
            files[i] = g_ptr_array_index (self->cut_files, 0);

        myfm_file_ref (files[i]);
    }

    return files;
}

void
myfm_clipboard_clear (MyFMClipBoard *self)
{
    gboolean copied = self->copied_files->len;

    if (copied) {
        g_ptr_array_remove_range (self->copied_files, 0,
                                  self->copied_files->len);
    }
    else {
        MyFMFile **files;
        gint n_files;

        files = myfm_clipboard_get_contents (self, &n_files,
                                             &copied);
        if (n_files) {
            g_ptr_array_remove_range (self->cut_files, 0,
                                      self->cut_files->len);
            g_signal_emit_by_name (self, "cut-uncut",
                                   files, n_files, FALSE);

            for (int i = 0; i < n_files; i++)
                myfm_file_unref (files[i]);
        }

        g_free (files);
    }
}

gboolean
myfm_clipboard_file_is_cut (MyFMClipBoard *self,
                            MyFMFile *file)
{
    gpointer *files = self->cut_files->pdata;

    for (int i = 0; i < self->cut_files->len; i++) {
        if (g_file_equal (myfm_file_get_g_file (file),
                          myfm_file_get_g_file (files[i])))
            return TRUE;
    }

    return FALSE;
}

gboolean
myfm_clipboard_is_empty (MyFMClipBoard *self)
{
    gboolean empty1;
    gboolean empty2;

    empty1 = !self->cut_files->len;
    empty2 = !self->copied_files->len;

    return empty1 && empty2;
}

static void
myfm_clipboard_finalize (GObject *object)
{
    MyFMClipBoard *self = MYFM_CLIPBOARD (object);

    g_ptr_array_free (self->copied_files, TRUE);
    g_ptr_array_free (self->cut_files, TRUE);

    G_OBJECT_CLASS (myfm_clipboard_parent_class)->finalize (object);
}

static void
myfm_clipboard_init (MyFMClipBoard *self)
{
    self->copied_files = g_ptr_array_new_with_free_func ((GDestroyNotify) myfm_file_unref);
    self->cut_files = g_ptr_array_new_with_free_func ((GDestroyNotify) myfm_file_unref);
}

static void
myfm_clipboard_class_init (MyFMClipBoardClass *cls)
{
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);
    object_cls->finalize = myfm_clipboard_finalize;

    /* signal emitted when files are added and removed
     * from the cut hash table in clipboard */
    g_signal_new ("cut-uncut", MYFM_TYPE_CLIPBOARD,
                  G_SIGNAL_RUN_FIRST, 0, NULL,
                  NULL, NULL, G_TYPE_NONE, 3,
                  G_TYPE_POINTER, /* array of myfm_files */
                  G_TYPE_INT, /* num of myfm_files */
                  G_TYPE_BOOLEAN /* TRUE if cut, FALSE if uncut */ );
}

MyFMClipBoard *
myfm_clipboard_new (void)
{
    return g_object_new (MYFM_TYPE_CLIPBOARD, NULL);
}
