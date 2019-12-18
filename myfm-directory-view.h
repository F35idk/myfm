/*
 * Created by f35 on 07.09.19.
*/

#ifndef __MYFM_DIRECTORY_VIEW_H
#define __MYFM_DIRECTORY_VIEW_H

#include <gtk/gtk.h>

#include "myfm-file.h"

/* gtk_tree_view subclass for displaying file directories. each column of the file manager
 * is its own myfm_directory_view. to create a new directory view and fill it with files simply
 * call myfm_directory_view_new () and myfm_directory_view_fill_store_async () */

#define MYFM_TYPE_DIRECTORY_VIEW (myfm_directory_view_get_type ())
G_DECLARE_FINAL_TYPE             (MyFMDirectoryView, myfm_directory_view, MYFM, DIRECTORY_VIEW, GtkTreeView)

typedef enum {
    MYFM_SORT_NEWLY_EDITED_FIRST,
    MYFM_SORT_NEWLY_EDITED_LAST,
    MYFM_SORT_NAME_A_TO_Z,
    MYFM_SORT_NAME_Z_TO_A,
    MYFM_SORT_LARGEST_FIRST,
    MYFM_SORT_LARGEST_LAST,
    MYFM_SORT_NONE,
} MyFMSortCriteria;

MyFMDirectoryView *myfm_directory_view_new                   (MyFMFile *directory);
MyFMFile          *myfm_directory_view_get_directory         (MyFMDirectoryView *self);
MyFMFile          *myfm_directory_view_get_file_from_path    (MyFMDirectoryView *self, GtkTreePath *path);
void              myfm_directory_view_set_show_hidden        (MyFMDirectoryView *self, gboolean show_hidden);
gboolean          myfm_directory_view_get_show_hidden        (MyFMDirectoryView *self);
void              myfm_directory_view_start_rename_file      (MyFMDirectoryView *self, MyFMFile *file, GtkTreePath *file_path);
void              myfm_directory_view_fill_store_async       (MyFMDirectoryView *self);
void              myfm_directory_view_refresh_files_async    (MyFMDirectoryView *self);
void              myfm_directory_view_set_file_sort_criteria (MyFMDirectoryView *self, MyFMSortCriteria criteria);

#endif /* __MYFM_DIRECTORY_VIEW_H */
