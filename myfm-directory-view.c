//
// Created by f35 on 07.09.19.
//

#include <gtk/gtk.h>

#include "myfm-directory-view.h"
#include "myfm-file.h"
#include "myfm-window.h"

struct _MyFMDirectoryView {
    GtkTreeView parent_instance;
    MyFMFile *directory;
    GFileMonitor *directory_monitor;
    GCancellable *IO_canceller;
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

/* functions only used in on_dir_change. should maybe be inlined to avoid confusion */
static void myfm_directory_view_on_file_renamed (MyFMDirectoryView *self, GFile *orig_g_file, GFile *new_g_file);
static void myfm_directory_view_on_file_moved_out (MyFMDirectoryView *self, GFile *orig_g_file);
static void myfm_directory_view_on_file_moved_in (MyFMDirectoryView *self, GFile *new_g_file);

static void myfm_directory_view_on_dir_change (GFileMonitor *monitor, GFile *file, GFile *other_file,
                           GFileMonitorEvent event_type, gpointer dirview)
{
   MyFMDirectoryView *self = MYFM_DIRECTORY_VIEW (dirview);

   switch (event_type) {
       case G_FILE_MONITOR_EVENT_RENAMED :
           myfm_directory_view_on_file_renamed (self, file, other_file);
           break;

       case G_FILE_MONITOR_EVENT_MOVED_OUT :
       case G_FILE_MONITOR_EVENT_DELETED :
           myfm_directory_view_on_file_moved_out (self, file);
           break;

       case G_FILE_MONITOR_EVENT_MOVED_IN :
       case G_FILE_MONITOR_EVENT_CREATED :
           myfm_directory_view_on_file_moved_in (self, file);
           break;
       case G_FILE_MONITOR_EVENT_CHANGED :
       case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED :
           puts ("changed");
           // TODO: fill in this
           break;
   }
}

static void myfm_directory_view_on_file_renamed (MyFMDirectoryView *self, GFile *orig_g_file, GFile *new_g_file)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gpointer myfm_file;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {

        do {
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &myfm_file, -1); /* out myfm_file */

            if (myfm_file != NULL && g_file_equal (((MyFMFile*) myfm_file)->g_file, orig_g_file)) {

                MyFMFile *new_myfm_file = myfm_file_new_without_io_fields (new_g_file);
                myfm_file_init_io_fields_async (new_myfm_file);

                // FIXME: ideally we should just have a myfm_file_refresh func that does all this
                new_myfm_file->is_open_dir = ((MyFMFile*) myfm_file)->is_open_dir;

                g_object_ref (new_g_file); // keep the g_file alive beyond the scope of on_dir_change (parent function)
                gtk_list_store_set (store, &iter, 0, new_myfm_file, -1); // FIXME: "promise" to set the file once it has been initialized async

                /* if the file is an opened directory we make sure to also update/replace the file
                 * held by the directory view as well as refresh the files it contains */
                if (((MyFMFile*) myfm_file)->is_open_dir) {

                    MyFMWindow *parent_win = MYFM_WINDOW(gtk_widget_get_toplevel (GTK_WIDGET (self)));
                    MyFMDirectoryView *next = myfm_window_get_next_directory_view (parent_win, self);

                    myfm_directory_view_replace_directory (next, new_myfm_file);
                    myfm_directory_view_refresh_all_async (next);
                }

                myfm_file_unref (myfm_file);
            }
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
    }

    g_debug ("renamed !!!!!!!!!! \n\n");
    puts ("renamed !!!!!!!!!! \n\n");
}

static void myfm_directory_view_on_file_moved_out (MyFMDirectoryView *self, GFile *orig_g_file)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gpointer myfm_file;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {

        do {
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &myfm_file, -1); /* out myfm_file */

            if (myfm_file != NULL && g_file_equal (((MyFMFile *) myfm_file)->g_file, orig_g_file)) {

                /* if the file is an opened directory, close it */
                if (((MyFMFile*) myfm_file)->is_open_dir) {

                    MyFMWindow *parent_win = MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self)));
                    MyFMDirectoryView *next = myfm_window_get_next_directory_view (parent_win, self);

                    myfm_window_close_directory_view (parent_win, next);
                }

                myfm_file_unref (myfm_file);
                gtk_list_store_remove (store, &iter);
            }
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));

    }

    g_debug ("moved out !!!!!!!!!! \n\n");
    puts ("moved out !!!!!!!!!! \n\n");
}

static void myfm_directory_view_on_file_moved_in (MyFMDirectoryView *self, GFile *new_g_file)
{
    GtkListStore *store;
    MyFMFile *myfm_file;
    MyFMFile *new_myfm_file;
    GtkTreeIter iter;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &myfm_file, -1); /* out myfm_file */
            /* if the moved in file already exists in the directory it is being moved to
             * (sometimes it might, for whatever reason) we exit the function */
            if (myfm_file != NULL && g_file_equal (((MyFMFile*) myfm_file)->g_file, new_g_file))
                return;
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
    }
    new_myfm_file = myfm_file_new_without_io_fields (new_g_file);
    myfm_file_init_io_fields_async (new_myfm_file);

    g_object_ref (new_g_file);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, new_myfm_file, -1);

    // TODO: ENABLE DEBUG MESSAGES
    g_debug ("moved in !!!!!!!!!! \n\n");
    puts ("moved in !!!!!!!!!! \n\n");
}

