/*
 * Created by f35 on 31.12.19.
*/

#ifndef __MYFM_DELETE_OPERATION_H
#define __MYFM_DELETE_OPERATION_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-window.h"

typedef void (*MyFMDeleteCallback)(gpointer);

void myfm_delete_operation_cancel      (void);
void myfm_delete_operation_start_async (MyFMFile **src_files, gint n_files, GtkWindow *active,
                                        MyFMDeleteCallback cb, gpointer data);

#endif /* __MYFM_DELETE_OPERATION_H */