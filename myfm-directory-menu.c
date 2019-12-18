/*
 * Created by f35 on 24.11.19.
*/

#include <gtk/gtk.h>

#include "myfm-window.h"
#include "myfm-directory-view.h"
#include "myfm-utils.h"
#include "myfm-directory-menu.h"
#define G_LOG_DOMAIN "myfm-directory-menu"

struct _MyFMDirectoryMenu {
    GtkMenu parent_instance;
    MyFMDirectoryView *dirview;
};

G_DEFINE_TYPE (MyFMDirectoryMenu, myfm_directory_menu, GTK_TYPE_MENU)

static void
on_sort_by_size_reverse_activate (GtkMenuItem *item, gpointer dirview)
{
    myfm_directory_view_set_file_sort_criteria (dirview, MYFM_SORT_LARGEST_FIRST);
}
static void
on_sort_by_size_activate (GtkMenuItem *item, gpointer dirview)
{
    myfm_directory_view_set_file_sort_criteria (dirview, MYFM_SORT_LARGEST_LAST);
}
static void
on_sort_by_name_z_a_activate (GtkMenuItem *item, gpointer dirview)
{
    myfm_directory_view_set_file_sort_criteria (dirview, MYFM_SORT_NAME_Z_TO_A);
}
static void
on_sort_by_name_a_z_activate (GtkMenuItem *item, gpointer dirview)
{
    myfm_directory_view_set_file_sort_criteria (dirview, MYFM_SORT_NAME_A_TO_Z);
}
static void
on_sort_by_date_reverse_activate (GtkMenuItem *item, gpointer dirview)
{
    myfm_directory_view_set_file_sort_criteria (dirview, MYFM_SORT_NEWLY_EDITED_LAST);
}
static void
on_sort_by_date_activate (GtkMenuItem *item, gpointer dirview)
{
    myfm_directory_view_set_file_sort_criteria (dirview, MYFM_SORT_NEWLY_EDITED_FIRST);
}

static void
setup_sort_subitem (GtkWidget *submenu, GtkWidget *subitem,
                    GCallback activate_handler, MyFMDirectoryView *dirview)
{
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), subitem);
    gtk_widget_show (subitem);
    g_signal_connect (GTK_MENU_ITEM (subitem), "activate",
                      activate_handler, dirview);
}

static GtkWidget *
myfm_directory_menu_new_submenu_for_sort (MyFMDirectoryMenu *self)
{
    GtkWidget *submenu;

    submenu = gtk_menu_new ();
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Last edited \
                        (most recent first)", 0, 0),
                        G_CALLBACK (on_sort_by_date_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Last edited \
                        (most recent last)", 0, 0),
                        G_CALLBACK (on_sort_by_date_reverse_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Name (A-Z)", 0, 0),
                        G_CALLBACK (on_sort_by_name_a_z_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Name (Z-A)", 0, 0),
                        G_CALLBACK (on_sort_by_name_z_a_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Size \
                        (smallest to largest)", 0, 0),
                        G_CALLBACK (on_sort_by_size_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Size \
                        (largest to smallest)", 0, 0),
                        G_CALLBACK (on_sort_by_size_reverse_activate), self->dirview);

    return submenu;
}

static void
myfm_directory_menu_fill (MyFMDirectoryMenu *self)
{
    GtkWidget *sort_item;
    GtkWidget *sort_submenu;

    sort_item = myfm_utils_new_menu_item ("Sort files by...", 0, 0);
    sort_submenu = myfm_directory_menu_new_submenu_for_sort (self);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (sort_item), sort_submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (self), sort_item);
    gtk_widget_show (sort_item);
}

static void
myfm_directory_menu_init (MyFMDirectoryMenu *self)
{
    self->dirview = NULL;
}

static void
myfm_directory_menu_class_init (MyFMDirectoryMenuClass *cls)
{
}

GtkWidget *
myfm_directory_menu_new (MyFMDirectoryView *dirview)
{
    MyFMDirectoryMenu *self;

    self = g_object_new (MYFM_TYPE_DIRECTORY_MENU, NULL);
    self->dirview = dirview;

    myfm_directory_menu_fill (self);

    return GTK_WIDGET (self);
}
