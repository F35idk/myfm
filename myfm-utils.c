/*
 * Created by f35 on 03.13.19.
*/

#include <stdarg.h>
#include <gtk/gtk.h>

#include "myfm-window.h"
#include "myfm-utils.h"
#define G_LOG_DOMAIN "myfm-utils"

void
myfm_utils_popup_error_dialog (MyFMWindow *parent, char *format_msg, ...)
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

    error_dialog = gtk_message_dialog_new (GTK_WINDOW (parent), GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                           "%s", new);
    gtk_window_set_title (GTK_WINDOW (error_dialog), "Error");
    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (error_dialog);
}

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

struct callback_data {
    ForEachFunc user_callback;
    gpointer user_data;
    GFile *dir;
    int io_priority;
    GCancellable *cancellable;
};

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
        free (cb_data);
        return;
    }
    else if (directory_list == NULL) {
        /* done listing, nothing left to add to store */
        g_object_unref (file_enumerator);
        cb_data->user_callback (cb_data->dir, NULL,
                                TRUE, NULL,
                                cb_data->user_data);
        free (cb_data);
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
        free (cb_data);
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
                           ForEachFunc func, gpointer user_data)
{
    struct callback_data *cb_data = malloc (sizeof (struct callback_data));
    cb_data->user_callback = func;
    // cb_data->dir = dir;
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
