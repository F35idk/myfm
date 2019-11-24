/*
 * Created by f35 on 15.08.19.
*/

#include <gtk/gtk.h>
#include <glib.h>

#include "myfm-application.h"
#include "myfm-window.h"
#include "myfm-directory-view-utils.h"
#include "myfm-file.h"
#include "myfm-directory-view.h"
#include "myfm-multi-paned.h"

struct _MyFMWindow {
    GtkApplicationWindow parent_instance;

    /* ordered list of directory views (columns in the file manager) */
    GList *directory_views;

    /* widget containing each directory view that allows them to be resized freely */
    MyFMMultiPaned *mpaned;

    /* parent of mpaned that allows horizontal scrolling */
    GtkScrolledWindow *pane_scroll;

    /* FIXME: these could be static vars instead */
    gint default_height;
    gint default_width;
};

G_DEFINE_TYPE (MyFMWindow, myfm_window, GTK_TYPE_APPLICATION_WINDOW)

/* TODO: REPLACE PRINTFS WITH G_DEBUGS! */
static void mpaned_scroll_left_callback (MyFMMultiPaned *mpaned, gdouble scroll_dest, gpointer pane_scroll)
{
    gboolean return_val;
    GtkAdjustment *adj;
    gdouble current_val;
    gdouble step_incr;
    gdouble page_size;

    adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pane_scroll));
    page_size = gtk_adjustment_get_page_size (adj);
    current_val = gtk_adjustment_get_value (adj);

    if (scroll_dest - page_size < 0)
        step_incr = current_val;
    else
        step_incr = current_val - scroll_dest + page_size;

    gtk_adjustment_set_step_increment (adj, step_incr);
    g_signal_emit_by_name (GTK_SCROLLED_WINDOW (pane_scroll), "scroll-child", GTK_SCROLL_STEP_LEFT, TRUE, &return_val);
}

static void mpaned_scroll_to_end_callback (MyFMMultiPaned *mpaned, gpointer pane_scroll)
{
    gboolean return_val;

    g_signal_emit_by_name (GTK_SCROLLED_WINDOW (pane_scroll), "scroll-child", GTK_SCROLL_END, TRUE, &return_val);
}

static void show_dirview_callback (gpointer dirview)
{
    gtk_widget_show (GTK_WIDGET (dirview));
}

/* function for opening directories */
static void myfm_window_open_dir_async (MyFMWindow *self, MyFMFile *dir, gint dirview_index)
{
    MyFMDirectoryView *dirview;
    GtkScrolledWindow *dirview_scroll;

    /* FIXME: only if the opening succeeds? it kind of has to if this function is even called though */
    myfm_file_set_is_open (dir, TRUE);

    /* create new directory view and fill it with files */
    dirview = myfm_directory_view_new (dir);
    myfm_directory_view_fill_store_async (dirview);

    /* "promise" to show our directory view once it has been filled */
    g_signal_connect (dirview, "filled", G_CALLBACK (show_dirview_callback), NULL);

    /* remove unused directory views from our ordered directory view list (truncate it) */
    GList *element = g_list_nth (self->directory_views, dirview_index+1);
    while (element != NULL) {
        GList *next = element->next; /* store pointer to next before it changes */
        self->directory_views = g_list_remove (self->directory_views, element->data);
        element = next;
    }

    /* truncate pane widget as well (function returns if no directory views can be removed) */
    myfm_multi_paned_truncate_by_index (self->mpaned, dirview_index + 1);

    /* add new directory view to our list */
    self->directory_views = g_list_append (self->directory_views, dirview);

    /* make our directory view scrollable and add it to our pane widget */
    dirview_scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
    gtk_container_add (GTK_CONTAINER (dirview_scroll), GTK_WIDGET (dirview));
    myfm_multi_paned_add (self->mpaned, GTK_WIDGET (dirview_scroll));
    myfm_multi_paned_update_size (self->mpaned);
    gtk_widget_show (GTK_WIDGET (dirview_scroll));
}

/* function for opening any file that is not a directory */
/* FIXME: not async */
static void myfm_window_open_other_async (MyFMWindow *self, MyFMFile *file)
{
    /* TODO: fill in this */
}

/* main function for opening files. gets connected to the "activate" signal of the "open_file" g_action.
 * to invoke, set the action arguments with 'myfm_application_set_action_args' and call
 *'g_action_group_activate_action (win, "open_file", NULL)' where win = a myfm_window.  */
