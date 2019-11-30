/*
 * Created by f35 on 24.11.19.
*/

#include <gtk/gtk.h>

#include "myfm-window.h"
#include "myfm-directory-view.h"
#include "myfm-context-menu.h"

struct _MyFMContextMenu {
    GtkMenu parent_instance;
    MyFMDirectoryView *dirview;
    MyFMFile *file;
};

G_DEFINE_TYPE (MyFMContextMenu, myfm_context_menu, GTK_TYPE_MENU)

static void myfm_context_menu_on_item_activate (GtkMenuItem *item, gpointer myfm_context_menu)
{
    MyFMWindow *window;
    MyFMContextMenu *self;
    const gchar *label;

    self = MYFM_CONTEXT_MENU (myfm_context_menu);
    label = gtk_menu_item_get_label (item);

    if (!strcmp (label, "Open")) {
        gint dirview_index;

        window = MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self->dirview)));
        dirview_index = myfm_window_get_directory_view_index (window, self->dirview);

        myfm_window_open_file_async (window, self->file, dirview_index);
    }
    else if (!strcmp (label, "test")) {
        puts ("test");
    }
}

static void myfm_context_menu_on_item_hover (GtkMenuItem *item, gpointer myfm_context_menu)
{
    /* NOTE: turns out submenus auto-popup, so we don't need to do it manually */
    /*
    MyFMWindow *window;
    MyFMContextMenu *self;
    const gchar *label;

    self = MYFM_CONTEXT_MENU (myfm_context_menu);
    label = gtk_menu_item_get_label (item);

    if (!strcmp (label, "Open With")) {
        GtkWidget *submenu;

        submenu = gtk_menu_item_get_submenu (item);
        gtk_menu_popup_at_widget (GTK_MENU (submenu), gtk_widget_get_parent (GTK_WIDGET (item)),
                                  GDK_GRAVITY_NORTH_EAST, GDK_GRAVITY_NORTH_WEST, NULL);
    }
    */
}

static void myfm_context_menu_on_open_with (GtkMenuItem *item, gpointer myfm_context_menu)
{
    MyFMContextMenu *self;
    GAppInfo *app_info;
    GList *g_file_list;

    self = MYFM_CONTEXT_MENU (myfm_context_menu);
    app_info = g_object_get_data (G_OBJECT (item), "app_info");
    g_file_list = g_list_append (NULL, myfm_file_get_g_file (self->file));

    /* FIXME: don't pass NULL as error */
    g_app_info_set_as_last_used_for_type (app_info, myfm_file_get_content_type (self->file), NULL);
    g_app_info_launch (app_info, g_file_list, NULL, NULL);

    g_list_free (g_file_list);
}

GtkWidget *new_menu_item (const gchar *label, guint keyval, GdkModifierType accel_mods)
{
    GtkWidget *menu_item;
    GtkWidget *child;

    menu_item = gtk_menu_item_new_with_label (label);
    child = gtk_bin_get_child (GTK_BIN (menu_item));

    if (keyval || accel_mods)
        gtk_accel_label_set_accel (GTK_ACCEL_LABEL (child), keyval, accel_mods);

    return menu_item;
}

GtkWidget *myfm_context_menu_new_submenu_for_open_with (MyFMContextMenu *self)
{
    GtkWidget *submenu;
    GList *recommended_apps;
    GList *current;

    submenu = gtk_menu_new ();
    recommended_apps = g_app_info_get_recommended_for_type (myfm_file_get_content_type (self->file));
    current = g_list_first (recommended_apps);

    while (current) {
        GtkWidget *item;
        GAppInfo *app_info;

        app_info = current->data;
        item = new_menu_item (g_app_info_get_display_name (app_info), 0, 0);
        gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
        gtk_widget_show (item);
        g_object_set_data_full (G_OBJECT (item), "app_info", app_info, g_object_unref);
        g_signal_connect (GTK_MENU_ITEM (item), "activate",
                          G_CALLBACK (myfm_context_menu_on_open_with), self);

        current = current->next;
    }

    g_list_free (recommended_apps);

    return submenu;
}

static void myfm_context_menu_append_and_setup (MyFMContextMenu *self, GtkWidget *menu_item, gboolean hover)
{
    gtk_menu_shell_append (GTK_MENU_SHELL (self), menu_item);
    gtk_widget_show (menu_item);

    if (!hover)
        g_signal_connect (GTK_MENU_ITEM (menu_item), "activate",
                          G_CALLBACK (myfm_context_menu_on_item_activate), self);
    else
        g_signal_connect (GTK_MENU_ITEM (menu_item), "select",
                          G_CALLBACK (myfm_context_menu_on_item_hover), self);
}

static void myfm_context_menu_fill_for_directory_view (MyFMContextMenu *self)
{
}

static void myfm_context_menu_fill_for_file (MyFMContextMenu *self)
{
    GtkWidget *open_with;
    GtkWidget *open_with_submenu;

    open_with = new_menu_item ("Open With", 0, 0);
    open_with_submenu = myfm_context_menu_new_submenu_for_open_with (self);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (open_with), open_with_submenu);

    myfm_context_menu_append_and_setup (self, new_menu_item ("Open", GDK_KEY_Return, 0), FALSE);
    myfm_context_menu_append_and_setup (self, open_with, TRUE);
    myfm_context_menu_append_and_setup (self, new_menu_item ("blabla", 0, 0), FALSE);
    myfm_context_menu_append_and_setup (self, new_menu_item ("blabla", 0, 0), FALSE);
    myfm_context_menu_append_and_setup (self, new_menu_item ("blabla", 0, 0), FALSE);
    myfm_context_menu_append_and_setup (self, new_menu_item ("blabla", 0, 0), FALSE);
    myfm_context_menu_append_and_setup (self, new_menu_item ("blabla", 0, 0), FALSE);
    myfm_context_menu_append_and_setup (self, new_menu_item ("test", 0, 0), FALSE);
}

static void myfm_context_menu_init (MyFMContextMenu *self)
{
    self->file = NULL;
    self->dirview = NULL;
}

static void myfm_context_menu_class_init (MyFMContextMenuClass *cls)
{
}

/* NOTE: these constructors may not be good for potential language bindings */
MyFMContextMenu *myfm_context_menu_new_for_file (MyFMDirectoryView *dirview, MyFMFile *file)
{
    MyFMContextMenu *self;

    self = g_object_new (MYFM_TYPE_CONTEXT_MENU, NULL);
    self->file = file;
    self->dirview = dirview;

    myfm_context_menu_fill_for_file (self);

    return self;
}

MyFMContextMenu *myfm_context_menu_new_for_directory_view (MyFMDirectoryView *dirview)
{
    MyFMContextMenu *self;

    self = g_object_new (MYFM_TYPE_CONTEXT_MENU, NULL);
    self->dirview = dirview;

    myfm_context_menu_fill_for_directory_view (self);

    return self;
}
