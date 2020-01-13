/*
 * Created by f35 on 24.11.19.
*/

#ifndef __MYFM_FILE_MENU_H
#define __MYFM_FILE_MENU_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-directory-view.h"

/* context menu that appears when right-clicking myfm-files */

#define MYFM_TYPE_FILE_MENU    (myfm_file_menu_get_type ())
G_DECLARE_FINAL_TYPE           (MyFMFileMenu, myfm_file_menu, MYFM, FILE_MENU, GtkMenu)

GtkWidget *myfm_file_menu_new (MyFMDirectoryView *dirview, MyFMFile *file);

#endif /* __MYFM_FILE_MENU_H */
