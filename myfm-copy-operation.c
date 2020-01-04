/*
 * Created by f35 on 26.12.19.
*/

#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-utils.h"
#include "myfm-delete-operation.h"
#include "myfm-copy-operation.h"
#define G_LOG_DOMAIN "myfm-copy-operation"

static GtkWindow *win = NULL;
static GCancellable *cp_canceller = NULL;
static gpointer user_data = NULL;

/* flags checked during copy operation
 * (set when handling error responses) */
static gboolean ignore_warn_errors = FALSE;
static gboolean ignore_merges = FALSE;
static gboolean make_copy_all = FALSE;
static gboolean merge_all = FALSE;

/* convenience dialog funcs below */

static MyFMDialogResponse /* (gint) */
run_merge_dialog (const gchar *format_msg, ...)
{
    va_list va;
    va_start (va, format_msg);
    return myfm_utils_run_merge_conflict_dialog_thread (win,
                                                        cp_canceller,
                                                        format_msg,
                                                        va);
}

static MyFMDialogResponse /* (gint) */
run_warn_error_dialog (const gchar *format_msg, ...)
{
    va_list va;
    va_start (va, format_msg);
    return myfm_utils_run_skippable_err_dialog_thread (win, cp_canceller,
                                                      "File Copy Error",
                                                      "There was an err"
                                                      "or during the co"
                                                      "py operation.",
                                                       format_msg, va);
}

static MyFMDialogResponse /* (gint) */
run_fatal_err_dialog (const gchar *format_msg, ...)
{
    va_list va;
    va_start (va, format_msg);
    return myfm_utils_run_unskippable_err_dialog_thread (win, cp_canceller,
                                                         "File Copy Error",
                                                         "There was a fatal"
                                                         " error during the"
                                                         " copy operation.",
                                                          format_msg, va);

}

static void
handle_warn_error (MyFMDialogResponse response)
{
    switch (response) {
        default : /* do nothing if skip_once */
            break;
        case MYFM_DIALOG_RESPONSE_SKIP_ALL_ERRORS :
            ignore_warn_errors = TRUE;
            break;
        case MYFM_DIALOG_RESPONSE_CANCEL :
            ignore_warn_errors = TRUE;
            ignore_merges = TRUE;
            break;
    }
}

/* forward (backward?) declarations for retry_copy_no_merge () */
static void copy_file_single (GFile *src, GFile *dest,
                              gboolean merge_once,
                              gboolean make_copy_once);
static void copy_dir_recursive (GFile *src, GFile *dest,
                                gboolean merge_once,
                                gboolean make_copy_once);

/* appends " (copy)" to end of  dest
 * file before copying. unrefs 'src'
 * (through calls to copy_file_single
 * and copy_dir_recursive), so make
 * sure to ref this before calling */
static void
retry_copy_no_merge (GFile *src,
                     GFile *dest,
                     gboolean is_dir)
{
    gchar *old_path;
    gchar *new_path;
    GFile *new_dest;

    old_path = g_file_get_path (dest);
    new_path = g_strconcat (old_path, " (copy)", NULL);
    new_dest = g_file_new_for_path (new_path);
    g_free (old_path);
    g_free (new_path);

    if (is_dir)
        copy_dir_recursive (src, new_dest,
                            FALSE, TRUE);
    else
        copy_file_single (src, new_dest,
                          FALSE, TRUE);
}

