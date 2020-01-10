/*
 * Created by f35 on 08.01.20.
*/

#include <gio/gio.h>

#include "myfm-utils.h"
#include "myfm-file-operations-private.h"
#include "myfm-file-operations.h"
#define G_LOG_DOMAIN "myfm-file-operations"

/* dialog functions used in copy, delete and move files
 * (declared in myfm-file-operations-private.h) */

MyFMDialogResponse /* (gint) */
_run_merge_dialog (GTask *operation,
                   const gchar *format_msg,
                   ...)
{
    va_list va;
    GCancellable *cancellable;
    GtkWindow *win;

    win = g_object_get_data (G_OBJECT (operation), "win");
    cancellable = g_task_get_cancellable (operation);

    va_start (va, format_msg);
    return myfm_utils_run_merge_conflict_dialog_thread (win,
                                                        cancellable,
                                                        format_msg, va);
}

MyFMDialogResponse /* (gint) */
_run_warn_err_dialog (GTask *operation,
                      _FileOpType type,
                      const gchar *format_msg,
                      ...)
{
    va_list va;
    GCancellable *cancellable;
    GtkWindow *win;
    gchar *title;
    gchar *primary;

    win = g_object_get_data (G_OBJECT (operation), "win");
    cancellable = g_task_get_cancellable (operation);

    if (type == FILE_OPERATION_COPY) {
        title = "File Copy Error";
        primary = "There was an error during "
                  "the copy operation.";
    }
    else if (type == FILE_OPERATION_MOVE) {
        title = "File Move Error";
        primary = "There was an error during "
                  "the move operation.";
    }
    else {
        return 0;
    }

    va_start (va, format_msg);
    return myfm_utils_run_skippable_err_dialog_thread (win,
                                                       cancellable,
                                                       title, primary,
                                                       format_msg, va);
}

MyFMDialogResponse /* (gint) */
_run_fatal_err_dialog (GTask *operation,
                       _FileOpType type,
                       const gchar *format_msg, ...)
{
    va_list va;
    GCancellable *cancellable;
    GtkWindow *win;
    gchar *title;
    gchar *primary;

    win = g_object_get_data (G_OBJECT (operation), "win");
    cancellable = g_task_get_cancellable (operation);

    if (type == FILE_OPERATION_COPY) {
        title = "File Copy Error";
        primary = "There was a fatal error "
                  "during the copy operation.";
    }
    else if (type == FILE_OPERATION_MOVE) {
        title = "File Move Error";
        primary = "There was a fatal error "
                  "during the move operation.";
    }
    else {
        title = "File Delete Error";
        primary = "There was a fatal error "
                  "during the delete operation.";
    }

    return myfm_utils_run_unskippable_err_dialog_thread (win, cancellable,
                                                         title, primary,
                                                          format_msg, va);
}

/* TODO: pass myfm_file array
 * to user callback through this */
static void
finish_operation_callback (GObject* src_object,
                           GAsyncResult *res,
                           gpointer _cb)
{
    MyFMFileOpCallback cb;
    GCancellable *cancellable;
    gpointer user_data;

    cb = _cb;
    cancellable = g_task_get_cancellable (G_TASK (res));
    user_data = g_object_get_data (G_OBJECT (res), "user_data");

    if (cb)
        cb (user_data);

    g_debug ("finished");
    g_object_unref (cancellable);
    g_object_unref (res);

}

static GTask *
new_file_op_task (gpointer files,
                  GtkWindow *active,
                  MyFMFileOpCallback user_cb,
                  gpointer user_data)

{
    GTask *task;

    task = g_task_new (NULL, g_cancellable_new (),
                       finish_operation_callback,
                       user_cb);

    g_task_set_task_data (task, files, NULL);
    g_object_set_data (G_OBJECT (task), "win", active);
    g_object_set_data (G_OBJECT (task), "user_data", user_data);
    g_task_set_priority (task, G_PRIORITY_DEFAULT); /* NOTE: redundant */

    return task;
}

void
myfm_file_operations_copy_async (MyFMFile **src_files, gint n_files,
                                 MyFMFile *dest_dir, GtkWindow *active,
                                 MyFMFileOpCallback cb, gpointer data)
{
    GTask *cp;
    struct _file_w_type *arr; /* array to pass to our g_thread_func */
    GFile *g_dest;

    g_return_if_fail (active != NULL);

    arr = g_malloc (sizeof (struct _file_w_type) * (n_files + 2));
    g_dest = g_file_dup (myfm_file_get_g_file (dest_dir));
    /* set dest as first element of array */
    arr[0] = (struct _file_w_type) {g_dest, G_FILE_TYPE_DIRECTORY};

    /* fill array with g_files and filetypes */
    for (int i = 1; i <= n_files; i ++) {
        GFile *dup;
        GFileType type;
        struct _file_w_type ft;

        /* NOTE: we duplicate for safety */
        dup = g_file_dup (myfm_file_get_g_file (src_files[i-1]));
        type = myfm_file_get_filetype (src_files[i-1]);
        ft = (struct _file_w_type) {dup, type};
        arr[i] = ft;
    }

    /* 'NULL'-terminate array */
    arr[n_files + 1] = (struct _file_w_type) {NULL, 0};
    cp = new_file_op_task (arr, active, cb, data);

    g_task_run_in_thread (cp, _copy_files_thread);
}

void
myfm_file_operations_move_async (MyFMFile **src_files, gint n_files,
                                 MyFMFile *dest_dir, GtkWindow *active,
                                 MyFMFileOpCallback cb, gpointer data)
{
    GTask *move;
    GFile **arr;

    g_return_if_fail (active != NULL);

    arr = g_malloc (sizeof (GFile *) * (n_files + 2));
    arr[0] = g_file_dup (myfm_file_get_g_file (dest_dir));

    for (int i = 0; i < n_files; i ++)
        arr[i + 1] = g_file_dup (myfm_file_get_g_file (src_files[i]));

    /* NULL-terminate */
    arr[n_files + 1] = NULL;
    move = new_file_op_task (arr, active, cb, data);

    g_task_run_in_thread (move, _move_files_thread);
}

/* TODO: if file is in clipboard, simply mark the
 * file as cut instead of starting delete operation? */
void
myfm_file_operations_delete_async (MyFMFile **src_files, gint n_files,
                                   GtkWindow *active, MyFMFileOpCallback cb,
                                   gpointer data)
{
    GTask *del;
    GFile **arr;

    g_return_if_fail (active != NULL);

    arr = g_malloc (sizeof (GFile *) * (n_files + 1));

    for (int i = 0; i < n_files; i ++)
        arr[i] = g_file_dup (myfm_file_get_g_file (src_files[i]));

    /* NULL-terminate */
    arr[n_files] = NULL;
    del = new_file_op_task (arr, active, cb, data);

    g_task_run_in_thread (del, _delete_files_thread);
}
