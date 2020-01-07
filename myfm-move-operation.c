/*
 * Created by f35 on 04.01.20.
*/

#include <gio/gio.h>

#include "myfm-utils.h"
#include "myfm-delete-operation.h"
#include "myfm-move-operation.h"
#define G_LOG_DOMAIN "myfm-move-operation"

static GtkWindow *win = NULL;
gpointer user_data = NULL;

/* NOTE: since similar 'run_dialog' convenience
 * wrappers exist in both copy and delete
 * operation, consider refactoring? by
 * improving dialog functions in myfm_utils? */

static MyFMDialogResponse
run_merge_dialog (GCancellable *cancellable,
                  const gchar *format_msg, ...)
{
    va_list va;
    va_start (va, format_msg);
    return myfm_utils_run_merge_conflict_dialog_thread (win,
                                                        cancellable,
                                                        format_msg, va);
}

static MyFMDialogResponse
run_warn_error (GCancellable *cancellable,
                const gchar *format_msg, ...)
{
    va_list va;
    va_start (va, format_msg);
    return myfm_utils_run_skippable_err_dialog_thread (win,
                                                       cancellable,
                                                       "File Move Error",
                                                       "There was an er"
                                                       "ror during the m"
                                                       "ove operation.",
                                                       format_msg, va);
}

/* TODO: make our move flags a combinable
 * bitflag enum instead of this bool struct.
 * then we could return them directly from our
 * convenience dialog functions and thus avoid
 * having to manually set them every time we
 * handle dialog responses. the same could be
 * done for myfm-copy-operation */
typedef struct {
    gboolean use_fallback;
    gboolean merge_all;
    gboolean merge_once;
    gboolean make_copy_once;
    gboolean make_copy_all;
    gboolean ignore_merges;
    gboolean ignore_warn_errors;
} MoveFlags;

static void
move_file_fallback (GFile *src, GFile *dest,
                    GCancellable *cancellable,
                    MoveFlags *flags)
{
    /* FIXME: implement fallback copy + delete code */

    g_object_unref (src);
    g_object_unref (dest);
}

