//
// Created by f35 on 15.08.19.
//

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-application.h"
#include "myfm-application-window.h"
#include "myfm-file.h"
#include "myfm-utils.h"

struct _MyFMApplication
{
    GtkApplication parent;
};

G_DEFINE_TYPE (MyFMApplication, myfm_application, GTK_TYPE_APPLICATION);

static void myfm_application_init (MyFMApplication *app)
{
}

static void myfm_filename_to_renderer (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (tree_model, iter, 0, &myfm_file, -1); // out myfm_file, pass by value
    if (myfm_file)
        g_object_set(cell, "text", ((MyFMFile *) myfm_file)->display_name, NULL);
}

static void insert_column (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_text_new (); // TODO: GLOBAL
    col = gtk_tree_view_column_new_with_attributes ("col", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (col, renderer, myfm_filename_to_renderer, NULL, NULL);
    gtk_tree_view_column_set_resizable (col, TRUE);
    gtk_tree_view_column_set_expand (col, TRUE);

    gtk_tree_view_append_column (treeview, col);
}

static void open_dir (MyFMFile *directory, GtkBox *window_box);

static void on_dir_select (GtkTreeView *treeview, GtkTreePath *path,
                           GtkTreeViewColumn *column, gpointer window_box)
{
    gpointer myfm_file;
    GtkTreeModel *tree_model;
    GtkTreeIter iter;

    tree_model = gtk_tree_view_get_model (treeview);
    gtk_tree_model_get_iter (tree_model, &iter, path);
    gtk_tree_model_get (tree_model, &iter, 0, &myfm_file, -1); // out myfm_file, pass by value
    if (myfm_file)
        open_dir ((MyFMFile*) myfm_file, GTK_BOX (window_box));
}

static void open_dir (MyFMFile *directory, GtkBox *window_box)
{
    GtkTreeView *treeview;
    GtkListStore *store;
    guint padding = 0; // TODO: STORE SOMEWHERE ELSE!!!!!!

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    children_to_store_async (directory->g_file, store);

    // children_to_store_finished.connect ( clear_unused_treeviews );
    // children_to_store_finished.connect ( setup_treeview );
    //      (above funcs should auto-disconnect themselves after finishing)


    treeview = (GtkTreeView*) gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    insert_column (treeview);
    gtk_tree_view_set_headers_visible (treeview, TRUE);
    g_signal_connect (treeview, "row-activated", G_CALLBACK (on_dir_select), (gpointer) window_box);
    gtk_box_pack_start (window_box, GTK_WIDGET (treeview), FALSE, FALSE, padding);
    gtk_widget_show (GTK_WIDGET (treeview));
}

/* to be used in activate and open virtual functions */
/* TODO: possibly useless */
static void myfm_application_new_window (MyFMApplication *app)
{
    GtkBox *box;
    MyFMApplicationWindow *win;
    gint spacing = 0;
    guint padding = 0;

    box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, spacing);

    win = myfm_application_window_new (app);
    gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET (box));

    gtk_widget_show_all (GTK_WIDGET (win));
    gtk_window_present (GTK_WINDOW (win));

    MyFMFile *home = malloc (sizeof (MyFMFile));
    myfm_file_from_path_async (home, "/home/f35/Documents"); // out home

    open_dir (home, box);
}

/* application launched with no args */
static void myfm_application_activate (GApplication *app)
{
    /* TODO: not sure we need our own GtkApplicationWindow subclass, but we'll keep it for now */
    // MyFMApplicationWindow *win;

    // win = myfm_application_window_new (MYFM_APPLICATION (app));
    myfm_application_new_window (MYFM_APPLICATION (app));


}

/* application launched with args */
static void myfm_application_open (GApplication *app, GFile **files,
                                   gint n_files, const gchar *hint)
{
    GList *windows;
    MyFMApplicationWindow *win;

    windows = gtk_application_get_windows (GTK_APPLICATION (app));
    if (windows)
        win = MYFM_APPLICATION_WINDOW (windows->data);
    else
        win = myfm_application_window_new (MYFM_APPLICATION (app));

    for (int i = 0; i < n_files; i++)
        myfm_application_window_open (win, files[i]);

    gtk_window_present (GTK_WINDOW (win));
}

static void myfm_application_startup (GApplication *app)
{
    /* chaining up */
    G_APPLICATION_CLASS (myfm_application_parent_class)->startup (app);
}

static void myfm_application_finalize (GObject *object)
{
    /* chaining up */
    G_OBJECT_CLASS (myfm_application_parent_class)->finalize (object);
}

static void myfm_application_class_init (MyFMApplicationClass *cls)
{
    /* replacing overridden functions in class object? i think that's what we're doing */
    GApplicationClass *app_cls = G_APPLICATION_CLASS (cls);
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);

    app_cls->activate = myfm_application_activate;
    app_cls->open = myfm_application_open;

    object_cls->finalize = myfm_application_finalize;
}

MyFMApplication *myfm_application_new (void)
{
    MyFMApplication *app;

    g_set_application_name ("myfm");
    app = g_object_new (MYFM_APPLICATION_TYPE, "application-id", "org.gtk.exampleapp", // TODO: replace the id!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        "flags", G_APPLICATION_HANDLES_OPEN, NULL);

    return app;
}