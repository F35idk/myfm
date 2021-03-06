/*
 * Created by f35 on 07.09.19.
*/

#include <gtk/gtk.h>

#include "myfm-icon-renderer.h"
#include "myfm-application.h"
#include "myfm-file-menu.h"
#include "myfm-directory-menu.h"
#include "myfm-file.h"
#include "myfm-window.h"
#include "myfm-directory-view.h"
#define G_LOG_DOMAIN "myfm-directory-view"

struct _MyFMDirectoryView {
    GtkTreeView parent_instance;
    MyFMFile *directory;
    GFileMonitor *directory_monitor;
    GCancellable *IO_canceller;
    gboolean show_hidden;
    MyFMSortCriteria file_sort_criteria;
};

G_DEFINE_TYPE (MyFMDirectoryView, myfm_directory_view, GTK_TYPE_TREE_VIEW)

static void
myfm_directory_view_on_hover (GtkWidget *widget, GdkEvent *event,
                              gpointer user_data)
{
    GtkTreePath *path = NULL;
    GtkTreeViewColumn *col = NULL;
    GdkEventMotion *motion;
    GtkTreeView *treeview;
    GdkWindow *bin_window;
    gint cell_x;
    gint cell_y;
    gboolean blank;

    treeview = GTK_TREE_VIEW (widget);
    bin_window = gtk_tree_view_get_bin_window (treeview);

    if (event->type == GDK_BUTTON_PRESS) {
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection (treeview);
        /* re-enable normal selection (this is disabled whenever
         * blank space is right-clicked, see on_rmb function) */
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    }
    if (event->type == GDK_MOTION_NOTIFY) {
        motion = (GdkEventMotion *) event;
        blank = gtk_tree_view_is_blank_at_pos (treeview, motion->x,
                                               motion->y, &path,
                                               &col, &cell_x,
                                               &cell_y);
        if (motion->window != bin_window)
            return;

        if (path)
            gtk_tree_path_free (path);

        if (!blank) {
            GdkDisplay *display;
            GdkCursor *pointer;

            display = gdk_window_get_display (bin_window);
            pointer = gdk_cursor_new_from_name (display, "pointer");

            gdk_window_set_cursor (bin_window, pointer);
            g_object_unref (pointer);
        }
        else {
            gdk_window_set_cursor (bin_window, NULL);
        }
    }
}

static void
myfm_directory_view_on_rmb (GtkWidget *widget, GdkEvent *event,
                            gpointer user_data)
{
    GdkWindow *bin_window;
    GdkEventButton *event_button;
    GtkTreeView *treeview;

    treeview = GTK_TREE_VIEW (widget);
    bin_window = gtk_tree_view_get_bin_window (treeview);
    event_button = (GdkEventButton *) event;

    if (event_button->button == 3 && event_button->window == bin_window) { /* rmb */
        GtkTreePath *path = NULL;
        GtkTreeViewColumn *col = NULL;
        gint cell_x;
        gint cell_y;
        gboolean blank;

        g_debug ("rmb");
        blank = gtk_tree_view_is_blank_at_pos (treeview, event_button->x,
                                               event_button->y, &path,
                                               &col, &cell_x, &cell_y);
        if (!blank) {
            if (path) { /* a file was right clicked */
                GtkWidget *menu;
                MyFMFile *selected;

                selected = myfm_directory_view_get_file_from_path (MYFM_DIRECTORY_VIEW (treeview),
                                                                   path);
                menu = myfm_file_menu_new (MYFM_DIRECTORY_VIEW (treeview), selected);
                gtk_menu_popup_at_pointer (GTK_MENU (menu), event);
                gtk_tree_path_free (path);
            }
        }
        else { /* the directory view was right clicked (no specific file) */
            GtkWidget *menu;
            GtkTreeSelection *selection;

            if (path)
                gtk_tree_path_free (path);

            selection = gtk_tree_view_get_selection (treeview);
            menu = myfm_directory_menu_new (MYFM_DIRECTORY_VIEW (treeview));

            /* prevent selecting rows */
            gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
            gtk_menu_popup_at_pointer (GTK_MENU (menu), event);
        }
    }
}

