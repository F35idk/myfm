/*
 * Created by f35 on 31.12.19.
*/

#include <gio/gio.h>

#include "myfm-utils.h"
#include "myfm-delete-operation.h"
#define G_LOG_DOMAIN "myfm-delete-operation"

/* NOTE: there is currently a lot of samey
 * code in this and myfm-copy-operation.
 * consider refactoring */

static GtkWindow *win = NULL;
static gpointer user_data = NULL;

static MyFMDialogResponse
run_error_dialog (GtkWindow *active,
                  GCancellable *cancellable,
                  const gchar *format_msg,
                  ...)
{
    va_list va;
    gchar *title;
    gchar *primary_msg;

    va_start (va, format_msg);
    title = g_strdup ("File Delete Error");
    primary_msg = g_strdup ("There was an error "
                            "during the delete "
                            "operation.");

    return myfm_utils_run_unskippable_err_dialog_thread (active,
                                                         cancellable,
                                                         title,
                                                         primary_msg,
                                                         format_msg,
                                                         va);
}

void
myfm_delete_operation_delete_single_sync (GFile *file,
                                          GtkWindow *active,
                                          GCancellable *cancellable,
                                          GError **error)
{
    GFileEnumerator *direnum;

    direnum = g_file_enumerate_children (file, MYFM_FILE_QUERY_ATTRIBUTES,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, NULL);
    /* if file is directory */
    while (direnum != NULL) {
        GFile *child = NULL;
        gboolean fail;

        fail = !g_file_enumerator_iterate (direnum, NULL, &child,
                                           cancellable, error);
        if (fail) {
            g_critical ("Error in myfm_delete_operation function "
                        "'delete_single_sync': %s", (*error)->message);
            return;
        }
        if (child == NULL) {
            /* dir has been iterated over and is now empty */
            break;
        }

        /* recurse */
        myfm_delete_operation_delete_single_sync (child, active,
                                                  cancellable, error);
        if (*error) {
            g_object_unref (direnum);
            return;
        }
    }

    if (direnum)
        g_object_unref (direnum);

    g_file_delete (file, cancellable, error);

    if (*error) {
        g_critical ("Error in myfm_delete_operation function "
                    "'delete_single_sync': %s", (*error)->message);
        return;
    }
}

static void
myfm_delete_operation_thread (GTask *task, gpointer src_object,
                              gpointer task_data,
                              GCancellable *cancellable)
{
    GFile **arr;
    GFile *current;
    GError *error = NULL;

    arr = task_data;
    for (int i = 0; (current = arr[i]); i ++) {
        myfm_delete_operation_delete_single_sync (current,
                                                  win,
                                                  cancellable,
                                                  &error);
        if (error) {
            run_error_dialog (win, cancellable,
                              "%s", error->message);
            g_error_free (error);
        }

        g_object_unref (current);
    }
    g_free (arr);

    g_debug ("finished");
}

static void
myfm_delete_operation_finish (GObject *src_object,
                              GAsyncResult *res,
                              gpointer _cb)
{
    MyFMDeleteCallback cb;
    MyFMApplication *app;
    GCancellable *cancellable;

    cb = _cb;
    if (cb)
        cb (user_data);

    app = MYFM_APPLICATION (gtk_window_get_application (win));
    myfm_application_set_delete_in_progress (app, FALSE);

    cancellable = g_task_get_cancellable (G_TASK (res));
    g_object_unref (cancellable);
    g_object_unref (res);

    win = NULL;
    user_data = NULL;
}

/* TODO: if file is in clipboard, simply mark the
 * file as cut instead of starting delete operation? */
void
myfm_delete_operation_start_async (MyFMFile **src_files, gint n_files,
                                   GtkWindow *active, MyFMDeleteCallback cb,
                                   gpointer data)
{
    GTask *del;
    GFile **arr;
    GCancellable *cancellable;
    MyFMApplication *app;

    app = MYFM_APPLICATION (gtk_window_get_application (active));
    g_return_if_fail (!myfm_application_delete_in_progress (app));
    g_return_if_fail (active != NULL);

    arr = g_malloc (sizeof (GFile *) * (n_files + 1));

    for (int i = 0; i < n_files; i ++)
        arr[i] = g_file_dup (myfm_file_get_g_file (src_files[i]));

    /* NULL-terminate */
    arr[n_files] = NULL;

    win = active;
    cancellable = g_cancellable_new ();
    user_data = data;
    del = g_task_new (NULL, cancellable,
                      myfm_delete_operation_finish,
                      cb);

    myfm_application_set_delete_in_progress (app, TRUE);

    g_task_set_task_data (del, arr, NULL);
    g_task_set_priority (del, G_PRIORITY_DEFAULT);
    g_task_run_in_thread (del, myfm_delete_operation_thread);
}
