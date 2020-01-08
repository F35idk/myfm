/*
 * Created by f35 on 31.12.19.
*/

#ifndef __MYFM_FILE_OPERATIONS_DELETE_H
#define __MYFM_FILE_OPERATIONS_DELETE_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-window.h"

typedef void (*MyFMDeleteCallback)(gpointer);

void myfm_delete_operation_start_async   (MyFMFile **src_files, gint n_files,
                                          GtkWindow *active, MyFMDeleteCallback cb,
                                          gpointer data);
void myfm_delete_operation_delete_single (GFile *file, GtkWindow *active,
                                          GCancellable *cancellable, GError **error);

#endif /* __MYFM_FILE_OPERATIONS_DELETE_H */