static void
myfm_directory_view_on_files_cut_uncut (MyFMClipBoard *cboard, gpointer files,
                                        gint n_files, gboolean cut, gpointer self)
{
    /* NOTE: anything else to do here? */
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
myfm_directory_view_on_file_select (GtkTreeView *treeview, GtkTreePath *path,
                                    GtkTreeViewColumn *column, gpointer user_data)
{
    MyFMWindow *parent_window;
    gint dirview_index;
    MyFMFile *myfm_file;

    parent_window = MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (treeview)));
    dirview_index = myfm_window_get_directory_view_index (parent_window,
                                                          MYFM_DIRECTORY_VIEW (treeview));
    myfm_file = myfm_directory_view_get_file_from_path (MYFM_DIRECTORY_VIEW (treeview), path);

    if (myfm_file)
        myfm_window_open_file_async (parent_window, (MyFMFile*) myfm_file, dirview_index);
}

static void
myfm_directory_view_update_file_sorting (MyFMDirectoryView *self)
{
    GtkTreeSortable *sortable;

    sortable = GTK_TREE_SORTABLE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    switch (self->file_sort_criteria) {
        case MYFM_SORT_NONE :
            return;
        case MYFM_SORT_NAME_A_TO_Z :
        case MYFM_SORT_NEWLY_EDITED_FIRST :
        case MYFM_SORT_LARGEST_FIRST :
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                                  GTK_SORT_ASCENDING);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_ASCENDING);
            break;
        case MYFM_SORT_NAME_Z_TO_A :
        case MYFM_SORT_NEWLY_EDITED_LAST :
        case MYFM_SORT_LARGEST_LAST :
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                                  GTK_SORT_DESCENDING);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_DESCENDING);
            break;
    }
}

/* currently just used for when a file has been renamed */
static void
myfm_directory_view_file_update_callback (MyFMFile *myfm_file,
                                          GFile *old_g_file,
                                          GError *error, gpointer self)
{
    if (error)
        g_error_free (error);
        /* NOTE: we don't do any special error handling here */

    /* redraw self. TODO: specify area? */
    gtk_widget_queue_draw (GTK_WIDGET (self));
    myfm_directory_view_update_file_sorting (self);

    /* if the file is open, close it */
    if (myfm_file_is_open (myfm_file)) {
        g_debug ("updated file is open");
        MyFMWindow *parent_win;
        MyFMDirectoryView *next;
        parent_win = MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self)));
        next = myfm_window_get_next_directory_view (parent_win, self);
        myfm_window_close_directory_view (parent_win, next);
    }
}

/* functions only used in on_dir_change. should maybe be inlined to avoid confusion */
static void myfm_directory_view_on_external_rename (MyFMDirectoryView *self,
                                                    GFile *orig_g_file, GFile *new_g_file);
static void myfm_directory_view_on_external_move_out (MyFMDirectoryView *self,
                                                      GFile *orig_g_file);
static void myfm_directory_view_on_external_move_in (MyFMDirectoryView *self,
                                                     GFile *new_g_file);

static void
myfm_directory_view_on_dir_change (GFileMonitor *monitor, GFile *file,
                                   GFile *other_file, GFileMonitorEvent event_type,
                                   gpointer dirview)
{
   MyFMDirectoryView *self = MYFM_DIRECTORY_VIEW (dirview);

   switch (event_type) {
       case G_FILE_MONITOR_EVENT_RENAMED :
           myfm_directory_view_on_external_rename (self, file, other_file);
           break;

       case G_FILE_MONITOR_EVENT_MOVED_OUT :
       case G_FILE_MONITOR_EVENT_DELETED :
           myfm_directory_view_on_external_move_out (self, file);
           break;

       case G_FILE_MONITOR_EVENT_MOVED_IN :
       case G_FILE_MONITOR_EVENT_CREATED :
           myfm_directory_view_on_external_move_in (self, file);
           break;

       /* case G_FILE_MONITOR_EVENT_CHANGED : */
       case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED :
           /* don't need to handle this, for now */
           g_debug ("file changed an attr."); /* g_file_get_path (file)); */
           break;
   }
}

