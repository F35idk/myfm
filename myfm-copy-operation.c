/*
 * Created by f35 on 26.12.19.
*/

#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-copy-operation.h"
#define G_LOG_DOMAIN "myfm-copy-operation"

static GtkWindow *win = NULL;
static GCancellable *cp_canceller = NULL;
static gpointer user_data = NULL;

/* flags checked during copy operation
 * (set when handling error responses) */
static gboolean ignore_std_errs = FALSE;
static gboolean ignore_merges = FALSE;
static gboolean make_copy_all = FALSE;
static gboolean merge_all = FALSE;

typedef enum {
    ERROR_TYPE_MERGE,
    ERROR_TYPE_STD, /* currently any error that isn't a merge conflict */
} ErrorType;

typedef enum {
    ERROR_RESPONSE_NONE,
    ERROR_RESPONSE_CANCEL,
    ERROR_RESPONSE_MAKE_COPY_ONCE,
    ERROR_RESPONSE_MAKE_COPY_ALL,
    ERROR_RESPONSE_SKIP_STD_ONCE,
    ERROR_RESPONSE_SKIP_STD_ALL,
    ERROR_RESPONSE_SKIP_MERGE_ONCE,
    ERROR_RESPONSE_SKIP_MERGE_ALL,
    ERROR_RESPONSE_MERGE_ONCE,
    ERROR_RESPONSE_MERGE_ALL,
} ErrorResponse;

static void
on_error_dialog_response (GtkDialog *dialog, gint response_id,
                          gpointer _data)
{
    gpointer *data;
    GtkToggleButton *apply_all;
    ErrorResponse response;
    ErrorType type;
    GMutex *mutex_ptr;
    GCond *cond_ptr;

    data = _data;
    apply_all = data[5];
    type = GPOINTER_TO_INT (data[2]);
    mutex_ptr = data[4];
    cond_ptr = data[3];

    g_mutex_lock (mutex_ptr);

    switch (response_id) {
        default :
            if (type == ERROR_TYPE_MERGE)
                response = ERROR_RESPONSE_SKIP_MERGE_ONCE;
            else
                response = ERROR_RESPONSE_SKIP_STD_ONCE;
            break;
        case 0 : /* 'Cancel' */
            response = ERROR_RESPONSE_CANCEL;
            g_cancellable_cancel (cp_canceller);
            break;
        case 1 : /* 'Make Copy' */
            if (gtk_toggle_button_get_active (apply_all))
                response = ERROR_RESPONSE_MAKE_COPY_ALL;
            else
                response = ERROR_RESPONSE_MAKE_COPY_ONCE;
            break;
        case 2 : /* 'Skip' */
            if (type == ERROR_TYPE_MERGE) {
                if (gtk_toggle_button_get_active (apply_all))
                    response = ERROR_RESPONSE_SKIP_MERGE_ALL;
                else
                    response = ERROR_RESPONSE_SKIP_MERGE_ONCE;
            }
            else {
                if (gtk_toggle_button_get_active (apply_all))
                    response = ERROR_RESPONSE_SKIP_STD_ALL;
                else
                    response = ERROR_RESPONSE_SKIP_STD_ONCE;
            }
            break;
        case 3 : /* 'Replace' */
            if (gtk_toggle_button_get_active (apply_all))
                response = ERROR_RESPONSE_MERGE_ALL;
            else
                response = ERROR_RESPONSE_MERGE_ONCE;
            break;
    }

    data[0] = GINT_TO_POINTER (response);
    g_cond_signal (cond_ptr);
    g_mutex_unlock (mutex_ptr);
}

static gboolean
popup_error_main_ctx (gpointer _data)
{
    GtkWidget *error_dialog;
    gpointer *data;
    ErrorType type;
    gchar *primary;
    GtkWidget *msg_area;
    GtkWidget *check;
    gchar *check_label;
    gchar *msg;
    gchar *title;

    data = _data;
    msg = data[1];
    type = GPOINTER_TO_INT (data[2]);

    if (type == ERROR_TYPE_MERGE) {
        primary = "Replace file?";
        title = "File Conflict";
        check_label = "Apply this action to all file conflicts";
    }
    else {
        primary = "There was an error during the copy operation.";
        title = "File Copy Error";
        check_label = "Apply this action to all errors";
    }

    error_dialog = gtk_message_dialog_new (GTK_WINDOW (win), GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                           "%s", primary);

    g_object_set (G_OBJECT (error_dialog), "secondary-text", msg, NULL);
    gtk_window_set_title (GTK_WINDOW (error_dialog), title);
    g_free (msg);

    if (type == ERROR_TYPE_MERGE )
        gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "Cancel", 0,
                                "Make Copy", 1, "Skip", 2, "Replace", 3, NULL);
    else
        gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "Cancel", 0, "Skip",
                                2, NULL);

    msg_area = gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (error_dialog));
    check = gtk_check_button_new_with_label (check_label);
    gtk_box_pack_end (GTK_BOX (msg_area), check, FALSE, TRUE, 0);
    gtk_widget_show (check);

    data[5] = check;
    g_signal_connect (GTK_DIALOG (error_dialog), "response",
                      G_CALLBACK (on_error_dialog_response), data);

    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (GTK_WIDGET (error_dialog));

    return FALSE;
}

