//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>

#include "myfm-application.h"
#include "myfm-application-window.h"
#include "myfm-utils.h"
#include "myfm-file.h"

struct _MyFMApplicationWindow
{
    GtkApplicationWindow parent;
};

G_DEFINE_TYPE (MyFMApplicationWindow, myfm_application_window, GTK_TYPE_APPLICATION_WINDOW);

/* forward declaration of non-class/instance functions*/
static void on_file_select (GtkTreeView *treeview, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer window_box);
static void myfm_filename_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data);
static void insert_column (GtkTreeView *treeview);
static void open_dir (GFile *g_file_dir, GtkBox *window_box);


static void on_file_select (GtkTreeView *treeview, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer window_box)
{
    gpointer myfm_file;
    GtkTreeModel *tree_model;
    GtkTreeIter iter;

    tree_model = gtk_tree_view_get_model (treeview);
    gtk_tree_model_get_iter (tree_model, &iter, path);
    gtk_tree_model_get (tree_model, &iter, 0, &myfm_file, -1);
    if (myfm_file)
        open_dir (((MyFMFile*) myfm_file)->g_file, GTK_BOX (window_box));
}

static void myfm_filename_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (tree_model, iter, 0, &myfm_file, -1); /* out myfm_file, pass by value */
    if (myfm_file)
        g_object_set (cell, "text", ((MyFMFile *) myfm_file)->IO_display_name, NULL);
}

static void insert_column (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes ("col", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (col, renderer, myfm_filename_data_func, NULL, NULL);
    gtk_tree_view_column_set_resizable (col, TRUE);
    gtk_tree_view_column_set_expand (col, TRUE);

    gtk_tree_view_append_column (treeview, col);
}

static void open_dir (GFile *g_file_dir, GtkBox *window_box)
{
    GFileType filetype = g_file_query_file_type (g_file_dir, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (filetype != G_FILE_TYPE_DIRECTORY)
        return; /* TODO: G_CRITICAL? SOMETHING */

    GtkTreeView *treeview;
    GtkListStore *store;
    guint padding = 0; /* TODO: INSTANCE VAR */

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    children_to_store_async (g_file_dir, store);

    // children_to_store_finished.connect ( clear_unused_treeviews );
    // children_to_store_finished.connect ( setup_treeview );
    //      (above funcs should auto-disconnect themselves after finishing)


    treeview = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref (store); /* treeview increases refcount, so we dont need to keep our reference */
    insert_column (treeview);
    gtk_tree_view_set_headers_visible (treeview, TRUE);
    g_signal_connect (treeview, "row-activated", G_CALLBACK (on_file_select), (gpointer) window_box);

    gtk_box_pack_start (window_box, GTK_WIDGET (treeview), FALSE, FALSE, padding);
    /* TODO: CONNECT THIS GUY TO STORE_FINISHED */
    gtk_widget_show (GTK_WIDGET (treeview));
}

void myfm_application_window_new_tab (MyFMApplicationWindow *win)
{
}

static void myfm_application_window_init (MyFMApplicationWindow *win)
{
}

static void myfm_application_window_class_init (MyFMApplicationWindowClass *cls)
{
}

MyFMApplicationWindow *myfm_application_window_new (MyFMApplication *app)
{
    return g_object_new (MYFM_APPLICATION_WINDOW_TYPE, "application", app, NULL);
}

void myfm_application_window_open (MyFMApplicationWindow *win, GFile *file)
{
    GtkBox *box;
    gint spacing = 0; // TODO: INSTANCE VAR

    box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, spacing);
    gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET (box));
    gtk_widget_show (GTK_WIDGET (box));

    open_dir (file, box); // TODO: IF FILE ISN'T DIR THEN APP WONT OPEN, FIX THIS
}