/* updates files that have been renamed externally (outside of myfm) */
static void
myfm_directory_view_on_external_rename (MyFMDirectoryView *self,
                                        GFile *orig_g_file, GFile *new_g_file)
{
    GtkListStore *store;
    GtkTreeIter iter;
    MyFMFile *myfm_file;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &myfm_file, -1); /* out myfm_file */
            /* if the g_file in myfm_file matches 'new_g_file', the rename that took place was done
             * by myfm and thus did not happen externally, so we don't need to handle it here */
            if (myfm_file != NULL && g_file_equal (myfm_file_get_g_file (myfm_file),
                                                   new_g_file)) {
                return;
            }
            else if (myfm_file != NULL && g_file_equal (myfm_file_get_g_file (myfm_file),
                                                        orig_g_file)) {
                /* connect callback to redraw self once the display name has been updated */
                g_debug ("%s renamed to (something)", myfm_file_get_display_name (myfm_file));
                myfm_file_update_async (myfm_file, new_g_file,
                                        myfm_directory_view_file_update_callback, self);
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
    }
}

static void
myfm_directory_view_on_external_move_out (MyFMDirectoryView *self,
                                          GFile *orig_g_file)
{
    GtkListStore *store;
    GtkTreeIter iter;
    MyFMFile *myfm_file;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &myfm_file, -1); /* out myfm_file */

            if (myfm_file != NULL && g_file_equal (myfm_file_get_g_file (myfm_file),
                                                   orig_g_file)) {

                /* if the file is an opened directory, close it */
                if (myfm_file_is_open (myfm_file)) {
                    MyFMWindow *parent_win;
                    MyFMDirectoryView *next;

                    parent_win = MYFM_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self)));
                    next = myfm_window_get_next_directory_view (parent_win, self);
                    myfm_window_close_directory_view (parent_win, next);
                }

                g_debug ("%s moved out", myfm_file_get_display_name (myfm_file));

                myfm_file_unref (myfm_file);
                gtk_list_store_remove (store, &iter);
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
    }
}

static void
myfm_directory_view_file_to_store_callback (MyFMFile *myfm_file, GFile *old_g_file,
                                            GError *error, gpointer self)
{
    if (error) {
        /* NOTE: unref only in case of
         * G_IO_ERROR_NOT_FOUND? or for all errors? */
        myfm_file_unref (myfm_file);
        g_error_free (error);
        return;
    }

    GtkTreeIter iter;
    GtkListStore *store;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));
    /* FIXME: check if file hidden before appending */
    gtk_list_store_append (GTK_LIST_STORE (store), &iter);
    gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, myfm_file, -1);
}

static void
myfm_directory_view_on_external_move_in (MyFMDirectoryView *self, GFile *new_g_file)
{
    GtkListStore *store;
    MyFMFile *myfm_file;
    GtkTreeIter iter;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 0, &myfm_file, -1);
            /* if the moved in file already exists in the directory it is being moved to
             * (sometimes it might, for whatever reason) we exit the function */
            if (myfm_file != NULL && g_file_equal (myfm_file_get_g_file (myfm_file),
                                                   new_g_file))
                return;

        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
    }

    myfm_file_from_g_file_async (new_g_file,
                                 myfm_directory_view_file_to_store_callback, self);

    g_debug ("file moved in"); /* g_file_get_path (new_g_file)); */
}

/* GtkCellRenderer data function for getting the display name of a file in a cell */
static void
myfm_file_name_data_func (GtkTreeViewColumn *tree_column,
                          GtkCellRenderer *cell, GtkTreeModel *model,
                          GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (model, iter, 0, &myfm_file, -1);

    if (myfm_file)
        g_object_set (cell, "text", myfm_file_get_display_name (myfm_file), NULL);
}

static void
myfm_file_icon_data_func (GtkTreeViewColumn *tree_column,
                          GtkCellRenderer *cell, GtkTreeModel *model,
                          GtkTreeIter *iter, gpointer user_data)
{
    gpointer myfm_file;

    gtk_tree_model_get (model, iter, 0, &myfm_file, -1);

    if (myfm_file) {
        GIcon *icon;
        GtkIconSize icon_size;
        MyFMApplication *app;
        GtkWindow *win;

        win = GTK_WINDOW (gtk_widget_get_toplevel (gtk_tree_view_column_get_tree_view (tree_column)));
        app = MYFM_APPLICATION (gtk_window_get_application (win));
        icon_size = myfm_application_get_icon_size (app);
        icon = myfm_file_get_icon (myfm_file);
        MyFMClipBoard *cboard = myfm_application_get_file_clipboard (app);

        if (myfm_clipboard_file_is_cut (cboard, myfm_file))
            myfm_icon_renderer_set_icon_faded (MYFM_ICON_RENDERER (cell), TRUE);
        else
            myfm_icon_renderer_set_icon_faded (MYFM_ICON_RENDERER (cell), FALSE);

        g_return_if_fail (icon != NULL);

        g_object_set (cell, "gicon", icon, NULL);
        g_object_set (cell, "stock-size", icon_size, NULL);
    }
}

