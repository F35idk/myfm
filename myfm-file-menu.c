/*
 * Created by f35 on 24.11.19.
*/

#include <gtk/gtk.h>

#include "myfm-window.h"
#include "myfm-directory-view.h"
#include "myfm-utils.h"
#include "myfm-file-operations.h"
#include "myfm-file-menu.h"
#define G_LOG_DOMAIN "myfm-file-menu"

struct _MyFMFileMenu {
    GtkMenu parent_instance;
    MyFMDirectoryView *dirview;
    MyFMFile *file;
};

G_DEFINE_TYPE (MyFMFileMenu, myfm_file_menu, GTK_TYPE_MENU)

/* gets the window file-menu was opened at */
static MyFMWindow *
myfm_file_menu_get_window (MyFMFileMenu *self)
{
    return MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self->dirview)));
}

static void
myfm_file_menu_on_item_activate (GtkMenuItem *item,
                                 gpointer myfm_file_menu)
{
    MyFMWindow *window;
    MyFMFileMenu *self;
    const gchar *label;

    self = MYFM_FILE_MENU (myfm_file_menu);
    label = gtk_menu_item_get_label (item);

    if (!strcmp (label, "Open")) {
        gint dirview_index;

        window = myfm_file_menu_get_window (self);
        dirview_index = myfm_window_get_directory_view_index (window,
                                                              self->dirview);
        myfm_window_open_file_async (window, self->file,
                                     dirview_index);
    }
    else if (!strcmp (label, "Rename...")) {
        /* force start editing cell in treeview */
        myfm_directory_view_start_rename_selected (self->dirview,
                                                   self->file);
        g_debug ("rename");
    }
    else if (!strcmp (label, "Cut")) {
        GtkApplication *app;
        MyFMClipBoard *cboard;

        app = gtk_window_get_application (GTK_WINDOW (myfm_file_menu_get_window (self)));
        cboard = myfm_application_get_file_clipboard (MYFM_APPLICATION (app));

        myfm_clipboard_add_to_cut (cboard, &self->file, 1, TRUE);
    }
    else if (!strcmp (label, "Copy")) {
        GtkApplication *app;
        MyFMClipBoard *cboard;

        app = gtk_window_get_application (GTK_WINDOW (myfm_file_menu_get_window (self)));
        cboard = myfm_application_get_file_clipboard (MYFM_APPLICATION (app));

        myfm_clipboard_add_to_copied (cboard, &self->file, 1, TRUE);
    }
    else if (!strcmp (label, "Delete")) {
        myfm_file_operations_delete_async (&self->file, 1,
                                           GTK_WINDOW (myfm_file_menu_get_window (self)),
                                           NULL, NULL);
    }
    else if (!strcmp (label, "Open in New Window")) {
        /* FIXME: implement */
    }
}

static void
myfm_file_menu_on_open_with_app (GtkMenuItem *item,
                                 gpointer myfm_file_menu)
{
    MyFMFileMenu *self;
    GAppInfo *app_info;
    GList *g_file_list;
    GError *error = NULL;

    self = MYFM_FILE_MENU (myfm_file_menu);
    app_info = g_object_get_data (G_OBJECT (item), "app_info");
    g_file_list = g_list_append (NULL, myfm_file_get_g_file (self->file));

    g_app_info_set_as_last_used_for_type (app_info,
                                          myfm_file_get_content_type (self->file),
                                          NULL);
    /* NOTE: this makes valgrind complain */
    g_app_info_launch (app_info, g_file_list, NULL, &error);

    if (error) {
        myfm_utils_run_error_dialog (GTK_WINDOW (myfm_file_menu_get_window (self)),
                                     "error in myfm_window when opening file(s)"
                                     "with '%s': %s \n",
                                     g_app_info_get_display_name (app_info),
                                     error->message);
        g_critical ("error in myfm_file_menu when opening file(s) with '%s': %s \n",
                    g_app_info_get_display_name (app_info), error->message);
        g_error_free (error);
    }

    g_list_free (g_file_list);
}

/* handler for gtk_app_chooser_widget's "application-activated" signal.
 * opens the myfm_file file_menu points at using 'app_info'. */
static void
on_app_chooser_item_activate (GtkAppChooserWidget *chooser_widget,
                              GAppInfo *app_info, gpointer chooser_dialog)
{
    GFile *g_file;
    GList *g_file_list;
    GError *error = NULL;

    g_object_get (G_OBJECT (chooser_dialog), "gfile", &g_file, NULL);
    g_file_list = g_list_append (NULL, g_file);

    g_app_info_launch (app_info, g_file_list, NULL, &error);
    /* this unref shouldn't be needed, but doesn't hurt. clears a
     * "definitely lost" in valgrind (false flag, most likey) */
    g_object_unref (app_info);
    app_info = NULL;

    if (error) {
        myfm_utils_run_error_dialog (GTK_WINDOW (chooser_dialog), "error \
                                     in myfm_window when opening file(s) with \
                                     '%s': %s \n",
                                     g_app_info_get_display_name (app_info),
                                     error->message);
        g_critical ("error in myfm_file_menu when opening file(s) with '%s': %s \n",
                    g_app_info_get_display_name (app_info), error->message);
        g_error_free (error);
    }
    else {
        gtk_window_close (GTK_WINDOW (chooser_dialog));
    }
}

