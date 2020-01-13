/*
 * Created by f35 on 03.13.19.
*/

#include <stdarg.h>
#include <gtk/gtk.h>

#include "widgets/myfm-window.h"
#include "myfm-utils.h"
#define G_LOG_DOMAIN "myfm-utils"

GtkWidget *
myfm_utils_new_menu_item (const gchar *label, guint keyval,
                          GdkModifierType accel_mods)
{
    GtkWidget *menu_item;
    GtkWidget *child;

    menu_item = gtk_menu_item_new_with_label (label);
    child = gtk_bin_get_child (GTK_BIN (menu_item));

    if (keyval || accel_mods)
        gtk_accel_label_set_accel (GTK_ACCEL_LABEL (child),
                                   keyval, accel_mods);

    return menu_item;
}

/* appends " (copy)" to end
 * of file's path. used by file
 * operations to avoid merges  */
GFile *
myfm_utils_new_renamed_g_file (GFile *old)
{
    gchar *old_path;
    gchar *new_path;
    GFile *new;

    old_path = g_file_get_path (old);
    new_path = g_strconcat (old_path, " (copy)", NULL);
    new = g_file_new_for_path (new_path);
    g_free (old_path);
    g_free (new_path);

    return new;
}

struct callback_data {
    GFileForEachFunc user_callback;
    gpointer user_data;
    GFile *dir;
    int io_priority;
    GCancellable *cancellable;
};

/* NOTE: get_children and its callbacks are dead code atm,
 * remove or switch to using it in myfm-directory-view */

static const gint n_files = 32;

static void
myfm_utils_next_files_callback (GObject *file_enumerator, GAsyncResult *result,
                                gpointer data)
{
    GError *error = NULL;
    GList *directory_list;
    struct callback_data *cb_data;

    cb_data = (struct callback_data *) data;
    directory_list = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (file_enumerator),
                                                          result, &error);
    if (error) {
        /* if IO was cancelled due to the directory view
         * being destroyed, if the file no longer exists, etc. */
        g_critical ("unable to add files to list: '%s'",
                    error->message);
        g_object_unref (file_enumerator);
        cb_data->user_callback (cb_data->dir, NULL,
                                FALSE, error,
                                cb_data->user_data);
        g_free (cb_data);
        return;
    }
    else if (directory_list == NULL) {
        /* done listing, nothing left to add to store */
        g_object_unref (file_enumerator);
        cb_data->user_callback (cb_data->dir, NULL,
                                TRUE, NULL,
                                cb_data->user_data);
        g_free (cb_data);
        return;
    }
    else {
        /* enumerator returned successfully */
        GFileInfo *child_info;
        GList *current_node = directory_list;

        while (current_node) {
            child_info = current_node->data;
            cb_data->user_callback (cb_data->dir, child_info,
                                    FALSE, NULL,
                                    cb_data->user_data);
            current_node = current_node->next;
        }
        /* continue calling next_files and next_files_callback recursively */
        g_file_enumerator_next_files_async (G_FILE_ENUMERATOR (file_enumerator),
                                            n_files, cb_data->io_priority,
                                            cb_data->cancellable,
                                            myfm_utils_next_files_callback,
                                            cb_data);
    }
    g_list_free (directory_list);
}

static void
myfm_utils_enum_finished_callback (GObject *dir, GAsyncResult *result,
                                   gpointer data)
{
    GFileEnumerator *file_enumerator;
    GError *error = NULL;
    struct callback_data *cb_data;


    cb_data = (struct callback_data *) data;
    file_enumerator = g_file_enumerate_children_finish (G_FILE (dir),
                                                        result, &error);
    if (error) {
        g_critical ("error in enum_finished_callback: '%s'",
                    error->message);

        /* execute user provided callback with error */
        cb_data->user_callback (cb_data->dir, NULL,
                                FALSE, error,
                                cb_data->user_data);
        g_free (cb_data);
        return;
    }
    else {
        g_file_enumerator_next_files_async (file_enumerator, n_files,
                                            cb_data->io_priority,
                                            cb_data->cancellable,
                                            myfm_utils_next_files_callback,
                                            cb_data);
    }
}

void
myfm_utils_for_each_child (GFile *dir, const gchar *attributes,
                           GCancellable *cancellable, gint io_priority,
                           GFileForEachFunc func, gpointer user_data)
{
    struct callback_data *cb_data = g_malloc (sizeof (struct callback_data));
    cb_data->user_callback = func;
    cb_data->user_data = user_data;
    cb_data->io_priority = io_priority;
    cb_data->dir = dir;
    cb_data->cancellable = cancellable;

    g_file_enumerate_children_async (dir, attributes,
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     io_priority, cancellable,
                                     myfm_utils_enum_finished_callback,
                                     cb_data);
}