static void
myfm_directory_view_on_cell_edited (GtkCellRendererText *renderer, gchar *path,
                                    gchar *new_text, gpointer directory_view)
{
    MyFMDirectoryView *self;
    MyFMFile *selected_file;
    GtkTreePath *file_path;

    self = MYFM_DIRECTORY_VIEW (directory_view);
    file_path = gtk_tree_path_new_from_string (path);
    selected_file = myfm_directory_view_get_file_from_path (self, file_path);

    gtk_tree_path_free (file_path);
    /* NOTE: we may want to use a different handler than "file_update_callback"
     * for this function, as display name errors may require more specific handling */
    myfm_file_set_display_name_async (selected_file, new_text,
                                      myfm_directory_view_file_update_callback,
                                      self);
}

void
myfm_directory_view_start_rename_selected (MyFMDirectoryView *self, MyFMFile *file)
{
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *file_path;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
    gtk_tree_selection_get_selected (selection, &model, &iter);
    file_path = gtk_tree_model_get_path (model, &iter);
    col = gtk_tree_view_get_column (GTK_TREE_VIEW (self), 0);
    renderer = g_object_get_data (G_OBJECT (col), "renderer");
    /* set and unset editable - never allow editing unless start_rename is called */
    g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
    gtk_tree_view_set_cursor_on_cell (GTK_TREE_VIEW (self), file_path,
                                      col, NULL, TRUE);
    g_object_set (G_OBJECT (renderer), "editable", FALSE, NULL);
    gtk_tree_path_free (file_path);
}

static gint
sort_by_size_func (GtkTreeModel *sortable, GtkTreeIter *a,
                   GtkTreeIter *b, gpointer data)
{
    MyFMFile *file_a;
    MyFMFile *file_b;

    gtk_tree_model_get (sortable, a, 0, &file_a, -1);
    gtk_tree_model_get (sortable, b, 0, &file_b, -1);

    if (file_a && file_b) /* sometimes files are NULL when func is called */
        return myfm_file_get_size (file_a) - myfm_file_get_size (file_b);
    else
        return 0;
}

static gint
sort_by_date_func (GtkTreeModel *sortable, GtkTreeIter *a,
                   GtkTreeIter *b, gpointer data)
{
    MyFMFile *file_a;
    MyFMFile *file_b;

    gtk_tree_model_get (sortable, a, 0, &file_a, -1);
    gtk_tree_model_get (sortable, b, 0, &file_b, -1);

    if (file_a && file_b) /* sometimes files are NULL when func is called */
        return g_date_time_compare (myfm_file_get_modification_time (file_b),
                                    myfm_file_get_modification_time (file_a));
    else
        return 0;
}

static gint
sort_by_name_func (GtkTreeModel *sortable, GtkTreeIter *a,
                   GtkTreeIter *b, gpointer data)
{
    MyFMFile *file_a;
    MyFMFile *file_b;

    gtk_tree_model_get (sortable, a, 0, &file_a, -1);
    gtk_tree_model_get (sortable, b, 0, &file_b, -1);

    if (file_a && file_b) /* sometimes files are NULL when func is called */
      return g_ascii_strcasecmp (myfm_file_get_display_name (file_a),
                                 myfm_file_get_display_name (file_b));
    else
        return 0;
}

