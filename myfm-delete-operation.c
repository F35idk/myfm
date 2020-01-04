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

/* convenience func */
static MyFMDialogResponse /* (gint) */
run_error_dialog (const gchar *format_msg,
                  GtkWindow *active,
                  GCancellable *cancellable,
                  ...)
{
    va_list va;
    MyFMDialogType type;
    gchar *title;
    gchar *primary_msg;
    gchar *secondary_msg;

    va_start (va, format_msg);
    secondary_msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    type = MYFM_DIALOG_TYPE_UNSKIPPABLE_ERR;
    title = g_strdup ("File Delete Error");
    primary_msg = g_strdup ("There was an error "
                            "during the delete "
                            "operation.");

    return myfm_utils_run_dialog_thread (type, active,
                                         cancellable,
                                         title, primary_msg,
                                         secondary_msg);
}

/* TODO: pass GError */
static void
delete_file_recursive (GFile *file,
                       GtkWindow *active,
                       GCancellable *cancellable)
{
    GFileEnumerator *direnum;
    GError *error = NULL;

    direnum = g_file_enumerate_children (file, MYFM_FILE_QUERY_ATTRIBUTES,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, NULL);
    /* if file is directory */
    while (direnum != NULL) {
        GFile *child = NULL;
        gboolean fail;

        fail = !g_file_enumerator_iterate (direnum, NULL, &child,
                                           cancellable, &error);
        if (fail) {
            g_critical ("Error in myfm_delete_operation function "
                       "'delete_file_recursive': %s", error->message);

            run_error_dialog ("%s", active, cancellable,
                              error->message);
            g_object_unref (direnum);
            g_error_free (error);
            return;
        }
        if (child == NULL) {
            /* dir has been iterated over and is now empty */
            break;
        }
        /* recurse */
        delete_file_recursive (child, active,
                               cancellable);
    }

    if (direnum)
        g_object_unref (direnum);

    g_file_delete (file, cancellable, &error);

    if (error) {
        g_critical ("Error in myfm_delete_operation function "
                   "'delete_file_recursive': %s", error->message);
        run_error_dialog ("%s", active, cancellable, error->message);
        g_error_free (error);
        return;
    }
}

static void
myfm_delete_operation_thread (GTask *task, gpointer src_object,
                              gpointer task_data,
                              GCancellable *cancellable)
{
    MyFMApplication *app;
    GFile **arr;
    GFile *current;

    arr = task_data;
    for (int i = 0; (current = arr[i]); i ++) {
        delete_file_recursive (current, win,
                               cancellable);
        g_object_unref (current);
    }

    g_free (arr);
    g_object_unref (cancellable);

    app = MYFM_APPLICATION (gtk_window_get_application (win));
    myfm_application_set_delete_in_progress (app, FALSE);

    win = NULL;
    g_debug ("finished");
}

static void
myfm_delete_operation_callback_wrapper (GObject *src_object,
                                        GAsyncResult *res,
                                        gpointer _cb)
{
    MyFMDeleteCallback cb = _cb;
    g_object_unref (res);

    if (cb)
        cb (user_data);

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

    arr = g_malloc (sizeof (GFile *) * (n_files + 1));

    for (int i = 0; i < n_files; i ++)
        arr[i] = g_file_dup (myfm_file_get_g_file (src_files[i]));

    /* NULL-terminate */
    arr[n_files] = NULL;

    win = active;
    cancellable = g_cancellable_new ();
    user_data = data;
    app = MYFM_APPLICATION (gtk_window_get_application (active));
    del = g_task_new (NULL, cancellable,
                      myfm_delete_operation_callback_wrapper,
                      cb);

    g_return_if_fail (!myfm_application_delete_in_progress (app));
    myfm_application_set_delete_in_progress (app, TRUE);

    g_task_set_task_data (del, arr, NULL);
    g_task_set_priority (del, G_PRIORITY_DEFAULT);
    g_task_run_in_thread (del, myfm_delete_operation_thread);
}

void
myfm_delete_operation_delete_single_sync (GFile *file,
                                          GtkWindow *active,
                                          GCancellable *cancellable)
{
    /* TODO: error stuff */
    delete_file_recursive (file, active,
                           cancellable);
}
