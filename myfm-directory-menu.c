/*
 * Created by f35 on 24.11.19.
*/

#include <gtk/gtk.h>

#include "myfm-window.h"
#include "myfm-directory-view.h"
#include "myfm-utils.h"
#include "myfm-copy-operation.h"
#include "myfm-directory-menu.h"
#define G_LOG_DOMAIN "myfm-directory-menu"

struct _MyFMDirectoryMenu {
    GtkMenu parent_instance;
    MyFMDirectoryView *dirview;
};

G_DEFINE_TYPE (MyFMDirectoryMenu, myfm_directory_menu, GTK_TYPE_MENU)

static MyFMWindow *
myfm_directory_menu_get_window (MyFMDirectoryMenu *self)
{
    return MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self->dirview)));
}

static void
myfm_directory_menu_on_paste_activate (GtkMenu *item,
                                       gpointer self)
{
    GtkApplication *app;
    MyFMClipBoard *cboard;
    MyFMWindow *win;
    MyFMFile **src_files;
    MyFMFile *dest_dir;
    gint n_files;
    gboolean files_copied;

    win = myfm_directory_menu_get_window (self);
    app = gtk_window_get_application (GTK_WINDOW (win));
    cboard = myfm_application_get_file_clipboard (MYFM_APPLICATION (app));
    dest_dir = myfm_directory_view_get_directory (MYFM_DIRECTORY_MENU (self)->dirview);

    src_files = myfm_clipboard_get_contents (cboard, &n_files, &files_copied);

    if (!n_files)
        return;

    if (files_copied) {
        myfm_copy_operation_start_async (src_files, n_files, dest_dir,
                                         GTK_WINDOW (myfm_directory_menu_get_window (self)),
                                         NULL, NULL);
    }
   else { /* clipboard files are cut */

    }

    for (int i = 0; i < n_files; i ++)
        myfm_file_unref (src_files[i]);

    g_free (src_files);
}

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

/* terrible naming. sets up the submenu for the "new" button */
static GtkWidget *
new_submenu_for_new (void)
{
    GtkWidget *submenu;
    GtkWidget *new_file;
    GtkWidget *new_directory;

    submenu = gtk_menu_new ();
    new_directory = myfm_utils_new_menu_item ("Directory", 0, 0);
    new_file = myfm_utils_new_menu_item ("Empty File", 0, 0);

    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), new_directory);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), new_file);
    gtk_widget_show (new_directory);
    gtk_widget_show (new_file);

    return submenu;
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
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Last edited "
                        "(most recent first)", 0, 0),
                        G_CALLBACK (on_sort_by_date_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Last edited "
                        "(most recent last)", 0, 0),
                        G_CALLBACK (on_sort_by_date_reverse_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Name (A-Z)", 0, 0),
                        G_CALLBACK (on_sort_by_name_a_z_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Name (Z-A)", 0, 0),
                        G_CALLBACK (on_sort_by_name_z_a_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Size "
                        "(smallest to largest)", 0, 0),
                        G_CALLBACK (on_sort_by_size_activate), self->dirview);
    setup_sort_subitem (submenu, myfm_utils_new_menu_item ("Size "
                        "(largest to smallest)", 0, 0),
                        G_CALLBACK (on_sort_by_size_reverse_activate), self->dirview);

    return submenu;
}

static void
myfm_directory_menu_fill (MyFMDirectoryMenu *self)
{
    GtkWindow *win;
    MyFMApplication *app;
    GtkWidget *new_item;
    GtkWidget *new_submenu;
    GtkWidget *paste_item;
    GtkWidget *sort_item;
    GtkWidget *sort_submenu;

    win = GTK_WINDOW (myfm_directory_menu_get_window (self));
    app = MYFM_APPLICATION (gtk_window_get_application (win));

    new_item = myfm_utils_new_menu_item ("New...", 0, 0);
    new_submenu = new_submenu_for_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (new_item), new_submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (self), new_item);
    gtk_widget_show (new_item);

    paste_item = myfm_utils_new_menu_item ("Paste", 0, 0);
    gtk_menu_shell_append (GTK_MENU_SHELL (self), paste_item);
    if (myfm_application_copy_in_progress (app))
        gtk_widget_set_sensitive (paste_item, FALSE);
    gtk_widget_show (paste_item);
    g_signal_connect (GTK_MENU_ITEM (paste_item), "activate",
                      myfm_directory_menu_on_paste_activate, self);

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
