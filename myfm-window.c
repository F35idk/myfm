//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>

#include "myfm-application.h"
#include "myfm-window.h"
#include "myfm-utils.h"
#include "myfm-file.h"
#include "myfm-tree-view.h"

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

G_DEFINE_TYPE (MyFMWindow, myfm_window, GTK_TYPE_APPLICATION_WINDOW)

static void show_treeview_callback (gpointer store, gpointer treeview_scroll)
{
    gtk_widget_show_all (GTK_WIDGET (treeview_scroll));
}

static void myfm_window_open_dir_async (MyFMWindow *self, GFile *g_file_dir)
{
    MyFMTreeView *treeview;
    GtkScrolledWindow *treeview_scroll;
    GtkListStore *store;

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    children_to_store_async (g_file_dir, store);

    treeview = myfm_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref (store);
    myfm_tree_view_append_file_column (treeview);

    treeview_scroll = (GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (treeview_scroll), GTK_WIDGET (treeview));

    gtk_box_pack_start (self->window_box, GTK_WIDGET (treeview_scroll), TRUE, TRUE, self->box_padding);

    /* wait until all files are in store to show our treeview */
    g_signal_connect (store, "children_added", G_CALLBACK (show_treeview_callback), treeview_scroll);
}

static void myfm_window_open_file_async (MyFMWindow *self, GFile *g_file)
{
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
    object_cls->finalize = myfm_window_finalize;
    object_cls->constructed = myfm_window_constructed;

    widget_cls->destroy = myfm_window_destroy;
}

MyFMWindow *myfm_window_new (MyFMApplication *app)
{
    return g_object_new (MYFM_WINDOW_TYPE, "application", app, NULL);
}