/* handler for gtk_app_chooser_dialog's "response"
 * signal (inherited from gtk_dialog) */
static void
on_app_chooser_response (GtkDialog *chooser_dialog,
                         gint response_id, gpointer data)
{
    if (response_id == GTK_RESPONSE_CANCEL) {
        gtk_window_close (GTK_WINDOW (chooser_dialog));
    }
    /* when 'select' button is pressed or an app is double clicked */
    else if (response_id == GTK_RESPONSE_OK) {
        GtkWidget *chooser_widget;
        GAppInfo *app_info;

        app_info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (chooser_dialog));
        chooser_widget = gtk_app_chooser_dialog_get_widget (GTK_APP_CHOOSER_DIALOG (chooser_dialog));

        on_app_chooser_item_activate (GTK_APP_CHOOSER_WIDGET (chooser_widget),
                                      app_info, chooser_dialog);
    }
}

static void
myfm_file_menu_on_open_with_other (GtkMenuItem *item,
                                   gpointer myfm_file_menu)
{
    GtkWidget *chooser_dialog;
    MyFMFileMenu *self;
    GtkWindow *parent;

    self = MYFM_FILE_MENU (myfm_file_menu);
    parent = GTK_WINDOW (myfm_file_menu_get_window (self));
    chooser_dialog = gtk_app_chooser_dialog_new (parent, GTK_DIALOG_MODAL,
                                          myfm_file_get_g_file (self->file));
    g_signal_connect (GTK_DIALOG (chooser_dialog), "response",
                      G_CALLBACK (on_app_chooser_response), self);

    gtk_widget_show_all (chooser_dialog);
}

static GtkWidget *
myfm_file_menu_new_submenu_for_open_with (MyFMFileMenu *self)
{
    GtkWidget *submenu;
    GtkWidget *other_apps;
    GList *recommended_apps;
    GList *current;

    submenu = gtk_menu_new ();
    recommended_apps = g_app_info_get_recommended_for_type (myfm_file_get_content_type (self->file));
    current = g_list_first (recommended_apps);

    while (current) {
        GtkWidget *item;
        GAppInfo *app_info;

        app_info = current->data;
        item = myfm_utils_new_menu_item (g_app_info_get_display_name (app_info), 0, 0);
        gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
        gtk_widget_show (item);
        g_object_set_data_full (G_OBJECT (item), "app_info",
                                app_info, g_object_unref);
        g_signal_connect (GTK_MENU_ITEM (item), "activate",
                          G_CALLBACK (myfm_file_menu_on_open_with_app),
                          self);

        current = current->next;
    }
    g_list_free (recommended_apps);

    other_apps = myfm_utils_new_menu_item ("Other Applications...", 0, 0);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), other_apps);
    gtk_widget_show (other_apps);
    g_signal_connect (GTK_MENU_ITEM (other_apps), "activate",
                      G_CALLBACK (myfm_file_menu_on_open_with_other),
                      self);

    return submenu;
}

static void
myfm_file_menu_append_and_setup (MyFMFileMenu *self, GtkWidget *menu_item)
{
    gtk_menu_shell_append (GTK_MENU_SHELL (self), menu_item);
    gtk_widget_show (menu_item);

    g_signal_connect (GTK_MENU_ITEM (menu_item), "activate",
                      G_CALLBACK (myfm_file_menu_on_item_activate),
                      self);
}

static void
myfm_file_menu_fill (MyFMFileMenu *self)
{
    GtkWidget *open_with;
    GtkWidget *open_with_submenu;
    GtkWidget *copy;
    GtkWidget *cut;

    open_with = myfm_utils_new_menu_item ("Open With", 0, 0);
    open_with_submenu = myfm_file_menu_new_submenu_for_open_with (self);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (open_with),
                               open_with_submenu);

    myfm_file_menu_append_and_setup (self, myfm_utils_new_menu_item ("Open", GDK_KEY_Return, 0));
    myfm_file_menu_append_and_setup (self, open_with);
    myfm_file_menu_append_and_setup (self, myfm_utils_new_menu_item ("Open in New Window", 0, 0));

    copy = myfm_utils_new_menu_item ("Copy", 0, 0);
    cut = myfm_utils_new_menu_item ("Cut", 0, 0);

    myfm_file_menu_append_and_setup (self, copy);
    myfm_file_menu_append_and_setup (self, cut);

    myfm_file_menu_append_and_setup (self, myfm_utils_new_menu_item ("Rename...", 0, 0));
    myfm_file_menu_append_and_setup (self, myfm_utils_new_menu_item ("Delete", 0, 0));
}


static void
myfm_file_menu_init (MyFMFileMenu *self)
{
    self->file = NULL;
    self->dirview = NULL;
}

static void
myfm_file_menu_class_init (MyFMFileMenuClass *cls)
{
}

GtkWidget *
myfm_file_menu_new (MyFMDirectoryView *dirview, MyFMFile *file)
{
    MyFMFileMenu *self;

    self = g_object_new (MYFM_TYPE_FILE_MENU, NULL);
    self->file = file;
    self->dirview = dirview;

    myfm_file_menu_fill (self);

    return GTK_WIDGET (self);
}
