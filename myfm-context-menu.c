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
}

static void myfm_context_menu_append_item (MyFMContextMenu *self, const gchar *label,
                                           guint keyval, GdkModifierType accel_mods)
{
    GtkWidget *menu_item;
    GtkWidget *child;

    menu_item = gtk_menu_item_new_with_label (label);
    child = gtk_bin_get_child (GTK_BIN (menu_item));

    gtk_accel_label_set_accel (GTK_ACCEL_LABEL (child), keyval, accel_mods);
    gtk_menu_shell_append (GTK_MENU_SHELL (self), menu_item);
    gtk_widget_show (menu_item);
    g_signal_connect (GTK_MENU_ITEM (menu_item), "activate",
                      G_CALLBACK (myfm_context_menu_on_item_activate), self);
}

static void myfm_context_menu_fill_for_directory_view (MyFMContextMenu *self)
{
}

static void myfm_context_menu_fill_for_file (MyFMContextMenu *self)
{
    myfm_context_menu_append_item (self, "Open", GDK_KEY_Return, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
    myfm_context_menu_append_item (self, "blabla", 0, 0);
}

void myfm_context_menu_init (MyFMContextMenu *self)
{
    self->file = NULL;
    self->dirview = NULL;
}

void myfm_context_menu_class_init (MyFMContextMenuClass *cls)
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