void
myfm_directory_view_set_file_sort_criteria (MyFMDirectoryView *self,
                                            MyFMSortCriteria criteria)
{
    GtkTreeSortable *sortable;

    sortable = GTK_TREE_SORTABLE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));
    self->file_sort_criteria = criteria;

    switch (criteria) {
        case MYFM_SORT_NONE :
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                                  GTK_SORT_ASCENDING);
            break;
        case MYFM_SORT_NEWLY_EDITED_FIRST :
            gtk_tree_sortable_set_default_sort_func (sortable, sort_by_date_func,
                                                     NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_ASCENDING);
            break;
        case MYFM_SORT_NEWLY_EDITED_LAST :
            gtk_tree_sortable_set_default_sort_func (sortable, sort_by_date_func,
                                                     NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_DESCENDING);
            break;
        case MYFM_SORT_NAME_A_TO_Z :
            gtk_tree_sortable_set_default_sort_func (sortable, sort_by_name_func,
                                                     NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_ASCENDING);
            break;
        case MYFM_SORT_NAME_Z_TO_A :
            gtk_tree_sortable_set_default_sort_func (sortable, sort_by_name_func,
                                                     NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_DESCENDING);
            break;
        case MYFM_SORT_LARGEST_FIRST :
            gtk_tree_sortable_set_default_sort_func (sortable, sort_by_size_func,
                                                     NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_DESCENDING);
            break;
        case MYFM_SORT_LARGEST_LAST :
            gtk_tree_sortable_set_default_sort_func (sortable, sort_by_size_func,
                                                     NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (sortable,
                                                  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                                  GTK_SORT_ASCENDING);
            break;
    }
}

/* sets whether to display hidden files (dotfiles) */
void
myfm_directory_view_set_show_hidden (MyFMDirectoryView *self,
                                     gboolean show_hidden)
{
    self->show_hidden = show_hidden;
}

gboolean
myfm_directory_view_get_show_hidden (MyFMDirectoryView *self)
{
    return self->show_hidden;
}

MyFMFile *
myfm_directory_view_get_file_from_path (MyFMDirectoryView *self,
                                        GtkTreePath *path)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    MyFMFile *myfm_file;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 0, &myfm_file, -1);

    return myfm_file;
}

MyFMFile *
myfm_directory_view_get_directory (MyFMDirectoryView *self)
{
    return self->directory;
}

/* foreach function to be called on each file in the directory view's
 * list store. unrefs and sets to null a single file in the store */
static gboolean
clear_file_in_store (GtkTreeModel *model, GtkTreePath *path,
                     GtkTreeIter *iter, gpointer data)
{
    gpointer myfm_file;

    if (!model)
        return TRUE; /* stop iterating */

    gtk_tree_model_get (model, iter, 0, &myfm_file, -1);
    if (myfm_file) {
        myfm_file_unref (myfm_file);
        gtk_list_store_set (GTK_LIST_STORE (model), iter, 0, NULL, -1);
    }

    return FALSE;
}

static void
myfm_directory_view_setup_store (MyFMDirectoryView *self)
{
    GtkListStore *store;

    store = gtk_list_store_new (2,G_TYPE_POINTER, GDK_TYPE_PIXBUF);
    gtk_tree_view_set_model (GTK_TREE_VIEW (self), GTK_TREE_MODEL (store));
    g_object_unref (store);
}

static void
myfm_directory_view_setup_files_column (MyFMDirectoryView *self)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    col = gtk_tree_view_column_new ();

    renderer = myfm_icon_renderer_new ();
    gtk_tree_view_column_pack_start (col, renderer, FALSE);
    gtk_tree_view_column_set_attributes (col, renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (col, renderer,
                                             myfm_file_icon_data_func,
                                             NULL, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_renderer_set_padding (renderer, 4, 1);
    gtk_tree_view_column_pack_start (col, renderer, TRUE);
    gtk_tree_view_column_set_attributes (col, renderer, NULL);
    gtk_tree_view_column_set_cell_data_func (col, renderer,
                                             myfm_file_name_data_func,
                                             NULL, NULL);
    g_object_set_data (G_OBJECT (col), "renderer", renderer);
    g_signal_connect (GTK_CELL_RENDERER_TEXT (renderer), "edited",
                      G_CALLBACK (myfm_directory_view_on_cell_edited),
                      self);

    gtk_tree_view_column_set_resizable (col, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self), col);
}

static void
myfm_directory_view_setup_monitor (MyFMDirectoryView *self)
{
    GError *error = NULL;

    self->directory_monitor = g_file_monitor_directory (myfm_file_get_g_file (self->directory),
                                                        G_FILE_MONITOR_WATCH_MOVES,
                                                        NULL, &error);

    if (error) {
        g_object_unref (self->directory_monitor);
        g_critical ("unable to initialize directory monitor "
                    "on directory view: '%s'", error->message);
        g_error_free (error);
        return;
    }

    g_signal_connect (self->directory_monitor, "changed",
                      G_CALLBACK (myfm_directory_view_on_dir_change), self);
}

