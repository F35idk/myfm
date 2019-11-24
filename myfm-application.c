/*
 * Created by f35 on 15.08.19.
*/

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-application.h"
#include "myfm-window.h"
#include "myfm-file.h"
#include "myfm-directory-view-utils.h"

struct _MyFMApplication {
    GtkApplication parent_instance;

    /* TODO: make configurable */
    GtkIconSize icon_size;

    /* convenience data structs for passing multiple arguments to
     * different g_actions throughout the application. can be accessed
     * through the application's get/set_action_struct functions */
    struct MyFMOpenFileArgs open_file_args;
    /* struct MyFMMoveFileArgs move_file_args;
     * ... */
};

G_DEFINE_TYPE (MyFMApplication, myfm_application, GTK_TYPE_APPLICATION)

/* copies contents of 'src' into the action struct corresponding to 'action_type' */
void myfm_application_set_action_args (MyFMApplication *self, gpointer src_struct, MyFMActionType action_type)
{
    switch (action_type) {
        case MYFM_OPEN_FILE_ACTION :
            if (!self->open_file_args.target_file) /* only set struct if it isn't already set */
                self->open_file_args = *(struct MyFMOpenFileArgs *) src_struct;
            break;
        case MYFM_RENAME_FILE_ACTION :
            /* fill in */
            break;
        case MYFM_MOVE_FILE_ACTION :
            /* fill in */
            break;
        case MYFM_COPY_FILE_ACTION :
            /* fill in */
            break;
        case MYFM_DELETE_FILE_ACTION :
            /* fill in */
            break;
        }
}

/* returns an action data struct (used for passing multiple arguments to window/app g_actions. a new
 * struct is not allocated once per call - the structs are fields/properties on the application class. */
gpointer myfm_application_get_action_args (MyFMApplication *self, MyFMActionType action_type)
{
    switch (action_type) {
        case MYFM_OPEN_FILE_ACTION :
            return &(self->open_file_args);
        case MYFM_RENAME_FILE_ACTION :
            /* fill in */
            break;
        case MYFM_MOVE_FILE_ACTION :
            /* fill in */
            break;
        case MYFM_COPY_FILE_ACTION :
            /* fill in */
            break;
        case MYFM_DELETE_FILE_ACTION :
            /* fill in */
            break;
        }
    return NULL;
}

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

    MYFM_APPLICATION (app)->open_file_args.target_file = home;
    MYFM_APPLICATION (app)->open_file_args.dirview_index = -1;
    g_action_group_activate_action (G_ACTION_GROUP (win), "open_file", NULL);

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

        MYFM_APPLICATION (app)->open_file_args.target_file = myfm_file;
        MYFM_APPLICATION (app)->open_file_args.dirview_index = -1;
        g_action_group_activate_action (G_ACTION_GROUP (win), "open_file", NULL);

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

    struct MyFMOpenFileArgs open_file_args = {NULL, 0};
    /* struct MyFMMoveFileArgs move_file_args = blabla;
     * ... */
    self->open_file_args = open_file_args;
    /* self->move_file_args = move_file_args;
     * ... */
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
