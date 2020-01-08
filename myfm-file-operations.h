/*
 * Created by f35 on 08.01.20.
*/

#ifndef __MYFM_FILE_OPERATIONS__H
#define __MYFM_FILE_OPERATIONS__H

#include <gtk/gtk.h>

#include "myfm-utils.h"

typedef void (*MyFMFileOpCallback)(gpointer);

void myfm_file_operations_delete_async (MyFMFile **src_files, gint n_files,
                                        GtkWindow *active, MyFMFileOpCallback cb,
                                        gpointer data);
void myfm_file_operations_move_async   (MyFMFile **src_files, gint n_files,
                                        MyFMFile *dest_dir, GtkWindow *active,
                                        MyFMFileOpCallback cb, gpointer data);
void myfm_file_operations_copy_async   (MyFMFile **src_files, gint n_files,
                                        MyFMFile *dest_dir, GtkWindow *active,
                                        MyFMFileOpCallback cb, gpointer data);

#endif /* __MYFM_FILE_OPERATIONS__H */
