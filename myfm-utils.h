/*
 * Created by f35 on 03.12.19.
*/

#ifndef __MYFM_UTILS_H
#define __MYFM_UTILS_H

#include <gtk/gtk.h>

#include "myfm-window.h"

typedef void (*GFileForEachFunc) (GFile *, GFileInfo *,
                                  gboolean, GError *,
                                  gpointer);

/* currently there are only two types of dialogs */
typedef enum {
    MYFM_DIALOG_TYPE_MERGE_CONFLICT,
    MYFM_DIALOG_TYPE_ERROR,
} MyFMDialogType;

/* user responses to our popup dialogs */
typedef enum {
    MYFM_DIALOG_RESPONSE_NONE,
    MYFM_DIALOG_RESPONSE_CANCEL,
    MYFM_DIALOG_RESPONSE_MAKE_COPY_ONCE,
    MYFM_DIALOG_RESPONSE_MAKE_COPY_ALL,
    MYFM_DIALOG_RESPONSE_SKIP_ERROR_ONCE,
    /* NOTE: 'skip_all_errors' does not also skip all merges */
    MYFM_DIALOG_RESPONSE_SKIP_ALL_ERRORS,
    MYFM_DIALOG_RESPONSE_SKIP_MERGE_ONCE,
    MYFM_DIALOG_RESPONSE_SKIP_ALL_MERGES,
    MYFM_DIALOG_RESPONSE_MERGE_ONCE,
    MYFM_DIALOG_RESPONSE_MERGE_ALL,
} MyFMDialogResponse;

gint      myfm_utils_run_error_dialog   (GtkWindow *parent, gchar *format_msg, ...);
gint      myfm_utils_run_dialog_thread  (MyFMDialogType type, GtkWindow *active, GCancellable *cancellable,
                                         gchar *title, gchar *primary_msg, gchar *format_msg, ...);
GtkWidget *myfm_utils_new_menu_item     (const gchar *label, guint keyval, GdkModifierType accel_mods);
void      myfm_utils_for_each_child     (GFile *dir, const gchar *attributes, GCancellable *cancellable,
                                         gint io_priority, GFileForEachFunc func, gpointer user_data);

#endif /* __MYFM_UTILS_H */
