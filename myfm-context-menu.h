/*
 * Created by f35 on 24.11.19.
*/

#ifndef __MYFM_CONTEXT_MENU_H
#define __MYFM_CONTEXT_MENU_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-directory-view.h"

/* context-dependent popup menu derived from gtk_menu. displays different info based on
 * what was right clicked (either a specific file or just blank space in the directory view) */

#define MYFM_TYPE_CONTEXT_MENU (myfm_context_menu_get_type ())
G_DECLARE_FINAL_TYPE           (MyFMContextMenu, myfm_context_menu, MYFM, CONTEXT_MENU, GtkMenu)

MyFMContextMenu *myfm_context_menu_new_for_file           (MyFMDirectoryView *dirview, MyFMFile *file,
                                                           GtkTreePath *file_path);
MyFMContextMenu *myfm_context_menu_new_for_directory_view (MyFMDirectoryView *dirview);

#endif /* __MYFM_CONTEXT_MENU_H */
