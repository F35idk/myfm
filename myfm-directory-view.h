//
// Created by f35 on 07.09.19.
//

#ifndef __MYFM_TREE_VIEW_H
#define __MYFM_TREE_VIEW_H

#include <gtk/gtk.h>

#define MYFM_DIRECTORY_VIEW_TYPE (myfm_directory_view_get_type ())
G_DECLARE_FINAL_TYPE (MyFMDirectoryView, myfm_directory_view, MYFM, DIRECTORY_VIEW, GtkTreeView)

MyFMDirectoryView *myfm_directory_view_new (void);
void myfm_directory_view_append_file_column (MyFMDirectoryView *self);

#endif // __MYFM_TREE_VIEW_H
