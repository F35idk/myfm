//
// Created by f35 on 15.08.19.
//

#ifndef __MYFM_WINDOW_H
#define __MYFM_WINDOW_H

#include <gtk/gtk.h>
#include "myfm-application.h"

#define MYFM_WINDOW_TYPE (myfm_window_get_type ())
G_DECLARE_FINAL_TYPE(MyFMWindow, myfm_window, MYFM, WINDOW, GtkApplicationWindow)

MyFMWindow *myfm_window_new (MyFMApplication *app);
void myfm_window_open_async (MyFMWindow *win, GFile *file);

#endif //__MYFM_APPLICATION_WINDOW_H