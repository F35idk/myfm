//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>

#include "myfm-application.h"
#include "myfm-application-window.h"

struct _MyFMApplicationWindow
{
    GtkApplicationWindow parent;
};

G_DEFINE_TYPE (MyFMApplicationWindow, myfm_application_window, GTK_TYPE_APPLICATION_WINDOW);

static void myfm_application_window_init (MyFMApplicationWindow *win)
{
}

static void myfm_application_window_class_init (MyFMApplicationWindowClass *cls)
{
}

static void delete_treeview (GtkTreeView *treeview)
{
}

static void setup_treeview (void)
{
}

MyFMApplicationWindow *myfm_application_window_new (MyFMApplication *app)
{
    return g_object_new (MYFM_APPLICATION_WINDOW_TYPE, "application", app, NULL);
}

void myfm_application_window_open (MyFMApplicationWindow *win, GFile *file)
{
}