/* GtkCellRenderer data function for getting the display name of a file in a cell */
static void myfm_filename_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                     GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (tree_model, iter, 0, &myfm_file, -1); /* out myfm_file, pass by value */

    if (myfm_file && ((MyFMFile*) myfm_file)->IO_display_name)
        g_object_set (cell, "text", ((MyFMFile*) myfm_file)->IO_display_name, NULL);
}

/* foreach function to be called on each file in the directory view's
 * list store. unrefs and sets to null a single file in the store */
static gboolean clear_file_in_store (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    gpointer myfm_file;

    if (!model)
        return TRUE; /* stop iterating */

    gtk_tree_model_get (model, iter, 0, &myfm_file, -1);
    if (myfm_file) {
        // myfm_file_free ((MyFMFile *) myfm_file);
        myfm_file_unref ((MyFMFile*) myfm_file);
        gtk_list_store_set (GTK_LIST_STORE (model), iter, 0, NULL, -1);
    }

    return FALSE;
}

static void myfm_directory_view_setup_store (MyFMDirectoryView *self)
{
    GtkListStore *store;

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), GTK_TREE_MODEL (store));
    g_object_unref (store);
}

static void myfm_directory_view_setup_files_column (MyFMDirectoryView *self)
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

static void myfm_directory_view_setup_monitor (MyFMDirectoryView *self)
{
    GError_autoptr error = NULL; /* auto */

    self->directory_monitor = g_file_monitor_directory (self->directory->g_file,
                                                        G_FILE_MONITOR_WATCH_MOVES, NULL, &error);

    if (error) {
        g_object_unref (self->directory_monitor);
        g_critical ("unable to initialize directory monitor on directory "
                   "view: %s \n", error->message);
        return;
    }

    g_signal_connect (self->directory_monitor, "changed",
                      G_CALLBACK (myfm_directory_view_on_dir_change), self);
}

/* used when self->directory has become outdated and needs to be replaced (after a rename, for example) */
void myfm_directory_view_replace_directory (MyFMDirectoryView *self, MyFMFile *new_dir)
{
    /* replace directory and directory monitor */
    myfm_file_unref (self->directory);
    g_object_unref (self->directory_monitor);
    myfm_file_ref (new_dir);
    self->directory = new_dir;
    myfm_directory_view_setup_monitor (self);
}

void myfm_directory_view_refresh_all_async (MyFMDirectoryView *self)
{
    GtkTreeModel *store;

    /* clear store and refill it with files */
    store = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));
    gtk_tree_model_foreach (store, clear_file_in_store, NULL);
    gtk_list_store_clear (GTK_LIST_STORE (store));
    myfm_directory_view_fill_store_async (self);
}

/* ------------------------------------------------------------------------------------------ *
 *  CALLBACKS START
 * ------------------------------------------------------------------------------------------ */
static gint n_files = 32; // number of files to request per g_file_enumerator iteration

/* https://stackoverflow.com/questions/35036909/c-glib-gio-how-to-list-files-asynchronously
 * function calls itself until all files have been listed */
static void next_files_callback (GObject *file_enumerator, GAsyncResult *result, gpointer self)
{
    GError_autoptr error = NULL; /* auto_ptr or not? at least we avoid g_error_free everywhere */
    GList *directory_list;

    directory_list = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (file_enumerator),
                                                          result, &error);
    if (error) {
        /* if IO was cancelled due to the directory view being destroyed, if
         * the file no longer exists, etc. */
        g_critical ("unable to add files to list: %s \n", error->message);
        g_object_unref (file_enumerator);
        return;
    }
    else if (directory_list == NULL) {
        /* done listing, nothing left to add to store */
        g_object_unref (file_enumerator);
        /* emit "filled" signal when done */
        g_signal_emit_by_name (MYFM_DIRECTORY_VIEW (self), "filled");
        return;
    }
    else {
        /* enumerator returned successfully */
        GtkTreeIter iter;
        GFileInfo *child_info; /* doesn't need autoptr */
        GList *current_node = directory_list;
        GFile *parent_dir = MYFM_DIRECTORY_VIEW (self)->directory->g_file;
        GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

        /* iterate over directory_list and add files to store */
        while (current_node) {

            child_info = current_node->data;
            const char *child_name = g_file_info_get_display_name (child_info); // TODO: no free
            GFile *child_g_file = g_file_get_child_for_display_name (parent_dir, child_name, &error);

            if (error) {
                g_object_unref (child_info);
                g_object_unref (child_g_file);
                g_warning ("Unable to create g_file in directory: %s \n", error->message);
                g_error_free (error);
                error = NULL; /* TODO: KEEP TABS */
            }
            else {
                MyFMFile *child_myfm_file = myfm_file_new_with_info (child_g_file, g_strdup (child_name));
                gtk_list_store_append (store, &iter); /* out iter */
                gtk_list_store_set (store, &iter, 0, (gpointer) child_myfm_file, -1);
                g_object_unref (child_info);
            }
            current_node = current_node->next;
        }
        /* continue calling next_files and next_files_callback recursively */
        g_file_enumerator_next_files_async (G_FILE_ENUMERATOR (file_enumerator), n_files,
                                            G_PRIORITY_HIGH, MYFM_DIRECTORY_VIEW (self)->IO_canceller,
                                            next_files_callback, self);
    }
    g_list_free (directory_list);
}

