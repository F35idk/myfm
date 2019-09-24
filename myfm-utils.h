//
// Created by f35 on 24.08.19.
//

#ifndef __MYFM_UTILS_H
#define __MYFM_UTILS_H

#include "myfm-file.h"

void files_to_store_async   (GFile *directory, GtkListStore *store);
gboolean free_file_in_store (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

#endif // __MYFM_UTILS_H
