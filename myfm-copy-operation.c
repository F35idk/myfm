/*
 * Created by f35 on 26.12.19.
*/

#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-copy-operation.h"

static GCancellable *cp_canceller = NULL;
static gint io_pr = G_PRIORITY_LOW; /* FIXME: default? */
static GtkWindow *win = NULL;

typedef enum {
    MYFM_RESPONSE_NONE,
    MYFM_RESPONSE_CANCEL,
    MYFM_RESPONSE_MAKE_COPY_ONCE,
    MYFM_RESPONSE_MAKE_COPY_ALL,
    MYFM_RESPONSE_SKIP_ONCE,
    MYFM_RESPONSE_SKIP_ALL,
    MYFM_RESPONSE_REPLACE_ONCE,
    MYFM_RESPONSE_REPLACE_ALL,
} ErrorResponse;

/* we use a condition variable to wait
 * on user response from error dialogs
 * we popup in the main thread */
static GCond cond;
static GMutex mutex;
static gboolean responded = FALSE;
static ErrorResponse response = MYFM_RESPONSE_NONE;

static gboolean skip_all = FALSE;
static gboolean make_copy_all = FALSE;
static gboolean overwrite_all = FALSE;

static void
on_error_dialog_response (GtkDialog *dialog, gint response_id,
                          gpointer apply_all)
{
    g_mutex_lock (&mutex);

    switch (response_id) {
        case 0 : /* 'Cancel' */
            g_cancellable_cancel (cp_canceller);
            response = MYFM_RESPONSE_CANCEL;
            break;
        case 1 : /* 'Make Copy' */
            if (gtk_toggle_button_get_active (apply_all))
                response = MYFM_RESPONSE_MAKE_COPY_ALL;
            else
                response = MYFM_RESPONSE_MAKE_COPY_ONCE;
            break;
        case 2 : /* 'Skip' */
            if (gtk_toggle_button_get_active (apply_all))
                response = MYFM_RESPONSE_SKIP_ALL;
            else
                response = MYFM_RESPONSE_SKIP_ONCE;
            break;
        case 3 : /* 'Replace' */
            if (gtk_toggle_button_get_active (apply_all))
                response = MYFM_RESPONSE_REPLACE_ALL;
            else
                response = MYFM_RESPONSE_REPLACE_ONCE;
            break;
    }

    responded = TRUE;
    g_cond_signal (&cond);
    g_mutex_unlock (&mutex);
}

static gboolean
popup_error_main_ctx (gpointer msg)
{
    GtkWidget *error_dialog;
    GtkWidget *msg_area;
    GtkWidget *check;

    error_dialog = gtk_message_dialog_new (GTK_WINDOW (win), GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
                                           "There was an error during the copy "
                                           "operation.");

    g_object_set (G_OBJECT (error_dialog), "secondary-text", msg, NULL);
    gtk_window_set_title (GTK_WINDOW (error_dialog), "Error");
    gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "Cancel", 0,
                            "Skip", 2, NULL);

    msg_area = gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (error_dialog));
    check = gtk_check_button_new_with_label ("Apply to all files");
    gtk_box_pack_end (GTK_BOX (msg_area), check, FALSE, TRUE, 0);
    gtk_widget_show (check);

    g_signal_connect (GTK_DIALOG (error_dialog), "response",
                       G_CALLBACK (on_error_dialog_response), check);

    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (GTK_WIDGET (error_dialog));

    return FALSE;
}

static gboolean
popup_overwrite_error_main_ctx (gpointer msg)
{
    GtkWidget *error_dialog;
    GtkWidget *msg_area;
    GtkWidget *check;

    /* FIXME: more specific overwrite error messages */
    error_dialog = gtk_message_dialog_new (GTK_WINDOW (win), GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                                           "Replace file? ");

    g_object_set (G_OBJECT (error_dialog), "secondary-text", msg, NULL);
    g_free (msg);
    gtk_window_set_title (GTK_WINDOW (error_dialog), "Error");
    gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "Cancel", 0,
                            "Make Copy", 1, "Skip", 2, "Replace", 3, NULL);

    msg_area = gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (error_dialog));
    check = gtk_check_button_new_with_label ("Apply to all files");
    gtk_box_pack_end (GTK_BOX (msg_area), check, FALSE, TRUE, 0);
    gtk_widget_show (check);

    g_signal_connect (GTK_DIALOG (error_dialog), "response",
                      G_CALLBACK (on_error_dialog_response), check);

    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (GTK_WIDGET (error_dialog));

    return FALSE;
}

