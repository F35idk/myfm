//
// Created by f35 on 07.09.19.
//

#ifndef __MYFM_TREE_VIEW_H
#define __MYFM_TREE_VIEW_H

#include <gtk/gtk.h>

#include "myfm-file.h"

/* gtk_tree_view subclass for displaying file directories. each column of the file manager
 * is its own myfm_directory_view. to create a new column representing a directory with
 * myfm_directory_view we do the following:
 *  * instantiate with myfm_directory_view_new ()
 *  * get the dir_view's list store and fill it with myfm_files using children_to_store ()
 *  * append the files to the dir_view using myfm_directory_view_append_file_column ()
 */

#define MYFM_DIRECTORY_VIEW_TYPE (myfm_directory_view_get_type ())
G_DECLARE_FINAL_TYPE (MyFMDirectoryView, myfm_directory_view, MYFM, DIRECTORY_VIEW, GtkTreeView)

MyFMDirectoryView *myfm_directory_view_new  (MyFMFile *directory);
void myfm_directory_view_fill_store_async   (MyFMDirectoryView *self);

#endif // __MYFM_TREE_VIEW_H
