//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>

#include "myfm-application.h"
#include "myfm-window.h"
#include "myfm-utils.h"
#include "myfm-file.h"
#include "myfm-directory-view.h"

struct _MyFMWindow
{
    GtkApplicationWindow parent_instance;

    GtkBox *window_box;
    GList *directory_views;
    guint box_padding;
    gint box_spacing;
    /* TODO: some of these could be static vars instead */
    gint default_height;
    gint default_width;
};

G_DEFINE_TYPE (MyFMWindow, myfm_window, GTK_TYPE_APPLICATION_WINDOW)

static void show_dirview_callback (gpointer store, gpointer dirview_scroll)
{
    gtk_widget_show_all (GTK_WIDGET (dirview_scroll));
}

/* function for opening directories */
static void myfm_window_open_dir_async (MyFMWindow *self, GFile *g_file_dir, gint dirview_index)
{
    MyFMDirectoryView *dirview;
    GtkScrolledWindow *dirview_scroll;
    GtkListStore *store;

    /* create new directory view and fill it with files */
    dirview = myfm_directory_view_new ();
    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dirview)));
    files_to_store_async (g_file_dir, store);
    myfm_directory_view_append_file_column (dirview);

    /* remove unused directory views */
    GList *element = g_list_nth (self->directory_views, dirview_index+1);
    while (element != NULL) {
        GList *next = element->next; /* store pointer to next before it changes */
        GtkWidget *scroll = gtk_widget_get_parent (GTK_WIDGET (element->data));
        self->directory_views = g_list_remove (self->directory_views, element->data);
        gtk_container_remove (GTK_CONTAINER (self->window_box), scroll); /* unrefs and destroys all children */
        element = next;
    }

    /* add new directory view to our ordered directory view list */
    self->directory_views = g_list_append (self->directory_views, dirview);

    /* make our directory view scrollable */
    dirview_scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
    gtk_container_add (GTK_CONTAINER (dirview_scroll), GTK_WIDGET (dirview));
    gtk_box_pack_start (self->window_box, GTK_WIDGET (dirview_scroll), TRUE, TRUE, self->box_padding);

    /* wait until all files are in store to show our directory view */
    g_signal_connect (store, "files_added", G_CALLBACK (show_dirview_callback), dirview_scroll);
}

/* function for opening any file that is not a directory */
static void myfm_window_open_other_async (MyFMWindow *self, GFile *g_file)
{
    /* make sure to keep the g_file passed into here alive! */
}

/* main function for opening files */
void myfm_window_open_file_async (MyFMWindow *self, MyFMFile *file, gint dirview_index)
{
    GFileType filetype;

    filetype = g_file_query_file_type (file->g_file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (filetype != G_FILE_TYPE_DIRECTORY) {
        myfm_window_open_other_async (self, file->g_file);
        file->is_open = TRUE;
    }
    else {
        myfm_window_open_dir_async (self, file->g_file, dirview_index);
    }
}

/* convenience alternative to open_file for when we just want to pass a g_file instead of a
 * myfm_file (happens on application startup). if a directory is passed, it will be opened
 * at index 0 (the leftmost column/directory_view) of the application window. */
void myfm_window_open_g_file_async (MyFMWindow *self, GFile *file)
{
    GFileType filetype;

    filetype = g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (filetype != G_FILE_TYPE_DIRECTORY)
        myfm_window_open_other_async  (self, file);
    else
        myfm_window_open_dir_async (self, file, -1);
}

GtkBox *myfm_window_get_box (MyFMWindow *self)
{
    return self->window_box;
}

gint myfm_window_get_directory_view_index (MyFMWindow *self, MyFMDirectoryView *dirview)
{
    return g_list_index (self->directory_views, dirview);
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
    self->directory_views = NULL;
    self->window_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, self->box_spacing));
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