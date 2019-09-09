//
// Created by f35 on 07.09.19.
//

#ifndef __MYFM_TREE_VIEW_H
#define __MYFM_TREE_VIEW_H

#include <gtk/gtk.h>

#define MYFM_TREE_VIEW_TYPE (myfm_tree_view_get_type ())
G_DECLARE_FINAL_TYPE (MyFMTreeView, myfm_tree_view, MYFM, TREE_VIEW, GtkTreeView)

MyFMTreeView *myfm_tree_view_new (void);
MyFMTreeView *myfm_tree_view_new_with_model (GtkTreeModel *model);
void myfm_tree_view_append_file_column (MyFMTreeView *self);

#endif // __MYFM_TREE_VIEW_H
