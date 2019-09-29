//
// Created by f35 on 07.09.19.
//

#include <gtk/gtk.h>

#include "myfm-directory-view.h"
#include "myfm-file.h"
#include "myfm-window.h"
#include "myfm-directory-view-utils.h"

struct _MyFMDirectoryView
{
    GtkTreeView parent_instance;
    MyFMFile *directory;
    GFileMonitor *directory_monitor;
};

G_DEFINE_TYPE (MyFMDirectoryView, myfm_directory_view, GTK_TYPE_TREE_VIEW)

static void on_file_select (GtkTreeView *treeview, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeModel *tree_model;
    MyFMWindow *parent_window;
    gint dirview_index;
    GtkTreeIter iter;
    gpointer myfm_file;

    parent_window = MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (treeview)));
    dirview_index = myfm_window_get_directory_view_index (parent_window, MYFM_DIRECTORY_VIEW (treeview));
    tree_model = gtk_tree_view_get_model (treeview);
    gtk_tree_model_get_iter (tree_model, &iter, path);
    gtk_tree_model_get (tree_model, &iter, 0, &myfm_file, -1);
    if (myfm_file)
        myfm_window_open_file_async (parent_window, (MyFMFile*) myfm_file, dirview_index);
}

/* GtkCellRenderer data function for getting the display name of a file in a cell */
static void myfm_filename_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                     GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (tree_model, iter, 0, &myfm_file, -1); /* out myfm_file, pass by value */
    if (myfm_file)
        g_object_set (cell, "text", ((MyFMFile *) myfm_file)->IO_display_name, NULL);
}

static void myfm_directory_view_append_file_column (MyFMDirectoryView *self)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes ("col", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (col, renderer, myfm_filename_data_func, NULL, NULL);
    gtk_tree_view_column_set_resizable (col, TRUE);
    gtk_tree_view_column_set_expand (col, TRUE);

    gtk_tree_view_append_column (GTK_TREE_VIEW (self), col);
}

static void myfm_directory_view_setup_store (MyFMDirectoryView *self)
{
    GtkListStore *store;

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), GTK_TREE_MODEL (store));
    g_object_unref (store);
}


void myfm_directory_view_fill_async (MyFMDirectoryView *self)
{
    files_to_store_async (self);
    myfm_directory_view_append_file_column (self);
}

MyFMFile *myfm_directory_view_get_directory (MyFMDirectoryView *self)
{
    return self->directory;
}

/* this exists only so i can set the dir on construction, and im too lazy (and
 * dont consider it worth it) to make self->directory a full blown gobject property */
static void myfm_directory_view_set_directory (MyFMDirectoryView *self, MyFMFile *dir)
{
    self->directory = dir;
    myfm_file_ref (dir);
}

static void myfm_directory_view_destroy (GtkWidget *widget)
{
    puts ("destryoing!!!!!!!!"); // TODO: DEBUG WARN, NOT PUTS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    GtkTreeModel *store;
    MyFMDirectoryView *self;

    self = MYFM_DIRECTORY_VIEW (widget);
    store = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));

    if (store)
        gtk_tree_model_foreach (store, clear_file_in_store, NULL);

    if (self->directory) {
        myfm_file_unref (self->directory);
        self->directory = NULL;
    }

    GTK_WIDGET_CLASS (myfm_directory_view_parent_class)->destroy (widget);
}

static void myfm_directory_view_dispose (GObject *object)
{
    G_OBJECT_CLASS (myfm_directory_view_parent_class)->dispose (object);
}

static void myfm_directory_view_finalize (GObject *object)
{
    G_OBJECT_CLASS (myfm_directory_view_parent_class)->finalize (object);
}

static void myfm_directory_view_constructed (GObject *object)
{
    MyFMDirectoryView *self;

    G_OBJECT_CLASS (myfm_directory_view_parent_class)->constructed (object);

    self = MYFM_DIRECTORY_VIEW (object);

    myfm_directory_view_setup_store (self);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
    gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (self), TRUE);

    g_signal_connect (self, "row-activated", G_CALLBACK (on_file_select), NULL);
}

static void myfm_directory_view_init (MyFMDirectoryView *self)
{
}

static void myfm_directory_view_class_init (MyFMDirectoryViewClass *cls)
{
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);
    GtkWidgetClass *widget_cls = GTK_WIDGET_CLASS (cls);

    object_cls->dispose = myfm_directory_view_dispose;
    object_cls->finalize = myfm_directory_view_finalize;
    object_cls->constructed = myfm_directory_view_constructed;

    widget_cls->destroy = myfm_directory_view_destroy;

    /* signal to emit when all files in dir are added (async) to store */
    g_signal_new ("filled", MYFM_DIRECTORY_VIEW_TYPE,
                  G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
                  NULL, G_TYPE_NONE, 0, NULL);
}

/* creates an empty directory view. */
MyFMDirectoryView *myfm_directory_view_new (MyFMFile *directory)
{
    MyFMDirectoryView *view;

    view = g_object_new (MYFM_DIRECTORY_VIEW_TYPE, NULL);
    myfm_directory_view_set_directory (view, directory);

    return view;
}