static gint
run_error_dialog (ErrorType type, const gchar *format_msg, ...)
{
    va_list va;
    gchar *msg;
    /* only one copy operation can happen at
     * once, so we can keep these vars static
     * and avoid mallocs (and frees) everywhere */
    static gpointer data[6];
    static GCond cond;
    static GMutex mutex;

    va_start (va, format_msg);
    msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    data[0] = GINT_TO_POINTER (ERROR_RESPONSE_NONE);
    data[1] = msg;
    data[2] = GINT_TO_POINTER (type);
    data[3] = &cond;
    data[4] = &mutex;
    /* we leave 5th index for check_button in above function */

    g_mutex_lock (&mutex);
    g_main_context_invoke (NULL, popup_error_main_ctx, data);

    /* wait on response from error dialog in main thread */
    while (GPOINTER_TO_INT (data[0]) == ERROR_RESPONSE_NONE)
        g_cond_wait (&cond, &mutex);

    g_mutex_unlock (&mutex);
    return GPOINTER_TO_INT (data[0]);
}

static void
handle_std_error (ErrorResponse response)
{
    switch (response) {
        default :
            break;
        case ERROR_RESPONSE_SKIP_STD_ALL :
            ignore_std_errs = TRUE;
            break;
        case ERROR_RESPONSE_CANCEL :
            ignore_std_errs = TRUE;
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
    ErrorResponse error_response;

    if (merge_once || merge_all) {
        /* copy w/ overwrite */
        g_file_copy (src, dest,
                     G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_OVERWRITE,
                     cp_canceller, NULL, NULL, &error);

        if (error) {
            g_critical ("Error in myfm_copy_operation function "
                       "'copy_file_single: %s", error->message);

            if (!ignore_std_errs) {
                error_response = run_error_dialog (ERROR_TYPE_STD,
                                                   "%s", error->message);
                handle_std_error (error_response);
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
                        error_response = run_error_dialog (ERROR_TYPE_MERGE,
                                                          "%s", error->message);
                        switch (error_response) {
                            default:
                                break;
                            case ERROR_RESPONSE_CANCEL:
                                ignore_std_errs = TRUE;
                                ignore_merges = TRUE;
                                break;
                            case ERROR_RESPONSE_MAKE_COPY_ONCE:
                                g_object_ref (src);
                                retry_copy_no_merge (src, dest, FALSE);
                                break;
                            case ERROR_RESPONSE_MAKE_COPY_ALL:
                                make_copy_all = TRUE;
                                g_object_ref (src);
                                retry_copy_no_merge (src, dest, FALSE);
                                break;
                            case ERROR_RESPONSE_SKIP_MERGE_ONCE:
                                break;
                            case ERROR_RESPONSE_SKIP_MERGE_ALL:
                                ignore_merges = TRUE;
                                break;
                            case ERROR_RESPONSE_MERGE_ONCE:
                                /* retry with merge_once = TRUE */
                                copy_file_single (src, dest, TRUE, FALSE);
                                break;
                            case ERROR_RESPONSE_MERGE_ALL:
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
                if (!ignore_std_errs) {
                    error_response = run_error_dialog (ERROR_TYPE_STD,
                                                       "%s", error->message);
                    handle_std_error (error_response);
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
    ErrorResponse error_response;
  
    g_file_make_directory (dest, cp_canceller, &error);

    if (error) {
        g_critical ("Error in myfm_copy_operation function "
                   "'copy_dir_recursive: %s", error->message);
        /* handle dir existing */
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            if (!ignore_merges) {
                if (!make_copy_once && !make_copy_all) {
                    if (!merge_all && !merge_once) {
                        error_response = run_error_dialog (ERROR_TYPE_MERGE,
                                                          "%s", error->message);
                        switch (error_response) {
                            default:
                                break;
                            case ERROR_RESPONSE_CANCEL:
                                ignore_std_errs = TRUE;
                                ignore_merges = TRUE;
                                break;
                            case ERROR_RESPONSE_MAKE_COPY_ONCE:
                                g_object_ref (src);
                                retry_copy_no_merge (src, dest, TRUE);
                                break;
                            case ERROR_RESPONSE_MAKE_COPY_ALL:
                                make_copy_all = TRUE;
                                g_object_ref (src);
                                retry_copy_no_merge (src, dest, TRUE);
                                break;
                            case ERROR_RESPONSE_SKIP_MERGE_ONCE:
                                break;
                            case ERROR_RESPONSE_SKIP_MERGE_ALL:
                                ignore_merges = TRUE;
                                break;
                            case ERROR_RESPONSE_MERGE_ONCE:
                                /* FIXME: need to implement
                                 * rm -rf to do this */
                                break;
                            case ERROR_RESPONSE_MERGE_ALL:
                                merge_all = TRUE;
                                break;
                        }
                    }
                    else if (merge_all || merge_once) {
                        /* TODO: currently we do not have
                         * code for recursive delete, so
                         * overwriting directories isn't
                         * possible to implement yet */
                    }
                }
                else if (make_copy_once || make_copy_all) {
                    g_object_ref (src); /* ref to avoid double free */
                    retry_copy_no_merge (src, dest, TRUE);
                }
            }
        }
        else {
            if (!ignore_std_errs) {
                error_response = run_error_dialog (ERROR_TYPE_STD, "%s",
                                                   error->message);
                handle_std_error (error_response);
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

        error_response = run_error_dialog (ERROR_TYPE_STD, "%s",
                                           error->message);
        handle_std_error (error_response);
        g_error_free (error);
        /* g_object_unref (direnum); */ /* (direnum is NULL) */
        g_object_unref (dest);
        g_object_unref (src);
        return;
    }

    while (TRUE) {
        GFile *child;
        GFileInfo *child_info;
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
                       "'copy_dir_recursive: %s", error->message);

            error_response = run_error_dialog (ERROR_TYPE_STD,
                                              "%s", error->message);
            handle_std_error (error_response);
            g_error_free (error2);
            continue;
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
    struct file_w_type current;
    GFile *dest_dir;
    gchar *dest_dir_path;
    int i;

    arr = (struct file_w_type *) task_data;
    dest_dir = arr[0].g_file;
    dest_dir_path = g_file_get_path (dest_dir);
    g_object_unref (dest_dir);

    i = 1;
    current = arr[i];

    while (current.g_file != NULL) {
        gchar *src_basename;
        GFile *dest;

        src_basename = g_file_get_basename (current.g_file);
        dest = g_file_new_build_filename (dest_dir_path,
                                          src_basename,
                                          NULL);
        g_free (src_basename);

        if (current.type == G_FILE_TYPE_DIRECTORY)
            copy_dir_recursive (current.g_file, dest,
                                FALSE, FALSE);
        else
            copy_file_single (current.g_file, dest,
                              FALSE, FALSE);

        i ++;
        current = arr[i];
    }
    g_free (dest_dir_path);
    g_free (arr);

    /* reset flags/statics */
    g_object_unref (cp_canceller);
    cp_canceller = NULL;
    win = NULL;
    user_data = NULL;
    ignore_std_errs = FALSE;
    ignore_merges = FALSE;
    make_copy_all = FALSE;
    merge_all = FALSE;

    g_debug ("finished");
    return;
}

void
myfm_copy_operation_cancel (void)
{
    if (cp_canceller)
        g_cancellable_cancel (cp_canceller);
}

static void
myfm_copy_operation_callback_wrapper (GObject *src_object,
                                      GAsyncResult *res,
                                      gpointer _cb)
{
    MyFMCopyCallback cb = _cb;
    g_object_unref (res);

    if (cb)
        cb (user_data);
}

void
myfm_copy_operation_start_async (MyFMFile **src_files, gint n_files,
                                 MyFMFile *dest_dir, GtkWindow *active,
                                 MyFMCopyCallback cb, gpointer data)
{
    GTask *cp;
    struct file_w_type *arr; /* array to pass to our g_thread_func */
    GFile *g_dest;

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
                     myfm_copy_operation_callback_wrapper,
                     data);

    g_task_set_task_data (cp, arr, NULL);
    g_task_set_priority (cp, G_PRIORITY_DEFAULT); /* NOTE: redundant */
    g_task_run_in_thread (cp, myfm_copy_operation_thread);

}