static void
popup_overwrite_error_dialog (const gchar *format_msg, ...)
{
    va_list va;
    gchar *msg;

    va_start (va, format_msg);
    msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    g_mutex_lock (&mutex);

    g_main_context_invoke (NULL, popup_overwrite_error_main_ctx,
                           msg);
    while (!responded)
        g_cond_wait (&cond, &mutex);

    /* 'response' flag is now set */
   
    g_mutex_unlock (&mutex);
}

static void
popup_error_dialog (const gchar *format_msg, ...)
{
    va_list va;
    gchar *msg;

    va_start (va, format_msg);
    msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    g_mutex_lock (&mutex);

    g_main_context_invoke (NULL, popup_error_main_ctx,
                           msg);
    while (!responded)
        g_cond_wait (&cond, &mutex);

    g_mutex_unlock (&mutex);
}

static void
copy_file_single (GFile *src,
                  gchar *dest_dir_path,
                  GCancellable *cancellable,
                  gboolean overwrite_once)
{
    GFile *dest;
    gchar *src_basename;
    GError *error = NULL;
    src_basename = g_file_get_basename (src);
    dest = g_file_new_build_filename (dest_dir_path,
                                      src_basename, NULL);
    if (overwrite_once || overwrite_all) {
        g_file_copy (src, dest,
                     G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_OVERWRITE,
                     cancellable, NULL, NULL, &error);

        if (error) {
            popup_error_dialog ("Error while copying file: %s");

            switch (response) {
                case MYFM_RESPONSE_CANCEL :
                    /* FIXME: handle */
                    break;
                case MYFM_RESPONSE_NONE :
                case MYFM_RESPONSE_SKIP_ONCE :
                    break;
                case MYFM_RESPONSE_SKIP_ALL :
                    skip_all = TRUE;
                    break;
            }

            g_error_free (error);
            return; /* FIXME: should ret? */
        }
    }
    else {
        g_file_copy (src, dest,
                     G_FILE_COPY_NOFOLLOW_SYMLINKS,
                     cancellable, NULL, NULL, &error);
        if (error) {
            /* handle the file already existing */
            if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
                gchar *old_path;
                gchar *new_path;
                GFile *new_dest;

                popup_overwrite_error_dialog ("%s", error->message);

                old_path = g_file_get_path (dest);
                new_path = g_strconcat (old_path, " (copy)", NULL);
                new_dest = g_file_new_for_path (new_path);
                g_free (old_path);
                g_free (new_path);

            }
            else {

            }

            g_error_free (error);
            return; /* FIXME: should ret? */
        }
    }
}

static void
copy_dir_recursive (GFile *src,
                    gchar *dest_dir_path,
                    GCancellable *cancellable,
                    gboolean overwrite_all,
                    gboolean overwrite_once)
{
    GFile *dest;
    gchar *src_basename;
    GFileEnumerator *direnum;
    GError *error = NULL;

    src_basename = g_file_get_basename (src);
    dest = g_file_new_build_filename (dest_dir_path,
                                      src_basename, NULL);
    g_free (src_basename);
    g_file_make_directory (dest, cancellable, &error);

    if (error) {
        /* TODO: popup */

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            if (overwrite_once) {
                /* retry */

                g_error_free (error);
                return;
            }
            if (overwrite_all) {
                /* retry */

                g_error_free (error);
                return;
            }
        }

        g_error_free (error);
        return;
    }

    direnum = g_file_enumerate_children (src, MYFM_FILE_QUERY_ATTRIBUTES,
                                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, &error);
    if (error) {
        /* TODO: POPUP */
        g_error_free (error);
        return;
    }

    while (direnum != NULL) {
        GFile *child;
        GFileInfo *child_info;
        GFileType child_type;
        GError *error2 = NULL;
        gboolean fail;
        gchar *new_dest_path;

        fail = !g_file_enumerator_iterate (direnum, &child_info,
                                           &child, cancellable,
                                           &error2);
        if (fail) {

            /* TODO: popup */
            g_error_free (error2);
            continue;
        }
        if (child_info == NULL) {
            break;
        }

        child_type = g_file_info_get_file_type (child_info);
        new_dest_path = g_file_get_path (dest);
        if (child_type == G_FILE_TYPE_DIRECTORY) {
            copy_dir_recursive (child, new_dest_path,
                                cancellable, overwrite_all,
                                FALSE);

        }
        else {
            // copy_file_single (child, new_dest_path,
            //                   cancellable, overwrite);

        }
    }
}

