/*
 * Created by f35 on 20.12.19.
*/

#ifndef __MYFM_FILE_OPERATIONS_H
#define __MYFM_FILE_OPERATIONS_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-window.h"

void myfm_file_operations_cancel_current (void);
void myfm_file_operations_copy_async     (MyFMFile *file, gchar *dest_dir_path,
                                          MyFMWindow *active_win, gboolean overwrite_dest,
                                          void (*CopyFinishedCallback)(gpointer, gpointer, gpointer),
                                          gpointer data1, gpointer data2, gpointer data3);

#endif /* __MYFM_FILE_OPERATIONS_H */
