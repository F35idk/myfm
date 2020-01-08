/*
 * Created by f35 on 04.01.20.
*/

#ifndef __MYFM_FILE_OPERATIONS_MOVE_H
#define __MYFM_FILE_OPERATIONS_MOVE_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-window.h"

typedef void (*MyFMMoveCallback)(gpointer);

void myfm_move_operation_start_async (MyFMFile **src_files, gint n_files, MyFMFile *dest_dir,
                                      GtkWindow *active, MyFMMoveCallback cb, gpointer data);

#endif /* __MYFM_FILE_OPERATIONS_MOVE_H */