void
myfm_directory_view_refresh_files_async (MyFMDirectoryView *self)
{
    GtkTreeModel *store;

    /* clear store and refill it with files */
    store = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));
    gtk_tree_model_foreach (store, clear_file_in_store, NULL);
    gtk_list_store_clear (GTK_LIST_STORE (store));
    myfm_directory_view_fill_store_async (self);
}

/* ------------------------------------------------------------------------------------------ *
 *  ENUMERATOR CALLBACKS START
 * ------------------------------------------------------------------------------------------ */
/* TODO: investigate lowering this */
static const gint n_files = 32; /* number of files to request per g_file_enumerator iteration */

/* https://stackoverflow.com/questions/35036909/c-glib-gio-how-to-list-files-asynchronously
 * function calls itself until all files have been added to store */
static void
myfm_directory_view_next_files_callback (GObject *file_enumerator, GAsyncResult *result,
                                         gpointer self)
{
    GError *error = NULL;
    GList *directory_list;

    directory_list = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (file_enumerator),
                                                          result, &error);
    if (error) {
        /* if IO was cancelled due to the directory view
         * being destroyed, if the file no longer exists, etc. */
        g_critical ("unable to add files to list: '%s'", error->message);
        g_object_unref (file_enumerator);
        g_error_free (error);
        return;
    }
    else if (directory_list == NULL) {
        /* done listing, nothing left to add to store */
        g_object_unref (file_enumerator);
        /* emit "filled" signal */
        g_signal_emit_by_name (MYFM_DIRECTORY_VIEW (self), "filled");
        return;
    }
    else {
        /* enumerator returned successfully */
        GtkTreeIter iter;
        GFileInfo *child_info;
        GList *current_node = directory_list;
        GFile *parent_dir = myfm_file_get_g_file (MYFM_DIRECTORY_VIEW (self)->directory);
        GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (self)));

        while (current_node) {
            /* iterate over directory_list and add files to store */
            child_info = current_node->data;

            /* ignore hidden files */
            if (!MYFM_DIRECTORY_VIEW (self)->show_hidden) {
                if (child_info && g_file_info_get_is_hidden (child_info)) {
                    g_object_unref (child_info);
                    current_node = current_node->next;
                    continue;
                }
            }

            const char *child_name = g_file_info_get_display_name (child_info);
            GFile *child_g_file = g_file_get_child_for_display_name (parent_dir, child_name,
                                                                     &error);

            if (error) {
                g_object_unref (child_info);
                g_object_unref (child_g_file);
                g_warning ("Unable to create g_file in directory: %s \n",
                           error->message);
                g_error_free (error);
                error = NULL;
            }
            else {
                MyFMFile *child_myfm_file = myfm_file_new_with_info (child_g_file, child_info);
                /* myfm_file refs the g_file, so we make sure to unref it here */
                g_object_unref (child_g_file);
                gtk_list_store_append (store, &iter); /* out iter */
                gtk_list_store_set (store, &iter, 0, (gpointer) child_myfm_file, -1);
                g_object_unref (child_info);
            }
            current_node = current_node->next;
        }
        /* continue calling next_files and next_files_callback recursively */
        g_file_enumerator_next_files_async (G_FILE_ENUMERATOR (file_enumerator),
                                            n_files, G_PRIORITY_HIGH,
                                            MYFM_DIRECTORY_VIEW (self)->IO_canceller,
                                            myfm_directory_view_next_files_callback, self);
    }
    g_list_free (directory_list);
}

static void
myfm_directory_view_enum_finished_callback (GObject *directory, GAsyncResult *result,
                                            gpointer self)
{
    GFileEnumerator *file_enumerator;
    GError *error = NULL;

    file_enumerator = g_file_enumerate_children_finish (G_FILE (directory),
                                                        result, &error);

    if (error) {
        g_critical ("error in enum_finished_callback: '%s'",
                    error->message);
        g_error_free (error);
        return;
    }
    else {
        g_file_enumerator_next_files_async (file_enumerator, n_files, G_PRIORITY_HIGH,
                                            MYFM_DIRECTORY_VIEW (self)->IO_canceller,
                                            myfm_directory_view_next_files_callback, self);
    }
}
/* ------------------------------------------------------------------------------------------ *
 *  ENUMERATOR CALLBACKS END
 * ------------------------------------------------------------------------------------------ */


