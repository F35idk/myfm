/*
 * Created by f35 on 26.12.19.
*/

#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-utils.h"
#include "myfm-file-operations.h"
#include "myfm-file-operations-private.h"
#define G_LOG_DOMAIN "myfm-file-operations-copy"

static void
handle_warn_error (MyFMDialogResponse response,
                   _CopyFlags *flags)
{
    switch (response) {
        default : /* do nothing if skip_once */
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

/* forward (backward?) declarations for retry_copy_no_merge () */
static void copy_file_single (GTask *cp, GFile *src, GFile *dest, gboolean merge_once,
                              gboolean make_copy_once, _CopyFlags *flags);
static void copy_dir_recursive (GTask *cp, GFile *src, GFile *dest, gboolean merge_once,
                                gboolean make_copy_once, _CopyFlags *Flags);

/* appends " (copy)" to end of  dest
 * file before copying. unrefs 'src'
 * (through calls to copy_file_single
 * and copy_dir_recursive), so make
 * sure to ref this before calling */
static void
retry_copy_no_merge (GTask *cp,
                     GFile *src,
                     GFile *dest,
                     gboolean is_dir,
                     _CopyFlags *flags)
{
    GFile *new_dest = myfm_utils_new_renamed_g_file (dest);

    if (is_dir)
        copy_dir_recursive (cp, src, new_dest,
                            FALSE, TRUE, flags);
    else
        copy_file_single (cp, src, new_dest,
                          FALSE, TRUE, flags);
}

static void
copy_file_single (GTask *cp,
                  GFile *src,
                  GFile *dest,
                  gboolean merge_once,
                  gboolean make_copy_once,
                  _CopyFlags *flags)
{
    GError *error = NULL;
    MyFMDialogResponse error_response;
    GCancellable *cancellable;

    cancellable = g_task_get_cancellable (cp);

    if (merge_once || flags->merge_all) {
        /* copy w/ overwrite */
        g_file_copy (src, dest,
                     G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_OVERWRITE,
                     cancellable, NULL, NULL, &error);

        if (error) {
            g_critical ("Error in myfm_file_operations_copy function "
                       "'copy_file_single': %s", error->message);

            if (!flags->ignore_warn_errors) {
                error_response = _run_warn_err_dialog (cp, FILE_OPERATION_COPY,
                                                       "%s", error->message);
                handle_warn_error (error_response, flags);
            }
            g_error_free (error);
            g_object_unref (dest);
            g_object_unref (src);
            return;
        }
    }
    else {
        /* copy w/o overwrite */
        g_file_copy (src, dest,
                     G_FILE_COPY_NOFOLLOW_SYMLINKS,
                     cancellable, NULL, NULL, &error);

        if (error) {
            g_critical ("Error in myfm_file_operations_copy function "
                       "'copy_file_single': %s", error->message);
            g_debug ("error in copy w/o overwrite");
            /* handle the file already existing */
            if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
                if (!flags->ignore_merges) {
                    if (!flags->make_copy_all && !make_copy_once) {
                        /* run dialog and handle user response */
                        error_response = _run_merge_dialog (cp, "%s", error->message);
                        switch (error_response) {
                            default :
                                break;
                            case MYFM_DIALOG_RESPONSE_CANCEL :
                                flags->ignore_warn_errors = TRUE;
                                flags->ignore_merges = TRUE;
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE :
                                g_object_ref (src);
                                retry_copy_no_merge (cp, src, dest,
                                                     FALSE, flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL :
                                flags->make_copy_all = TRUE;
                                g_object_ref (src);
                                retry_copy_no_merge (cp, src, dest,
                                                     FALSE, flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE :
                                break;
                            case MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES :
                                flags->ignore_merges = TRUE;
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ONCE :
                                /* retry and pass merge_once = TRUE */
                                g_object_ref (src);
                                g_object_ref (dest);
                                copy_file_single (cp, src, dest,
                                                  TRUE, FALSE, flags);
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ALL :
                                /* set merge_all = TRUE and retry */
                                flags->merge_all = TRUE;
                                g_object_ref (src);
                                g_object_ref (dest);
                                copy_file_single (cp, src, dest,
                                                  FALSE, FALSE, flags);
                                break;
                        }
                    }
                    else if (flags->make_copy_all || make_copy_once) {
                        g_object_ref (src);
                        retry_copy_no_merge (cp, src, dest, FALSE, flags);
                    }
                }
            }
            else {
                if (!flags->ignore_warn_errors) {
                    error_response = _run_warn_err_dialog (cp, FILE_OPERATION_COPY,
                                                           "%s", error->message);
                    handle_warn_error (error_response, flags);
                }
            }
            g_error_free (error);
        }
    }
    g_object_unref (dest);
    g_object_unref (src);
}

static void
copy_dir_recursive (GTask *cp,
                    GFile *src,
                    GFile *dest,
                    gboolean merge_once,
                    gboolean make_copy_once,
                    _CopyFlags *flags)
{
    GFileEnumerator *direnum;
    MyFMDialogResponse error_response;
    GError *error = NULL;
    GCancellable *cancellable;
    GtkWindow *win;

    cancellable = g_task_get_cancellable (cp);
    win = g_object_get_data (G_OBJECT (cp), "win");

    g_file_make_directory (dest, cancellable, &error);

    if (error) {
        GError *del_error = NULL;

        g_critical ("Error in myfm_file_operations_copy function "
                   "'copy_dir_recursive': %s", error->message);
        /* handle dir existing */
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            if (!flags->ignore_merges) {
                if (flags->merge_all || merge_once) {
                    /* delete dest and retry */
                    g_debug ("merging");
                    _delete_file_single (dest, cancellable, &del_error);
                    if (del_error) {
                        g_critical ("Error in myfm_file_operations_copy function "
                                   "'copy_dir_recursive': %s", del_error->message);
                        /* operation will be canceled */
                        _run_fatal_err_dialog (cp, FILE_OPERATION_COPY,
                                               "%s", del_error->message);
                        g_error_free (del_error);
                    }
                    else {
                        g_object_ref (src);
                        g_object_ref (dest);
                        copy_dir_recursive (cp, src, dest,
                                            FALSE, FALSE, flags);
                    }
                }
                else if (make_copy_once || flags->make_copy_all) {
                     /* ref to avoid double free with call to retry_copy */
                    g_object_ref (src);
                    retry_copy_no_merge (cp, src, dest, TRUE, flags);
                }
                else { /* no flags are set */
                    error_response = _run_merge_dialog (cp, "%s", error->message);
                    switch (error_response) {
                        default :
                            break;
                        case MYFM_DIALOG_RESPONSE_CANCEL :
                            flags->ignore_warn_errors = TRUE;
                            flags->ignore_merges = TRUE;
                            break;
                        case MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE :
                            g_object_ref (src);
                            retry_copy_no_merge (cp, src, dest,
                                                 TRUE, flags);
                            break;
                        case MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL :
                            flags->make_copy_all = TRUE;
                            g_object_ref (src);
                            retry_copy_no_merge (cp, src, dest,
                                                 TRUE, flags);
                            break;
                        case MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE :
                            break;
                        case MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES :
                            flags-> ignore_merges = TRUE;
                            break;
                        case MYFM_DIALOG_RESPONSE_MERGE_ONCE :
                            g_object_ref (src);
                            g_object_ref (dest);
                            /* retry with merge_once = TRUE */
                            copy_dir_recursive (cp, src, dest,
                                                TRUE, FALSE, flags);
                            break;
                            break;
                        case MYFM_DIALOG_RESPONSE_MERGE_ALL :
                            flags->merge_all = TRUE;
                            g_object_ref (src);
                            g_object_ref (dest);
                            copy_dir_recursive (cp, src, dest,
                                                FALSE, FALSE, flags);
                            break;
                    }
                }
            }
        }
        else {
            if (!flags->ignore_warn_errors) {
                error_response = _run_warn_err_dialog (cp, FILE_OPERATION_COPY,
                                                       "%s", error->message);
                handle_warn_error (error_response, flags);
            }
        }
        g_error_free (error);
        g_object_unref (dest);
        g_object_unref (src);
        return;
    }

    direnum = g_file_enumerate_children (src, MYFM_FILE_QUERY_ATTRIBUTES,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, &error);
    if (error) {
        g_critical ("Error in myfm_file_operations_copy function "
                   "'copy_dir_recursive': %s", error->message);

        error_response = _run_warn_err_dialog (cp, FILE_OPERATION_COPY,
                                               "%s", error->message);
        handle_warn_error (error_response, flags);
        g_error_free (error);
        /* g_object_unref (direnum); */ /* (direnum is NULL) */
        g_object_unref (dest);
        g_object_unref (src);
        return;
    }

    while (TRUE) {
        GFile *child = NULL;
        GFileInfo *child_info = NULL;
        GError *error2 = NULL;
        gboolean fail;
        GFileType child_type;
        gchar *child_basename;
        gchar *dest_dir_path;
        GFile *new_dest;

        fail = !g_file_enumerator_iterate (direnum, &child_info,
                                           &child, cancellable,
                                           &error2);
        if (fail) {
            g_critical ("Error in myfm_file_operations_copy function "
                       "'copy_dir_recursive': %s", error->message);

            error_response = _run_warn_err_dialog (cp, FILE_OPERATION_COPY,
                                                   "%s", error->message);
            handle_warn_error (error_response, flags);
            g_error_free (error2);
            g_object_unref (direnum);
            return;
        }
        if (child_info == NULL) {
            break;
        }

        /* create new dest g_file */
        child_basename = g_file_get_basename (child);
        dest_dir_path = g_file_get_path (dest);
        new_dest = g_file_new_build_filename (dest_dir_path,
                                              child_basename, NULL);
        g_object_ref (child);
        g_free (child_basename);
        g_free (dest_dir_path);

        child_type = g_file_info_get_file_type (child_info);

        /* recurse */
        if (child_type == G_FILE_TYPE_DIRECTORY)
            copy_dir_recursive (cp, child, new_dest,
                                FALSE, FALSE, flags);
        else
            copy_file_single (cp, child, new_dest,
                              FALSE, FALSE, flags);
    }

    g_object_unref (dest);
    g_object_unref (src);
    g_object_unref (direnum);
}

void
_copy_files_thread (GTask *task,
                    gpointer src_object,
                    gpointer task_data,
                    GCancellable *cancellable)
{
    struct _file_w_type *arr;
    GFile *dest_dir;
    gchar *dest_dir_path;
    _CopyFlags flags;

    flags = (_CopyFlags) {0};
    arr = (struct _file_w_type *) task_data;
    dest_dir = arr[0].g_file;
    dest_dir_path = g_file_get_path (dest_dir);
   
    g_object_unref (dest_dir);

    /* last struct element has g_file = NULL */
    struct _file_w_type *current;
    for (current = arr + 1; (*current).g_file; current++) {
        gchar *src_basename;
        GFile *dest;

        src_basename = g_file_get_basename ((*current).g_file);
        dest = g_file_new_build_filename (dest_dir_path,
                                          src_basename,
                                          NULL);
        g_free (src_basename);

        if ((*current).type == G_FILE_TYPE_DIRECTORY)
            copy_dir_recursive (task, (*current).g_file,
                                dest, FALSE, FALSE, &flags);
        else
            copy_file_single (task, (*current).g_file,
                              dest, FALSE, FALSE, &flags);
    }
    g_free (dest_dir_path);
    g_free (arr);

    return; /* no need for g_task_return_x */
}