static void enum_finished_callback (GObject *directory, GAsyncResult *result, gpointer self)
{
    GFileEnumerator_autoptr file_enumerator = NULL;
    GError_autoptr error = NULL;

    file_enumerator = g_file_enumerate_children_finish (G_FILE (directory), result, &error);

    if (error) {
        g_critical ("error in enum_finished_callback: %s \n", error->message);
        return;
    }
    else {
        g_file_enumerator_next_files_async (file_enumerator, n_files, G_PRIORITY_HIGH,
                                            MYFM_DIRECTORY_VIEW (self)->IO_canceller,
                                            next_files_callback, self);
        file_enumerator = NULL; /* set local pointer to NULL to prevent auto_unref before callback is invoked */
    }
}
/* ------------------------------------------------------------------------------------------ *
 *  CALLBACKS END
 * ------------------------------------------------------------------------------------------ */


/* adds the files in the directory view's directory to its list store. sets
 * off a chain of crazy async functions and callbacks (above). also emits the
 * "filled" signal when done. order of execution is:
 * myfm_directory_view_fill_store_async --> g_file_enumerate_children_async
 * --> enum_files_callback --> g_file_enumerator_next_files_async
 * --> next_files_callback. welcome to callback hell. */
void myfm_directory_view_fill_store_async (MyFMDirectoryView *self)
{
    g_file_enumerate_children_async (self->directory->g_file, "*",
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     G_PRIORITY_HIGH, self->IO_canceller,
                                     enum_finished_callback,
                                     self);
}

static void myfm_directory_view_destroy (GtkWidget *widget)
{
    GtkTreeModel *store;
    MyFMDirectoryView *self;

    g_debug ("destroying directory view \n");

    self = MYFM_DIRECTORY_VIEW (widget);
    store = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));

    if (store)
        gtk_tree_model_foreach (store, clear_file_in_store, NULL);

    if (self->directory) {
        self->directory->is_open_dir = FALSE;
        myfm_file_unref (self->directory);
        self->directory = NULL;
    }

    if (self->directory_monitor) {
        g_object_unref (self->directory_monitor);
        self->directory_monitor = NULL;
    }

    /* cancel all IO on the directory view */
    if (self->IO_canceller) {
        g_cancellable_cancel (self->IO_canceller);
        g_object_unref (self->IO_canceller);
        self->IO_canceller = NULL;
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
    myfm_directory_view_setup_files_column (self);

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
    gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (self), TRUE);
    gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (self), TRUE); // TODO: allow configurable

    g_signal_connect (self, "row-activated", G_CALLBACK (on_file_select), NULL);
}

static void myfm_directory_view_init (MyFMDirectoryView *self)
{
    self->IO_canceller = g_cancellable_new ();
}

static void myfm_directory_view_class_init (MyFMDirectoryViewClass *cls)
{
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);
    GtkWidgetClass *widget_cls = GTK_WIDGET_CLASS (cls);

    object_cls->dispose = myfm_directory_view_dispose;
    object_cls->finalize = myfm_directory_view_finalize;
    object_cls->constructed = myfm_directory_view_constructed;

    widget_cls->destroy = myfm_directory_view_destroy;

    /* signal to emit when all files in dir are added (async) to store. */
    g_signal_new ("filled", MYFM_DIRECTORY_VIEW_TYPE,
                  G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
                  NULL, G_TYPE_NONE, 0, NULL);
}

/* creates an empty directory view. call "myfm_directory_view_fill_store_async" to fill it */
// TODO: avoid construct-only properties. ideally, this func should only contain g_obj_new
//  and nothing else, as this helps with potential language bindings (extending with vala??)
MyFMDirectoryView *myfm_directory_view_new (MyFMFile *directory)
{
    MyFMDirectoryView *dirview;

    dirview = g_object_new (MYFM_DIRECTORY_VIEW_TYPE, NULL);
    dirview->directory = directory;
    myfm_file_ref (directory);
    myfm_directory_view_setup_monitor (dirview);

    return dirview;
}
