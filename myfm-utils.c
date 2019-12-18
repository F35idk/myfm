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
