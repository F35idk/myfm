//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>

#include "myfm-application.h"
#include "myfm-window.h"
#include "myfm-utils.h"
#include "myfm-file.h"

struct _MyFMWindow
{
    GtkApplicationWindow parent_instance;

    GtkBox *window_box;
    guint box_padding;
    gint box_spacing;
    /* TODO: some of these could be static vars instead */
    gint default_height;
    gint default_width;
};

G_DEFINE_TYPE (MyFMWindow, myfm_window, GTK_TYPE_APPLICATION_WINDOW);

/* forward declare to allow mutual recursion (on_file_select and this func depend on each other) */
static void myfm_window_open_dir_async (MyFMWindow *self, GFile *g_file_dir);

/* TODO: belongs in our future treeview subclass */
static void on_file_select (GtkTreeView *treeview, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer win)
{
    gpointer myfm_file;
    GtkTreeModel *tree_model;
    GtkTreeIter iter;

    tree_model = gtk_tree_view_get_model (treeview);
    gtk_tree_model_get_iter (tree_model, &iter, path);
    gtk_tree_model_get (tree_model, &iter, 0, &myfm_file, -1);
    if (myfm_file)
        myfm_window_open_dir_async ((MyFMWindow*) win, ((MyFMFile*) myfm_file)->g_file);
}

/* TODO: belongs in our future treeview subclass */
static void myfm_filename_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (tree_model, iter, 0, &myfm_file, -1); /* out myfm_file, pass by value */
    if (myfm_file)
        g_object_set (cell, "text", ((MyFMFile *) myfm_file)->IO_display_name, NULL);
}

/* TODO: belongs in our future treeview subclass */
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

static void myfm_window_open_file_async (MyFMWindow *self, GFile *g_file)
{
}

/* usually doesn't return before async IO is finished, but it might, so keep this in mind */
static void myfm_window_open_dir_async (MyFMWindow *self, GFile *g_file_dir)
{
    GtkTreeView *treeview;
    GtkListStore *store;

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    children_to_store_async (g_file_dir, store);

    // children_to_store_finished.connect ( clear_unused_treeviews );
    // children_to_store_finished.connect ( setup_treeview );
    //      (above funcs should auto-disconnect themselves after finishing)


    treeview = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref (store); /* treeview increases refcount, so we dont need to keep our reference */
    insert_column (treeview);
    gtk_tree_view_set_headers_visible (treeview, FALSE);
    gtk_tree_view_set_rubber_banding (treeview, TRUE); /* not effective yet */
    g_signal_connect (treeview, "row-activated", G_CALLBACK (on_file_select), (gpointer) self);

    gtk_box_pack_start (self->window_box, GTK_WIDGET (treeview), TRUE, TRUE, self->box_padding);
    /* TODO: CONNECT THIS GUY TO STORE_FINISHED */
    gtk_widget_show (GTK_WIDGET (treeview));
}

void myfm_window_open_async (MyFMWindow *self, GFile *file)
{
    GFileType filetype;

    filetype = g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (filetype != G_FILE_TYPE_DIRECTORY)
        myfm_window_open_file_async  (self, file);
    else
        myfm_window_open_dir_async (self, file);
}

static void myfm_window_destroy (GtkWidget *widget)
{
    /* chaining up */
    GTK_WIDGET_CLASS (myfm_window_parent_class)->destroy (widget);
}

static void myfm_window_dispose (GObject *object)
{
    /* TODO: G_CLEAR_OBJECT and so on */

    /* chaining up */
    G_OBJECT_CLASS (myfm_window_parent_class)->dispose (object);
}

static void myfm_window_finalize (GObject *object)
{
    /* TODO: run free funcs, g_assert that shutdown is going well, etc. */

    G_OBJECT_CLASS (myfm_window_parent_class)->finalize (object);
}

static void myfm_window_constructed (GObject *object)
{
    MyFMWindow *self;

    G_OBJECT_CLASS (myfm_window_parent_class)->constructed (object);


    self = MYFM_WINDOW (object);
    gtk_window_set_default_size (GTK_WINDOW (self), self->default_width, self->default_height);
    /* do these belong here? */
    gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->window_box));
    gtk_widget_show (GTK_WIDGET (self->window_box));
}

static void myfm_window_init (MyFMWindow *self)
{
    /* set up instance vars */
    self->default_height = 550; /* these don't need to be tied to our instance */
    self->default_width = 890;
    self->box_padding = 0;
    self->box_spacing = 0;
    self->window_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, self->box_spacing);
}

static void myfm_window_class_init (MyFMWindowClass *cls)
{

    GtkWidgetClass *widget_cls = GTK_WIDGET_CLASS (cls);
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);

    object_cls->dispose = myfm_window_dispose;
    object_cls->constructed = myfm_window_constructed;

    widget_cls->destroy = myfm_window_destroy;
}

MyFMWindow *myfm_window_new (MyFMApplication *app)
{
    return g_object_new (MYFM_WINDOW_TYPE, "application", app, NULL);
}