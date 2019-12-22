/*
 * Created by f35 on 20.12.19.
*/

#ifndef __MYFM_FILE_OPERATIONS_H
#define __MYFM_FILE_OPERATIONS_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-window.h"

void myfm_file_operations_copy_async (MyFMFile *file, gchar *dest_dir, MyFMWindow *active_win, gboolean overwrite_dest);
void myfm_file_operations_cancel_current (void);

#endif /* __MYFM_FILE_OPERATIONS_H */
