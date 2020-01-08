/*
 * Created by f35 on 26.12.19.
*/

#ifndef __MYFM_FILE_OPERATIONS_COPY_H
#define __MYFM_FILE_OPERATIONS_COPY_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-window.h"

typedef void (*MyFMCopyCallback)(gpointer);

void myfm_copy_operation_cancel      (void);
void myfm_copy_operation_start_async (MyFMFile **src_files, gint n_files, MyFMFile *dest_dir,
                                      GtkWindow *active, MyFMCopyCallback cb, gpointer data);

#endif /* __MYFM_FILE_OPERATIONS_COPY_H */
