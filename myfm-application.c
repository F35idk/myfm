//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-application.h"
#include "myfm-application-window.h"
#include "myfm-file.h"
#include "myfm-utils.h"
#include "myfm-signals.h"

struct _MyFMApplication
{
    GtkApplication parent_instance;
};

G_DEFINE_TYPE (MyFMApplication, myfm_application, GTK_TYPE_APPLICATION);

/* application launched with no args */
static void myfm_application_activate (GApplication *app)
{
    MyFMApplicationWindow *win;

    /* should default to home directory, currently that's just my home though */
    win = myfm_application_window_new (MYFM_APPLICATION (app));
    myfm_application_window_open (win, g_file_new_for_path ("/home/f35/"));

    gtk_window_present (GTK_WINDOW (win));
}

/* application launched with args */
static void myfm_application_open (GApplication *app, GFile **files,
                                   gint n_files, const gchar *hint)
{
    GList *windows;
    MyFMApplicationWindow *win;

    windows = gtk_application_get_windows (GTK_APPLICATION (app));
    if (windows)
        win = MYFM_APPLICATION_WINDOW (windows->data);
    else
        win = myfm_application_window_new (MYFM_APPLICATION (app));

    for (int i = 0; i < n_files; i++)
        myfm_application_window_open (win, files[i]);

    gtk_window_present (GTK_WINDOW (win));
}

static void myfm_application_startup (GApplication *app)
{
    /* chaining up */
    G_APPLICATION_CLASS (myfm_application_parent_class)->startup (app);
}

static void myfm_application_finalize (GObject *object)
{
    /* chaining up */
    G_OBJECT_CLASS (myfm_application_parent_class)->finalize (object);
}

static void myfm_application_init (MyFMApplication *self)
{
}

static void myfm_application_class_init (MyFMApplicationClass *cls)
{
    /* replacing overridden functions in parent class objects */
    GApplicationClass *app_cls = G_APPLICATION_CLASS (cls);
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);

    app_cls->activate = myfm_application_activate;
    app_cls->open = myfm_application_open;

    object_cls->finalize = myfm_application_finalize;
}

/* TODO: should only contain g_object_new? */
MyFMApplication *myfm_application_new (void)
{
    MyFMApplication *app;

    g_set_application_name ("myfm");
    app = g_object_new (MYFM_APPLICATION_TYPE, "application-id", "com.github.F35idk.myfm",
                        "flags", G_APPLICATION_HANDLES_OPEN, NULL);

    return app;
}