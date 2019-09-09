//
// Created by f35 on 07.09.19.
//

#include <gtk/gtk.h>

#include "myfm-tree-view.h"
#include "myfm-file.h"
#include "myfm-window.h"

struct _MyFMTreeView
{
    GtkTreeView parent_instance;
    MyFMWindow *parent_window;
};

G_DEFINE_TYPE (MyFMTreeView, myfm_tree_view, GTK_TYPE_TREE_VIEW)

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
        myfm_window_open_async ((MyFMWindow*) win, ((MyFMFile*) myfm_file)->g_file);
}

static void myfm_filename_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                     GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (tree_model, iter, 0, &myfm_file, -1); /* out myfm_file, pass by value */
    if (myfm_file)
        g_object_set (cell, "text", ((MyFMFile *) myfm_file)->IO_display_name, NULL);
}

void myfm_tree_view_append_file_column (MyFMTreeView *self)
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

static void myfm_tree_view_destroy (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (myfm_tree_view_parent_class)->destroy (widget);
}

static void myfm_tree_view_dispose (GObject *object)
{
    /* TODO: G_CLEAR_OBJECT and so on */

    G_OBJECT_CLASS (myfm_tree_view_parent_class)->dispose (object);
}

static void myfm_tree_view_finalize (GObject *object)
{
    /* TODO: free stuff, etc. */

    G_OBJECT_CLASS (myfm_tree_view_parent_class)->finalize (object);
}

static void myfm_tree_view_constructed (GObject *object)
{
    MyFMTreeView *self;

    G_OBJECT_CLASS (myfm_tree_view_parent_class)->constructed (object);

    self = MYFM_TREE_VIEW (object); /* pointless */

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
    gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (self), TRUE);

    g_signal_connect (self, "row-activated", G_CALLBACK (on_file_select), (gpointer) self->parent_window);
}

static void myfm_tree_view_init (MyFMTreeView *self)
{
    MyFMWindow *parent_window;

    if (GTK_IS_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))))
        parent_window = (MyFMWindow*) gtk_widget_get_toplevel (GTK_WIDGET (self));
    else
        parent_window = NULL;

    self->parent_window = parent_window;
}

static void myfm_tree_view_class_init (MyFMTreeViewClass *cls)
{
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);
    GtkWidgetClass *widget_cls = GTK_WIDGET_CLASS (cls);
    /* GtkTreeViewClass *treeview_cls = GTK_TREE_VIEW_CLASS (cls); */

    object_cls->dispose = myfm_tree_view_dispose;
    object_cls->finalize = myfm_tree_view_finalize;
    object_cls->constructed = myfm_tree_view_constructed;

    widget_cls->destroy = myfm_tree_view_destroy;
}

MyFMTreeView *myfm_tree_view_new_with_model (GtkTreeModel *model)
{
    return g_object_new (MYFM_TREE_VIEW_TYPE, "model", model, NULL);
}

MyFMTreeView *myfm_tree_view_new (void)
{
    return g_object_new (MYFM_TREE_VIEW_TYPE, NULL);
}