static void
copy_file_single (GFile *src,
                  GFile *dest,
                  gboolean merge_once,
                  gboolean make_copy_once)
{
    GError *error = NULL;
    MyFMDialogResponse error_response;

    if (merge_once || merge_all) {
        /* copy w/ overwrite */
        g_file_copy (src, dest,
                     G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_OVERWRITE,
                     cp_canceller, NULL, NULL, &error);

        if (error) {
            g_critical ("Error in myfm_copy_operation function "
                       "'copy_file_single: %s", error->message);

            if (!ignore_warn_errors) {
                error_response = run_warn_error_dialog ("%s", error->message);
                handle_warn_error (error_response);
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
                     cp_canceller, NULL, NULL, &error);

        if (error) {
            g_critical ("Error in myfm_copy_operation function "
                       "'copy_file_single: %s", error->message);
            g_debug ("error in copy w/o overwrite");
            /* handle the file already existing */
            if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
                if (!ignore_merges) {
                    if (!make_copy_all && !make_copy_once) {
                        /* run dialog and handle user response */
                        error_response = run_merge_dialog ("%s", error->message);
                        switch (error_response) {
                            default:
                                break;
                            case MYFM_DIALOG_RESPONSE_CANCEL:
                                ignore_warn_errors = TRUE;
                                ignore_merges = TRUE;
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE:
                                g_object_ref (src);
                                retry_copy_no_merge (src, dest, FALSE);
                                break;
                            case MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL:
                                make_copy_all = TRUE;
                                g_object_ref (src);
                                retry_copy_no_merge (src, dest, FALSE);
                                break;
                            case MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE:
                                break;
                            case MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES:
                                ignore_merges = TRUE;
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ONCE:
                                /* retry with merge_once = TRUE */
                                copy_file_single (src, dest, TRUE, FALSE);
                                break;
                            case MYFM_DIALOG_RESPONSE_MERGE_ALL:
                                merge_all = TRUE;
                                break;
                        }
                    }
                    else if (make_copy_all || make_copy_once) {
                        g_object_ref (src);
                        retry_copy_no_merge (src, dest, FALSE);
                    }
                }
            }
            else {
                if (!ignore_warn_errors) {
                    error_response = run_warn_error_dialog ("%s", error->message);
                    handle_warn_error (error_response);
                }
            }
            g_error_free (error);
            g_object_unref (dest);
            g_object_unref (src);
            return;
        }
    }
    g_object_unref (dest);
    g_object_unref (src);
}

static void
copy_dir_recursive (GFile *src,
                    GFile *dest,
                    gboolean merge_once,
                    gboolean make_copy_once)
{
    GFileEnumerator *direnum;
    GError *error = NULL;
    MyFMDialogResponse error_response;

    if (merge_all || merge_once) {
        myfm_delete_operation_delete_single_sync (dest, win,
                                                  cp_canceller,
                                                  &error);

        if (error) {
            g_critical ("Error in myfm_copy_operation function "
                       "'copy_dir_recursive': %s", error->message);
            /* operation will be canceled */
            run_fatal_err_dialog ("%s", error->message);
            g_error_free (error);
            return;
        }
    }

    g_file_make_directory (dest, cp_canceller, &error);

    if (error) {
        g_critical ("Error in myfm_copy_operation function "
                   "'copy_dir_recursive: %s", error->message);
        /* handle dir existing */
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            if (!ignore_merges) {
                if (!make_copy_once && !make_copy_all) {
                    error_response = run_merge_dialog ("%s", error->message);
                    switch (error_response) {
                        default:
                            break;
                        case MYFM_DIALOG_RESPONSE_CANCEL:
                            ignore_warn_errors = TRUE;
                            ignore_merges = TRUE;
                            break;
                        case MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE:
                            g_object_ref (src);
                            retry_copy_no_merge (src, dest, TRUE);
                            break;
                        case MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL:
                            make_copy_all = TRUE;
                            g_object_ref (src);
                            retry_copy_no_merge (src, dest, TRUE);
                            break;
                        case MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE:
                            break;
                        case MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES:
                            ignore_merges = TRUE;
                            break;
                        case MYFM_DIALOG_RESPONSE_MERGE_ONCE:
                            g_object_ref (src);
                            g_object_ref (dest);
                            /* retry with merge_once = TRUE */
                            copy_dir_recursive (src, dest, TRUE, FALSE);
                            break;
                        case MYFM_DIALOG_RESPONSE_MERGE_ALL:
                            merge_all = TRUE;
                            g_object_ref (src);
                            g_object_ref (dest);
                            copy_dir_recursive (src, dest, FALSE, FALSE);
                            break;
                    }
                }
                else if (make_copy_once || make_copy_all) {
                     /* ref to avoid double free with call to retry_copy */
                    g_object_ref (src);
                    retry_copy_no_merge (src, dest, TRUE);
                }
            }
        }
        else {
            if (!ignore_warn_errors) {
                error_response = run_warn_error_dialog ("%s", error->message);
                handle_warn_error (error_response);
            }
        }
        g_error_free (error);
        g_object_unref (dest);
        g_object_unref (src);
        return;
    }

    direnum = g_file_enumerate_children (src, MYFM_FILE_QUERY_ATTRIBUTES,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cp_canceller, &error);
    if (error) {
        g_critical ("Error in myfm_copy_operation function "
                   "'copy_dir_recursive: %s", error->message);

        error_response = run_warn_error_dialog ("%s", error->message);
        handle_warn_error (error_response);
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
                                           &child, cp_canceller,
                                           &error2);
        if (fail) {
            g_critical ("Error in myfm_copy_operation function "
                       "'copy_dir_recursive': %s", error->message);

            error_response = run_warn_error_dialog ("%s", error->message);
            handle_warn_error (error_response);
            g_error_free (error2);
            g_object_unref (direnum);
            return; /* NOTE: continue instead? probably not */
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
            copy_dir_recursive (child, new_dest, FALSE, FALSE);
        else
            copy_file_single (child, new_dest, FALSE, FALSE);
    }

    g_object_unref (dest);
    g_object_unref (src);
    g_object_unref (direnum);
}

