//
// Created by f35 on 24.08.19.
//

#ifndef __MYFM_DIRECTORY_VIEW_UTILS_H
#define __MYFM_DIRECTORY_VIEW_UTILS_H
#include "myfm-file.h"
#include "myfm-directory-view.h"

void files_to_store_async (MyFMDirectoryView *dirview);
gboolean clear_file_in_store (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

#endif // __MYFM_STORE_UTILS_H
