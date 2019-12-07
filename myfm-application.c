/*
 * Created by f35 on 15.08.19.
*/

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-application.h"
#include "myfm-window.h"
#include "myfm-file.h"
#define G_LOG_DOMAIN "myfm-application"

struct _MyFMApplication {
    GtkApplication parent_instance;
    GtkIconSize icon_size; /* TODO: make configurable */
};

G_DEFINE_TYPE (MyFMApplication, myfm_application, GTK_TYPE_APPLICATION)

GtkIconSize myfm_application_get_icon_size (MyFMApplication *self)
{
    return self->icon_size;
}

/* application launched with no args */
static void myfm_application_activate (GApplication *app)
{
    MyFMWindow *win;
    MyFMFile *home;

    /* should default to home directory, currently that's just my home though */
    home = myfm_file_from_path ("/home/f35/");
    win = myfm_window_new (MYFM_APPLICATION (app));
    myfm_window_open_file_async (win, home, -1);

    myfm_file_unref (home);
    gtk_window_present (GTK_WINDOW (win));
}

/* application launched with args */
static void myfm_application_open (GApplication *app, GFile **files,
                                   gint n_files, const gchar *hint)
{
    for (int i = 0; i < n_files; i++) {
        MyFMWindow *win = myfm_window_new (MYFM_APPLICATION (app));
        /* normally we would ref the g_files here, since they're freed when this function exits.
         * but myfm_file_from_g_file () takes care of that for us, so there's no need */
        MyFMFile *myfm_file = myfm_file_from_g_file (files[i]);
        myfm_window_open_file_async (win, myfm_file, -1);
        myfm_file_unref (myfm_file);
        gtk_window_present (GTK_WINDOW (win));
    }
}

static void myfm_application_startup (GApplication *app)
{
    /* chaining up */
    G_APPLICATION_CLASS (myfm_application_parent_class)->startup (app);

    g_set_application_name ("myfm");
}

static void myfm_application_finalize (GObject *object)
{
    /* chaining up */
    G_OBJECT_CLASS (myfm_application_parent_class)->finalize (object);
}

static void myfm_application_init (MyFMApplication *self)
{
    /* FIXME: turns out this is deprecated. but it saves us a ton of slowness and complexity, so.. */
    /* TODO: instead of doing this, implement an entire gtk_icon_theme when this is needed in the future */
    self->icon_size = gtk_icon_size_register ("default_icon_size", 20, 20);
}

static void myfm_application_class_init (MyFMApplicationClass *cls)
{
    /* replacing overridden functions in our class' parent instances */
    GApplicationClass *app_cls = G_APPLICATION_CLASS (cls);
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);

    app_cls->startup = myfm_application_startup;
    app_cls->activate = myfm_application_activate;
    app_cls->open = myfm_application_open;

    object_cls->finalize = myfm_application_finalize;
}

MyFMApplication *myfm_application_new (void)
{
    return g_object_new (MYFM_TYPE_APPLICATION, "application-id", "com.github.F35idk.myfm",
                        "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}