gint
myfm_utils_run_error_dialog (GtkWindow *parent, gchar *format_msg, ...)
{
    va_list va;
    va_list va_cp;
    int len;
    GtkWidget *error_dialog;

    va_start (va, format_msg);
    va_copy (va_cp, va);
    len = vsnprintf (NULL, 0, format_msg, va_cp);
    char new[len+1]; /* NOTE: VLA, ehhh */
    new[len+1] = '\0';
    vsprintf (new, format_msg, va);
    va_end (va);

    error_dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                           "%s", new);
    gtk_window_set_title (GTK_WINDOW (error_dialog), "Error");
    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (error_dialog);

    /* only possible response from this dialog */
    return MYFM_DIALOG_RESPONSE_NONE;
}

struct dialog_data {
    gchar *primary_msg;
    gchar *secondary_msg;
    gchar *title;
    MyFMDialogType type;
    MyFMDialogResponse response;
    GCancellable *cancellable;
    GtkWindow *win;
    GtkWidget *check;
    GtkButtonsType buttons;
    GCond cond;
    GMutex mutex;
};

static void
on_error_dialog_response (GtkDialog *dialog, gint response_id,
                          gpointer _data)
{
    struct dialog_data *data;
    GtkToggleButton *check;
    gboolean apply_to_all;

    data = _data;
    g_mutex_lock (&data->mutex);

    check = GTK_TOGGLE_BUTTON (data->check);
    if (data->type != MYFM_DIALOG_TYPE_UNSKIPPABLE_ERR)
        apply_to_all = gtk_toggle_button_get_active (check);
    else
        apply_to_all = FALSE;

    switch (response_id) {
        default :
            if (data->type == MYFM_DIALOG_TYPE_MERGE_CONFLICT)
                data->response = MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE;
            else
                data->response = MYFM_DIALOG_RESPONSE_SKIP_ERROR_ONCE;
            break;
        case 0 : /* 'Cancel' */
            data->response = MYFM_DIALOG_RESPONSE_CANCEL;
            g_cancellable_cancel (data->cancellable);
            break;
        case 1 : /* 'Make Copy' */
            if (apply_to_all)
                data->response = MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL;
            else
                data->response = MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE;
            break;
        case 2 : /* 'Skip' */
            if (data->type == MYFM_DIALOG_TYPE_MERGE_CONFLICT) {
                if (apply_to_all)
                    data->response = MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES;
                else
                    data->response = MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE;
            }
            else {
                if (apply_to_all)
                    data->response = MYFM_DIALOG_RESPONSE_SKIP_ALL_ERRORS;
                else
                    data->response = MYFM_DIALOG_RESPONSE_SKIP_ERROR_ONCE;
            }
            break;
        case 3 : /* 'Replace' */
            if (apply_to_all)
                data->response = MYFM_DIALOG_RESPONSE_MERGE_ALL;
            else
                data->response = MYFM_DIALOG_RESPONSE_MERGE_ONCE;
            break;
        case 4 : /* 'OK' */
            data->response = MYFM_DIALOG_RESPONSE_OK;
            break;
    }

    g_cond_signal (&data->cond);
    g_mutex_unlock (&data->mutex);
}

static gboolean
run_dialog_main_ctx (gpointer _data)
{
    GtkWidget *error_dialog;
    struct dialog_data *data;
    GtkWidget *msg_area;
    GtkMessageType msg_type;
    GtkWidget *check;
    gchar *check_label;
    gchar *primary;
    gchar *title;

    data = _data;

    if (data->type == MYFM_DIALOG_TYPE_MERGE_CONFLICT) {
        msg_type = GTK_MESSAGE_QUESTION;
        /* defaults for nullable msgs */
        if (data->primary_msg == NULL)
            primary = "File already exists. Replace it?";
        else
            primary = data->primary_msg;
        if (data->title == NULL)
            title = "File Conflict";
        else
            title = data->title;
    }
    /* FIXME: compact this bloated code */
    else {
        msg_type = GTK_MESSAGE_ERROR;
        /* defaults for nullable msgs */
        if (data->primary_msg == NULL)
            primary = "There was an error.";
        else
            primary = data->primary_msg;
        if (data->title == NULL)
            title = "Error";
        else
            title = data->title;
    }

    error_dialog = gtk_message_dialog_new (data->win, GTK_DIALOG_MODAL,
                                           msg_type, GTK_BUTTONS_NONE,
                                           "%s", primary);
    g_object_set (G_OBJECT (error_dialog), "secondary-text",
                  data->secondary_msg, NULL);
    gtk_window_set_title (GTK_WINDOW (error_dialog), title);

    if (data->type != MYFM_DIALOG_TYPE_UNSKIPPABLE_ERR) {
        if (data->type == MYFM_DIALOG_TYPE_MERGE_CONFLICT) {
            check_label = "Apply this action to all file conflicts";
            gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "Cancel", 0,
                                    "Make Copy", 1, "Skip", 2, "Replace", 3, NULL);
        }
        else if (data->type == MYFM_DIALOG_TYPE_SKIPPABLE_ERR) {
            check_label = "Apply this action to all errors";
            gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "Cancel", 0, "Skip",
                                    2, NULL);
        }

        msg_area = gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (error_dialog));
        check = gtk_check_button_new_with_label (check_label);
        gtk_box_pack_end (GTK_BOX (msg_area), check, FALSE, TRUE, 0);
        gtk_widget_show (check);

        data->check = check;
        g_signal_connect (GTK_DIALOG (error_dialog), "response",
                          G_CALLBACK (on_error_dialog_response), data);
    }
    else {
        gtk_dialog_add_buttons (GTK_DIALOG (error_dialog), "OK", 4, NULL);
    }

    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (GTK_WIDGET (error_dialog));

    /* if error is unskippable and fatal, cancel no matter the response */
    if (data->type == MYFM_DIALOG_TYPE_UNSKIPPABLE_ERR)
        g_cancellable_cancel (data->cancellable);

    return FALSE;
}


