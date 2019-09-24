//
// Created by f35 on 15.08.19.
//

#ifndef __MYFM_WINDOW_H
#define __MYFM_WINDOW_H

#include <gtk/gtk.h>

#include "myfm-application.h"
#include "myfm-file.h"
#include "myfm-directory-view.h"

#define MYFM_WINDOW_TYPE (myfm_window_get_type ())
G_DECLARE_FINAL_TYPE (MyFMWindow, myfm_window, MYFM, WINDOW, GtkApplicationWindow)

MyFMWindow *myfm_window_new               (MyFMApplication *app);
void myfm_window_open_file_async          (MyFMWindow *win, MyFMFile *file, gint dirview_index);
void myfm_window_open_g_file_async        (MyFMWindow *self, GFile *file);
GtkBox *myfm_window_get_box               (MyFMWindow *self);
gint myfm_window_get_directory_view_index (MyFMWindow *self, MyFMDirectoryView *dirview);

#endif //__MYFM_APPLICATION_WINDOW_H