/* adds the files in the directory view's directory to its list store. sets
 * off a chain of crazy async functions and callbacks (above). emits the
 * "filled" signal when done. order of execution is:
 * myfm_directory_view_fill_store_async --> g_file_enumerate_children_async
 * --> enum_files_callback --> g_file_enumerator_next_files_async
 * --> next_files_callback. welcome to callback hell. */
void
myfm_directory_view_fill_store_async (MyFMDirectoryView *self)
{
    g_file_enumerate_children_async (myfm_file_get_g_file (self->directory),
                                     MYFM_FILE_QUERY_ATTRIBUTES,
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     G_PRIORITY_HIGH, self->IO_canceller,
                                     myfm_directory_view_enum_finished_callback,
                                     self);
}

static void
myfm_directory_view_destroy (GtkWidget *widget)
{
    GtkTreeModel *store;
    MyFMDirectoryView *self;

    g_debug ("destroying directory view");

    self = MYFM_DIRECTORY_VIEW (widget);
    store = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));

    if (store)
        gtk_tree_model_foreach (store, clear_file_in_store, NULL);

    if (self->directory) {
        myfm_file_set_is_open (self->directory, FALSE);
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

static void
myfm_directory_view_dispose (GObject *object)
{
    G_OBJECT_CLASS (myfm_directory_view_parent_class)->dispose (object);
}

static void
myfm_directory_view_finalize (GObject *object)
{
    G_OBJECT_CLASS (myfm_directory_view_parent_class)->finalize (object);
}

static void
myfm_directory_view_constructed (GObject *object)
{
    MyFMDirectoryView *self;

    G_OBJECT_CLASS (myfm_directory_view_parent_class)->constructed (object);

    self = MYFM_DIRECTORY_VIEW (object);

    myfm_directory_view_setup_store (self);
    myfm_directory_view_setup_files_column (self);

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
    gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (self), TRUE);
    /* TODO: allow configurable */
    gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (self), TRUE);
    /* enable receiving mouse events */
    gtk_widget_add_events (GTK_WIDGET (self), GDK_POINTER_MOTION_MASK |
                           GDK_BUTTON_RELEASE_MASK);

    g_signal_connect (GTK_WIDGET (self), "event",
                      G_CALLBACK (myfm_directory_view_on_hover),
                      NULL);
    g_signal_connect (self, "row-activated",
                      G_CALLBACK (myfm_directory_view_on_file_select),
                      NULL);
    g_signal_connect (GTK_WIDGET (self), "button-press-event",
                      G_CALLBACK (myfm_directory_view_on_rmb),
                      NULL);
}

static void
myfm_directory_view_init (MyFMDirectoryView *self)
{
    self->IO_canceller = g_cancellable_new ();
    self->show_hidden = FALSE; /* TODO: make configurable */
    self->file_sort_criteria = MYFM_SORT_NONE; /* TODO: make configurable */
}

static void
myfm_directory_view_class_init (MyFMDirectoryViewClass *cls)
{
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);
    GtkWidgetClass *widget_cls = GTK_WIDGET_CLASS (cls);

    object_cls->dispose = myfm_directory_view_dispose;
    object_cls->finalize = myfm_directory_view_finalize;
    object_cls->constructed = myfm_directory_view_constructed;

    widget_cls->destroy = myfm_directory_view_destroy;

    /* signal to emit when all files in dir are added (async) to store. */
    g_signal_new ("filled", MYFM_TYPE_DIRECTORY_VIEW,
                  G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
                  NULL, G_TYPE_NONE, 0, NULL);
}

/* creates an empty directory view. call "myfm_directory_view_fill_store_async"
 * to fill it */
/* TODO: avoid construct-only properties. ideally, this func
 * should only contain g_obj_new and nothing else, as this helps
 * with potential language bindings (extending with vala??) */
MyFMDirectoryView *
myfm_directory_view_new (MyFMFile *directory, MyFMApplication *app)
{
    MyFMDirectoryView *dirview;
    MyFMClipBoard *cboard;

    cboard = myfm_application_get_file_clipboard (app);
    dirview = g_object_new (MYFM_TYPE_DIRECTORY_VIEW, NULL);
    dirview->directory = directory;

    myfm_file_ref (directory);
    myfm_directory_view_setup_monitor (dirview);

    /* use connect_object to prevent use-after-free */
    g_signal_connect_object (cboard, "cut-uncut",
                             G_CALLBACK (myfm_directory_view_on_files_cut_uncut),
                             dirview, 0);

    return dirview;
}
