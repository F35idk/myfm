//
// Created by f35 on 15.08.19.
//

#ifndef __MYFM_APPLICATION_WINDOW_H
#define __MYFM_APPLICATION_WINDOW_H

#include <gtk/gtk.h>
#include "myfm-application.h"

#define MYFM_APPLICATION_WINDOW_TYPE (myfm_application_window_get_type ())
G_DECLARE_FINAL_TYPE(MyFMApplicationWindow, myfm_application_window, MYFM, APPLICATION_WINDOW, GtkApplicationWindow)

MyFMApplicationWindow *myfm_application_window_new (MyFMApplication *app);
void myfm_application_window_open (MyFMApplicationWindow *win, GFile *file);
void myfm_application_window_new_tab (MyFMApplicationWindow *win);

#endif //__MYFM_APPLICATION_WINDOW_H