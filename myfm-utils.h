/*
 * Created by f35 on 03.12.19.
*/

#ifndef __MYFM_UTILS_H
#define __MYFM_UTILS_H

#include <gtk/gtk.h>

#include "myfm-window.h"

/* FIXME: too vaguely named */
typedef void (*GFileForEachFunc) (GFile *, GFileInfo *,
                                  gboolean, GError *,
                                  gpointer);

void      myfm_utils_popup_error_dialog (GtkWindow *parent, char *format_msg, ...);
GtkWidget *myfm_utils_new_menu_item     (const gchar *label, guint keyval, GdkModifierType accel_mods);
void      myfm_utils_for_each_child     (GFile *dir, const gchar *attributes, GCancellable *cancellable,
                                         gint io_priority, GFileForEachFunc func, gpointer user_data);

#endif /* __MYFM_UTILS_H */