static void
move_file_default (GFile *src, GFile *dest,
                   GCancellable *cancellable,
                   MoveFlags *flags)
{
    GError *error = NULL;

    g_file_move (src, dest,
                 G_FILE_COPY_NOFOLLOW_SYMLINKS |
                 G_FILE_COPY_NO_FALLBACK_FOR_MOVE,
                 cancellable, NULL, NULL, &error);

    if (error) {
        GError *del_error = NULL;
        MyFMDialogResponse response;

        g_critical ("Error in myfm_move_operation funct"
                    "ion 'myfm_move_operation_thread: %s",
                    error->message);

        switch (error->code) {
            default :
                if (!flags->ignore_warn_errors) {
                    /* run standard warning dialog */
                    response = run_warn_error (cancellable, "%s",
                                               error->message);
                    switch (response ) {
                        default :
                            break;
                        case MYFM_DIALOG_RESPONSE_SKIP_ALL_ERRORS :
                            flags->ignore_warn_errors = TRUE;
                            break;
                        case MYFM_DIALOG_RESPONSE_CANCEL :
                            flags->ignore_warn_errors = TRUE;
                            flags->ignore_merges = TRUE;
                            break;
                    }
                }
                break;
            case G_IO_ERROR_NOT_SUPPORTED :
            case G_IO_ERROR_WOULD_RECURSE :
                /* retry and use fallback code */
                flags->use_fallback = TRUE;
                g_object_ref (src);
                g_object_ref (dest);
                move_file_default (src, dest, cancellable, flags);
                break;
            case G_IO_ERROR_EXISTS :
                /* handle dest file already existing */
                if (!flags->ignore_merges) {
                    if (flags->merge_all || flags->merge_once) {
                        /* remove dest and retry */
                        g_debug ("merging");
                        myfm_delete_operation_delete_single (dest, win,
                                                             cancellable,
                                                             &del_error);
                        if (del_error) {
                            /* FIXME: handle */
                            g_error_free (del_error);
                        }

                        g_object_ref (src);
                        g_object_ref (dest);
                        flags->merge_once = FALSE;
                        move_file_default (src, dest, cancellable, flags);
                    }
                    else if (flags->make_copy_once || flags->make_copy_all) {
                        /* append " (copy)" to dest name and retry */
                        GFile *new_dest = myfm_utils_new_renamed_g_file (dest);

                        g_object_ref (src);
                        move_file_default (src, new_dest, cancellable, flags);
                    }
                    else { /* no flags are set */
                        /* run dialog for merge conflicts
                         * and handle user response */
                        response = run_merge_dialog (cancellable, "%s",
                                                     error->message);
                        switch (response) {
                            default :
                                break;
                            case MYFM_DIALOG_RESPONSE_CANCEL :
                                flags->ignore_warn_errors = TRUE;
                                flags->ignore_merges = TRUE;
                                break;
                            case MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES :
                                flags->ignore_merges = TRUE;
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ALL :
                                flags->merge_all = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (src, dest,
                                                   cancellable,
                                                   flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ONCE :
                                flags->merge_once = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (src, dest,
                                                   cancellable,
                                                   flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE :
                                flags->make_copy_once = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (src, dest,
                                                   cancellable,
                                                   flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL :
                                flags->make_copy_all = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (src, dest,
                                                   cancellable,
                                                   flags);
                                break;
                        }
                    }
                }
                break;
        }
        g_error_free (error);
    }
    g_object_unref (src);
    g_object_unref (dest);
}

static void
myfm_move_operation_thread (GTask *task, gpointer src_object,
                            gpointer task_data,
                            GCancellable *cancellable)
{
    GFile **arr;
    gchar *dest_dir_path;
    GFile *current;
    MoveFlags flags;

    arr = task_data;
    dest_dir_path = g_file_get_path (arr[0]);
    flags = (MoveFlags) {0};

    for (int i = 1; (current = arr[i]); i ++) {
        GFile *dest;
        gchar *src_basename;

        src_basename = g_file_get_basename (current);
        dest = g_file_new_build_filename (dest_dir_path,
                                          src_basename,
                                          NULL);

        if (!flags.use_fallback) {
            move_file_default (current, dest,
                               cancellable, &flags);
        }
        else {
            move_file_fallback (current, dest,
                                cancellable, &flags);
        }
    }
    g_free (arr);

    /* NOTE: this has to know whether it was cancelled or not */
}

static void
myfm_move_operation_finish (GObject *src_object,
                            GAsyncResult *res,
                            gpointer _cb)
{

    g_debug ("finished");
}

/* NOTE: we need to use g_file_copy_attributes () in fallback
 * copy + delete code to transfer attributes properly */

void
myfm_move_operation_start_async (MyFMFile **src_files, gint n_files,
                                 MyFMFile *dest_dir, GtkWindow *active,
                                 MyFMMoveCallback cb, gpointer data)
{
    GTask *move;
    GCancellable *cancellable;
    MyFMApplication *app;
    GFile **arr;

    app = MYFM_APPLICATION (gtk_window_get_application (active));
    // g_return_if_fail (!myfm_application_move_in_progress (app));
    g_return_if_fail (active != NULL);

    arr = g_malloc (sizeof (GFile *) * (n_files + 2));
    arr[0] = myfm_file_get_g_file (dest_dir);

    for (int i = 0; i < n_files; i ++)
        arr[i + 1] = g_file_dup (myfm_file_get_g_file (src_files[i]));

    /* NULL-terminate */
    arr[n_files + 1] = NULL;

    win = active;
    user_data = data;
    cancellable = g_cancellable_new ();
    move = g_task_new (NULL, cancellable,
                       myfm_move_operation_finish,
                       cb);

    // myfm_application_set_move_in_progress (app, TRUE);

    g_task_set_task_data (move, arr, NULL);
    g_task_set_priority (move, G_PRIORITY_DEFAULT);
    g_task_run_in_thread (move, myfm_move_operation_thread);
}