/* struct for passing g_files and
 * their type to our g_thread_func */
struct file_w_type {
    GFile *g_file;
    GFileType type;
};

static void
myfm_copy_operation_thread (GTask *task, gpointer src_object,
                            gpointer task_data,
                            GCancellable *cancellable)
{
    struct file_w_type *arr;
    GFile *dest_dir;
    gchar *dest_dir_path;

    arr = (struct file_w_type *) task_data;
    dest_dir = arr[0].g_file;
    dest_dir_path = g_file_get_path (dest_dir);
    g_object_unref (dest_dir);

    /* last struct element has g_file = NULL */
    struct file_w_type *current;
    for (current = arr + 1; (*current).g_file; current++) {
        gchar *src_basename;
        GFile *dest;

        src_basename = g_file_get_basename ((*current).g_file);
        dest = g_file_new_build_filename (dest_dir_path,
                                          src_basename,
                                          NULL);
        g_free (src_basename);

        if ((*current).type == G_FILE_TYPE_DIRECTORY)
            copy_dir_recursive ((*current).g_file, dest,
                                FALSE, FALSE);
        else
            copy_file_single ((*current).g_file, dest,
                              FALSE, FALSE);
    }
    g_free (dest_dir_path);
    g_free (arr);

    g_debug ("finished");
    return; /* no need for g_task_return_x */
}

void
myfm_copy_operation_cancel (void)
{
    if (cp_canceller)
        g_cancellable_cancel (cp_canceller);
}

/* TODO: pass myfm_file array
 * to user callback through this */
static void
myfm_copy_operation_finish (GObject *src_object,
                            GAsyncResult *res,
                            gpointer _cb)
{
    MyFMApplication *app;
    MyFMCopyCallback cb;

    cb = _cb;
    if (cb)
        cb (user_data);

    app = MYFM_APPLICATION (gtk_window_get_application (win));
    myfm_application_set_copy_in_progress (app, FALSE);
    g_object_unref (cp_canceller);
    g_object_unref (res);

    /* reset flags/statics */
    win = NULL;
    cp_canceller = NULL;
    user_data = NULL;
    ignore_warn_errors = FALSE;
    ignore_merges = FALSE;
    make_copy_all = FALSE;
    merge_all = FALSE;
}

void
myfm_copy_operation_start_async (MyFMFile **src_files, gint n_files,
                                 MyFMFile *dest_dir, GtkWindow *active,
                                 MyFMCopyCallback cb, gpointer data)
{
    GTask *cp;
    struct file_w_type *arr; /* array to pass to our g_thread_func */
    GFile *g_dest;
    MyFMApplication *app;

    app = MYFM_APPLICATION (gtk_window_get_application (active));
    g_return_if_fail (!myfm_application_copy_in_progress (app));
    g_return_if_fail (active != NULL);

    arr = g_malloc (sizeof (struct file_w_type) * (n_files + 2));
    g_dest = g_file_dup (myfm_file_get_g_file (dest_dir));
    /* set dest as first element of array */
    arr[0] = (struct file_w_type) {g_dest, G_FILE_TYPE_DIRECTORY};

    /* fill array with g_files and filetypes */
    for (int i = 1; i <= n_files; i ++) {
        GFile *dup;
        GFileType type;
        struct file_w_type ft;

        /* NOTE: we duplicate for safety */
        dup = g_file_dup (myfm_file_get_g_file (src_files[i-1]));
        type = myfm_file_get_filetype (src_files[i-1]);
        ft = (struct file_w_type) {dup, type};
        arr[i] = ft;
    }
    /* 'NULL'-terminate array */
    arr[n_files + 1] = (struct file_w_type) {NULL, 0};

    win = active;
    cp_canceller = g_cancellable_new ();
    user_data = data;
    cp = g_task_new (NULL, cp_canceller,
                     myfm_copy_operation_finish,
                     cb);

    myfm_application_set_copy_in_progress (app, TRUE);

    g_task_set_task_data (cp, arr, NULL);
    g_task_set_priority (cp, G_PRIORITY_DEFAULT); /* NOTE: redundant */
    g_task_run_in_thread (cp, myfm_copy_operation_thread);

}
