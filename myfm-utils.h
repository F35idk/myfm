/*
 * Created by f35 on 03.13.19.
*/

#ifndef __MYFM_UTILS_H
#define __MYFM_UTILS_H

#include <gtk/gtk.h>

#include "myfm-window.h"

void       myfm_utils_popup_error_dialog (MyFMWindow *parent, char *format_msg, ...);
GtkWidget *myfm_utils_new_menu_item      (const gchar *label, guint keyval, GdkModifierType accel_mods);

#endif /* __MYFM_UTILS_H */
