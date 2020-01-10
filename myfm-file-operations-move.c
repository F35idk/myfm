/*
 * Created by f35 on 04.01.20.
*/

#include <gio/gio.h>

#include "myfm-utils.h"
#include "myfm-file-operations.h"
#include "myfm-file-operations-private.h"
#define G_LOG_DOMAIN "myfm-file-operations-move"

static void
move_file_fallback (GTask *move, GFile *src,
                    GFile *dest, _MoveFlags *flags)
{
    /* FIXME: implement fallback copy + delete code */
    /* NOTE: need to use g_file_copy_attributes () here
     * to make sure attributes transfer properly */
    g_object_unref (src);
    g_object_unref (dest);
}

static void
move_file_default (GTask *move, GFile *src,
                   GFile *dest, _MoveFlags *flags)
{
    GError *error = NULL;
    GtkWindow *win;
    GCancellable *cancellable;

    win = g_object_get_data (G_OBJECT (move), "win");
    cancellable = g_task_get_cancellable (move);

    g_file_move (src, dest,
                 G_FILE_COPY_NOFOLLOW_SYMLINKS |
                 G_FILE_COPY_NO_FALLBACK_FOR_MOVE,
                 cancellable, NULL, NULL, &error);

    if (error) {
        GError *del_error = NULL;
        MyFMDialogResponse response;

        g_critical ("Error in myfm_file_operati"
                    "ons_move function 'move_fi"
                    "le_default: %s",
                    error->message);

        switch (error->code) {
            default :
                if (!flags->ignore_warn_errors) {
                    /* run standard warning dialog */
                    response = _run_warn_err_dialog (move, FILE_OPERATION_MOVE,
                                                     "%s", error->message);
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
                move_file_default (move, src, dest, flags);
                break;
            case G_IO_ERROR_EXISTS :
                /* handle dest file already existing */
                if (!flags->ignore_merges) {
                    if (flags->merge_all || flags->merge_once) {
                        /* remove dest and retry */
                        g_debug ("merging");
                        _delete_file_single (dest, cancellable, &del_error);
                        if (del_error) {
                            g_critical ("Error in myfm_file_operations_move function "
                                       "'move_file_default': %s", del_error->message);
                            /* operation will be canceled */
                            _run_fatal_err_dialog (move, FILE_OPERATION_COPY,
                                                   "%s", del_error->message);
                            g_error_free (del_error);
                        }

                        g_object_ref (src);
                        g_object_ref (dest);
                        flags->merge_once = FALSE;
                        move_file_default (move, src, dest, flags);
                    }
                    else if (flags->make_copy_once || flags->make_copy_all) {
                        /* append " (copy)" to dest name and retry */
                        GFile *new_dest = myfm_utils_new_renamed_g_file (dest);

                        g_object_ref (src);
                        flags->make_copy_once = FALSE;
                        move_file_default (move, src, new_dest, flags);
                    }
                    else { /* no flags are set */
                        /* run dialog for merge conflicts
                         * and handle user response */
                        response = _run_merge_dialog (move, "%s",
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
                                move_file_default (move, src, dest, flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ONCE :
                                flags->merge_once = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (move, src, dest, flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE :
                                flags->make_copy_once = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (move, src, dest, flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL :
                                flags->make_copy_all = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                move_file_default (move, src, dest, flags);
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

void
_move_files_thread (GTask *task,
                    gpointer src_object,
                    gpointer task_data,
                    GCancellable *cancellable)
{
    GFile **arr;
    gchar *dest_dir_path;
    GFile *current;
    _MoveFlags flags;

    arr = task_data;
    dest_dir_path = g_file_get_path (arr[0]);
    flags = (_MoveFlags) {0};

    for (int i = 1; (current = arr[i]); i ++) {
        GFile *dest;
        gchar *src_basename;

        src_basename = g_file_get_basename (current);
        dest = g_file_new_build_filename (dest_dir_path,
                                          src_basename,
                                          NULL);

        if (!flags.use_fallback)
            move_file_default (task, current,
                               dest, &flags);
        else
            move_file_fallback (task, current,
                                dest, &flags);
    }
    g_free (arr);

    /* NOTE: this has to know whether it was cancelled or not */
}
