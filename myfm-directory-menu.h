/*
 * Created by f35 on 24.11.19.
*/

#ifndef __MYFM_DIRECTORY_MENU_H
#define __MYFM_DIRECTORY_MENU_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-directory-view.h"

/* context menu that appears when right-clicking myfm-directory-views */
/* TODO: implement this */

#define MYFM_TYPE_DIRECTORY_MENU (myfm_directory_menu_get_type ())
G_DECLARE_FINAL_TYPE           (MyFMDirectoryMenu, myfm_directory_menu, MYFM, DIRECTORY_MENU, GtkMenu)

GtkWidget *myfm_directory_menu_new (MyFMDirectoryView *dirview);

#endif /* __MYFM_DIRECTORY_MENU_H */