/* struct for passing gfiles and some
 * of their data  to our g_thread_func */
struct file_w_type {
    GFile *g_file;
    GFileType type;
    gboolean end;
};

static void
myfm_copy_operation_thread (GTask *task, gpointer src_object,
                            gpointer task_data,
                            GCancellable *cancellable)
{
    struct file_w_type *arr;
    struct file_w_type current;
    int i;

    arr = (struct file_w_type *) task_data;
    i = 0;
    current = arr[i];

    /* iterate until 'g_file' field is NULL (end of arr) */
    // while (!current.end) {
    //     i ++;
    //     current = arr[i];

    // }
    //
    //for (; !current.end; i++) {
    while (TRUE) {
        GFile *file;
        GFileType type;

        i ++;
        current = arr[i];
        file = current.g_file;
        type = current.type;

        if (current.end) {

            break;
        }
    }
}

void
myfm_copy_operation_cancel (void)
{

}

void
myfm_copy_operation_start_async (MyFMFile **src_files, gint n_files,
                                 MyFMFile *dest_dir, GtkWindow *active,
                                 GAsyncReadyCallback cb, gpointer data)
{
    GTask *cp;
    struct file_w_type *arr;
    GFile *dup_dest;

    arr = g_malloc (sizeof (struct file_w_type) * (n_files + 1));

    /* fill array with g_files and filetypes */
    for (int i = 0; i < n_files; i ++) {
        GFile *dup;
        GFileType type;
        struct file_w_type ft;

        /* NOTE: we duplicate to avoid sharing memory between threads */
        dup = g_file_dup (myfm_file_get_g_file (src_files[i]));
        type = myfm_file_get_filetype (src_files[i]);
        ft = (struct file_w_type) {dup, type, FALSE};
        arr[i] = ft;
    }

    dup_dest = g_file_dup (myfm_file_get_g_file (dest_dir));
    /* pass destination and terminate array with end=TRUE */
    arr[n_files] = (struct file_w_type) {dup_dest, G_FILE_TYPE_DIRECTORY, TRUE};

    win = active;
    cp_canceller = g_cancellable_new ();
    cp = g_task_new (NULL, cp_canceller, cb, data);
    g_task_set_task_data (cp, arr, NULL); /* FIXME: needs func to kill the data */
    // g_task_set_priority ()
    g_task_run_in_thread (cp, myfm_copy_operation_thread);

}

static gboolean
whatever_popup_func (gpointer data)
{
    // GtkDialog *p = new_error_dialog ("hello");

    // g_mutex_lock (&data->mut);
    // gtk_dialog_run (p);
   // gtk_widget_show (GTK_WIDGET (p));
    // gtk_widget_destroy (GTK_WIDGET (p));
    // g_mutex_unlock (&data->mut);
    // g_mutex_clear (&data->mut);

    return FALSE;
}

static void
whatever_thread (GTask *task, gpointer src_object,
                 gpointer task_data,
                 GCancellable *cancellable)
{
    // g_main_context_invoke (NULL,
    //                        whatever_popup_func,
    //                        NULL);

    // /* TODO: wait on cond until dialog destoyed */
    // puts ("????");

    // popup_test ("hello");
    popup_overwrite_error_dialog ("hello");

    puts ("????");


    g_task_return_pointer (task, NULL, NULL);
}

void
whatever (void)
{
    struct whatever_data *data;
    GTask *t;

    t = g_task_new (NULL, NULL, NULL, NULL);
    g_task_set_task_data (t, data, NULL);

    g_task_run_in_thread (t, whatever_thread);
}