static void myfm_window_open_file_async (GSimpleAction *action, GVariant *param, gpointer window)
{
    MyFMApplication *app;
    struct MyFMOpenFileArgs *open_file_args;

    app = MYFM_APPLICATION (gtk_window_get_application (GTK_WINDOW (window)));
    open_file_args = myfm_application_get_action_args (app, MYFM_OPEN_FILE_ACTION);

    if (myfm_file_get_filetype (open_file_args->target_file) != G_FILE_TYPE_DIRECTORY)
        myfm_window_open_other_async (window, open_file_args->target_file);
    else
        myfm_window_open_dir_async (window, open_file_args->target_file,
                                    open_file_args->dirview_index);

    /* clear args struct afterwards */
    open_file_args->target_file = NULL;
}

/* function for closing any open directory view (and thus all directory views to the right of it in
 * the window as well). make sure to pass valid directory views into this - the function itself does no checking */
void myfm_window_close_directory_view (MyFMWindow *self, MyFMDirectoryView *dirview)
{
    gint index;
    GList *element;

    index = myfm_window_get_directory_view_index (self, dirview); /* get index before view is removed */
    element = g_list_find (self->directory_views, dirview);
    while (element != NULL) {
        GList *next = element->next; /* store pointer to next before it changes */
        MyFMDirectoryView *current_dirview = MYFM_DIRECTORY_VIEW (element->data);
        MyFMFile *dir = myfm_directory_view_get_directory (current_dirview);
        /* set open_dir to false to prevent recursive closes on already removed directories */
        myfm_file_set_is_open (dir, FALSE);
        self->directory_views = g_list_remove (self->directory_views, element->data);
        element = next;
    }

    myfm_multi_paned_truncate_by_index (self->mpaned, index);
    myfm_multi_paned_update_size (self->mpaned);
}

MyFMDirectoryView *myfm_window_get_next_directory_view (MyFMWindow *self, MyFMDirectoryView *dirview)
{
    GList *element = g_list_find (self->directory_views, dirview);
    if (element->next)
        return element->next->data;
    else
        return NULL;
}

gint myfm_window_get_directory_view_index (MyFMWindow *self, MyFMDirectoryView *dirview)
{
    return g_list_index (self->directory_views, dirview);
}

/* set up g_actions */
static void myfm_window_setup_actions (MyFMWindow *self)
{
    GSimpleAction *open_file_action;

    open_file_action = g_simple_action_new ("open_file", NULL);
    g_signal_connect (open_file_action, "activate", G_CALLBACK (myfm_window_open_file_async), self);
    /* ... */

    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (open_file_action));
}

static void myfm_window_destroy (GtkWidget *widget)
{
    /* TODO: what to put here? finalize is always run anyway so */

    GTK_WIDGET_CLASS (myfm_window_parent_class)->destroy (widget);
}

static void myfm_window_dispose (GObject *object)
{
    /* chaining up */
    G_OBJECT_CLASS (myfm_window_parent_class)->dispose (object);
}

static void myfm_window_finalize (GObject *object)
{
    MyFMWindow *self;

    g_debug ("finalizing window");

    self = MYFM_WINDOW (object);

    if (self->directory_views) {
        g_list_free (self->directory_views);
        self->directory_views = NULL;
    }

    G_OBJECT_CLASS (myfm_window_parent_class)->finalize (object);
}

static void myfm_window_constructed (GObject *object)
{
    MyFMWindow *self;

    G_OBJECT_CLASS (myfm_window_parent_class)->constructed (object);

    self = MYFM_WINDOW (object);

    myfm_window_setup_actions (self);
    gtk_window_set_default_size (GTK_WINDOW (self), self->default_width, self->default_height);

    /* hide scrollbar */
    /* TODO: make configurable */
    gtk_scrolled_window_set_policy (self->pane_scroll, GTK_POLICY_ALWAYS, GTK_POLICY_AUTOMATIC);
    GtkWidget *scrollbar = gtk_scrolled_window_get_hscrollbar (self->pane_scroll);
    gtk_widget_hide (scrollbar);

    gtk_container_add (GTK_CONTAINER (self->pane_scroll), GTK_WIDGET (self->mpaned));
    gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->pane_scroll));

    /* TODO: wrap text */
    g_signal_connect (self->mpaned, "shrink", G_CALLBACK (mpaned_scroll_left_callback), self->pane_scroll);
    g_signal_connect_after (self->mpaned, "expand", G_CALLBACK (mpaned_scroll_to_end_callback), self->pane_scroll);

    gtk_widget_show (GTK_WIDGET (self->pane_scroll));
    gtk_widget_show (GTK_WIDGET (self->mpaned));
}

static void myfm_window_init (MyFMWindow *self)
{
    self->default_height = 550;
    self->default_width = 890;
    self->directory_views = NULL;
    self->mpaned = myfm_multi_paned_new ();
    self->pane_scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
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
    return g_object_new (MYFM_TYPE_WINDOW, "application", app, NULL);
}
