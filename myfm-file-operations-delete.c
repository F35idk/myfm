/*
 * Created by f35 on 31.12.19.
*/

#include <gio/gio.h>

#include "myfm-utils.h"
#include "myfm-file-operations.h"
#include "myfm-file-operations-private.h"
#define G_LOG_DOMAIN "myfm-file-operations-delete"

void
_delete_file_single (GFile *file,
                     GtkWindow *active,
                     GCancellable *cancellable,
                     GError **error)
{
    GFileEnumerator *direnum;

    direnum = g_file_enumerate_children (file, MYFM_FILE_QUERY_ATTRIBUTES,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, NULL);
    /* if file is directory, iterate */
    while (direnum != NULL) {
        GFile *child = NULL;
        gboolean fail;

        fail = !g_file_enumerator_iterate (direnum, NULL, &child,
                                           cancellable, error);
        if (fail) {
            g_critical ("Error in myfm_file_operations_delete function "
                        "'_delete_file_single': %s", (*error)->message);
            return;
        }
        if (child == NULL) {
            /* dir has been iterated over and is now empty */
            break;
        }

        /* recurse */
        _delete_file_single (child, active, cancellable, error);
        if (*error) {
            g_object_unref (direnum);
            return;
        }
    }

    if (direnum)
        g_object_unref (direnum);

    g_file_delete (file, cancellable, error);

    if (*error) {
        g_critical ("Error in myfm_file_operations_delete function "
                    "'_delete_file_single': %s", (*error)->message);
        return;
    }
}

void
_delete_files_thread (GTask *task, gpointer src_object,
                      gpointer task_data,
                      GCancellable *cancellable)
{
    GFile **arr;
    GFile *current;
    GError *error = NULL;
    GtkWindow *win;

    win = g_object_get_data (G_OBJECT (task), "win");
    arr = task_data;

    for (int i = 0; (current = arr[i]); i ++) {
        _delete_file_single (current, win, cancellable,  &error);
        if (error) {
            _run_fatal_err_dialog (task, FILE_OPERATION_DELETE,
                                   "%s", error->message);
            g_error_free (error);
        }

        g_object_unref (current);
    }
    g_free (arr);

    g_debug ("finished");
}

void
_delete_files_finish (GObject *src_object,
                      GAsyncResult *res,
                      gpointer _cb)
{
    MyFMFileOpCallback cb;
    GCancellable *cancellable;
    gpointer user_data;
    GtkWindow *win;

    cb = _cb;
    cancellable = g_task_get_cancellable (G_TASK (res));
    user_data = g_object_get_data (G_OBJECT (res), "user_data");
    win = g_object_get_data (G_OBJECT (res), "win");

    if (cb)
        cb (user_data);

    cancellable = g_task_get_cancellable (G_TASK (res));
    g_object_unref (cancellable);
    g_object_unref (res);

    win = NULL;
    user_data = NULL;
}