/* for use outside of the main context. pops up a dialog
 * of the type specified by 'type'. the thread this is
 * run in will sleep until the user responds. 'cancellable'
 * will be cancelled if the user presses the 'Cancel' button
 * in the popup */
/* NOTE: this frees the passed strings, so make sure to
 * strdup them */
gint
myfm_utils_run_dialog_thread (MyFMDialogType type,
                              GtkWindow *active,
                              GCancellable *cancellable,
                              gchar *title,       /* nullable */
                              gchar *primary_msg, /* nullable */
                              gchar *secondary_msg)
{
    struct dialog_data *data;
    MyFMDialogResponse response;

    data = g_malloc0 (sizeof (struct dialog_data));
    g_mutex_init (&data->mutex);
    g_cond_init (&data->cond);

    data->type = type;
    data->win = active;
    data->response = MYFM_DIALOG_RESPONSE_NONE;
    data->cancellable = cancellable;

    if (title)
        data->title = title;
    if (primary_msg)
        data->primary_msg = primary_msg;

    data->secondary_msg = secondary_msg;

    g_mutex_lock (&data->mutex);
    g_main_context_invoke (NULL, run_dialog_main_ctx, data);

    /* wait on response from error dialog in main thread */
    while (data->response == MYFM_DIALOG_RESPONSE_NONE)
        g_cond_wait (&data->cond, &data->mutex);

    response = data->response;
    g_mutex_unlock (&data->mutex);
    g_mutex_clear (&data->mutex);
    g_cond_clear (&data->cond);

    if (title)
        g_free (data->title);
    if (primary_msg)
        g_free (data->primary_msg);

    g_free (data->secondary_msg);
    g_free (data);

    return response;
}

/* convenience/wrapper varargs function for merge conflicts */
gint
myfm_utils_run_merge_conflict_dialog_thread (GtkWindow *active,
                                             GCancellable *cancellable,
                                             const gchar *format_msg,
                                             va_list va)
{
    gchar *secondary_msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    return myfm_utils_run_dialog_thread (MYFM_DIALOG_TYPE_MERGE_CONFLICT,
                                         active, cancellable, NULL,
                                         NULL, secondary_msg);
}

/* convenience/wrapper varargs function for skippable errors */
gint
myfm_utils_run_skippable_err_dialog_thread (GtkWindow *active,
                                            GCancellable *cancellable,
                                            const gchar *title,
                                            const gchar *primary_msg,
                                            const gchar *format_msg,
                                            va_list va)

{
    gchar *secondary_msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    return myfm_utils_run_dialog_thread (MYFM_DIALOG_TYPE_SKIPPABLE_ERR,
                                         active, cancellable,
                                         g_strdup (title),
                                         g_strdup (primary_msg),
                                         secondary_msg);
}

/* for unskippable, fatal errors. will cancel
 * 'cancellable' no matter the user response */
gint
myfm_utils_run_unskippable_err_dialog_thread (GtkWindow *active,
                                              GCancellable *cancellable,
                                              const gchar *title,
                                              const gchar *primary_msg,
                                              const gchar *format_msg,
                                              va_list va)
{
    MyFMDialogType type;
    gchar *secondary_msg;

    type = MYFM_DIALOG_TYPE_UNSKIPPABLE_ERR;
    secondary_msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    return myfm_utils_run_dialog_thread (type, active,
                                         cancellable,
                                         g_strdup (title),
                                         g_strdup (primary_msg),
                                         secondary_msg);
}
