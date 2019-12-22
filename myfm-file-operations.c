/*
 * Created by f35 on 20.12.19.
*/

#include <stdarg.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-utils.h"
#define G_LOG_DOMAIN "myfm-file-operations"

/* given that only one copy operation can happen at once, it should be
 * fine to keep some of the important cross-callback data static like this.
 * rather this than spinning up an entire gobject for each copy operation */
static gint ongoing_count = 0;
static GCancellable *copy_canceller = NULL;
static MyFMWindow *win = NULL;
static MyFMApplication *app = NULL;
static gboolean overwrite = FALSE;
/* thread priority shouldn't be too high */
static gint io_pr = G_PRIORITY_LOW;
/* in the case of errors, multiple error dialogs
 * might pop up at once, covering the screen (due
 * to the async nature of our file copy operation).
 * to avoid this, we keep a queue of each popup
 * message and make sure to only activate the popups
 * one at a time (see queue_popup_error_dialog ()) */
static GQueue *dialog_msg_queue = NULL;
static gboolean dialog_active = FALSE;
static gboolean skip_all_popups = FALSE;

void
myfm_file_operations_cancel_current (void)
{
    if (!copy_canceller)
        return;

    g_cancellable_cancel (copy_canceller);
    g_object_unref (copy_canceller);
    copy_canceller = NULL;
}

static void
myfm_file_operations_copy_finish (void)
{
    if (copy_canceller) {
        g_object_unref (copy_canceller);
        copy_canceller = NULL;
    }
    // if (dialog_msg_queue) {
    //     g_queue_free_full (dialog_msg_queue,
    //                        g_free);
    //     dialog_msg_queue = NULL;
    // }

    myfm_application_set_copy_in_progress (app, FALSE);

    win = NULL;
    overwrite = FALSE;
    dialog_active = FALSE;
    skip_all_popups = FALSE;
    ongoing_count = 0;

    g_debug ("finished");
}

static void
on_error_dialog_response (GtkDialog *dialog, gint response_id,
                          gpointer data)
{
    if (response_id == 0) /* 'Cancel Operation' button */
        myfm_file_operations_cancel_current ();
    else if (response_id == 2) /* 'Ignore All' button */
        skip_all_popups = TRUE;
}

static void
queue_popup_error_dialog (const gchar *format_msg, ...)
{
    va_list va;
    gchar *msg;

    /* if operation is cancelled or win is NULL */
    if (!win || !copy_canceller || skip_all_popups)
        return;

    if (!dialog_msg_queue)
        dialog_msg_queue = g_queue_new ();

    va_start (va, format_msg);
    msg = g_strdup_vprintf (format_msg, va);
    va_end (va);

    g_queue_push_tail (dialog_msg_queue, msg);

    if (!dialog_active) {
        /* while operation not cancelled, queue not empty and skip_all false */
        while (!g_queue_is_empty (dialog_msg_queue)) {
            GtkWidget *error_dialog;
            gchar *next_msg;

            if (skip_all_popups) {
                g_queue_free_full (dialog_msg_queue,
                                   g_free);
                dialog_msg_queue = NULL;
                return;
            }

            dialog_active = TRUE;
            next_msg = g_queue_pop_head (dialog_msg_queue);
            error_dialog = gtk_message_dialog_new (GTK_WINDOW (win), GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
                                                   "%s", next_msg);
            gtk_window_set_title (GTK_WINDOW (error_dialog), "Error");
            gtk_dialog_add_buttons (GTK_DIALOG (error_dialog),
                                    "Cancel Operation", 0, "Ignore",
                                    1, "Ignore All", 2, NULL);
            g_free (next_msg);
            g_signal_connect (GTK_DIALOG (error_dialog), "response",
                              G_CALLBACK (on_error_dialog_response), NULL);
            gtk_dialog_run (GTK_DIALOG (error_dialog));
            gtk_widget_destroy (error_dialog);
            dialog_active = FALSE;
        }
    }
}

static void
g_file_copy_callback (GObject *g_file, GAsyncResult *res,
                      gpointer dest_file)
{
    GError *error = NULL;

    g_file_copy_finish (G_FILE (g_file), res, &error);
    ongoing_count --;

    if (error) {
        if (!overwrite && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            gchar *path;
            gchar *new_path;
            GFile *new_dest;

            path = g_file_get_path (dest_file);
            new_path = g_strconcat (path, " (copy)", NULL);
            new_dest = g_file_new_for_path (new_path);
            g_free (path);
            g_free (new_path);
            ongoing_count ++;
            g_file_copy_async (G_FILE (g_file), new_dest,
                               G_FILE_COPY_NOFOLLOW_SYMLINKS, /* TODO: more/other copy flags */
                               io_pr, copy_canceller,
                               NULL, NULL, g_file_copy_callback,
                               new_dest);
            /* keep parent alive */
            g_object_ref (g_file);
        }
        else {
            g_critical ("error in myfm_file_operations "
                        "function 'g_file_copy_callback': '%s'",
                        error->message);
            queue_popup_error_dialog ("Error while copying "
                                      "file: '%s' Ignore?",
                                      error->message);
        }
        g_error_free (error);
    }

    if (!ongoing_count)
        myfm_file_operations_copy_finish ();

    g_object_unref (dest_file);
    g_object_unref (g_file);
}

/* forward (backward?) declare so we can call
 * for_each recursively from make_dir_callback */
static void g_file_foreach_func (GFile *parent, GFileInfo *child_info,
                                 gboolean finished, GError *error,
                                 gpointer dest_dir_path);

static void
g_file_make_dir_callback (GObject *g_file, GAsyncResult *res,
                          gpointer source_file)
{
    GError *error = NULL;
    gchar *dir_path;

    g_file_make_directory_finish (G_FILE (g_file), res,
                                  &error);
    dir_path = g_file_get_path (G_FILE (g_file));
    g_object_unref (g_file);

    if (error) {
        if (!overwrite && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            gchar *new_path;
            GFile *new_dir;

            new_path = g_strconcat (dir_path, " (copy)", NULL);
            new_dir = g_file_new_for_path (new_path);

            g_file_make_directory_async (new_dir, io_pr,
                                         copy_canceller,
                                         g_file_make_dir_callback,
                                         source_file);
            g_free (new_path);
            g_free (dir_path);
            g_error_free (error);
            return;
        }
        else {
            g_critical ("error in myfm_file_operations "
                        "function 'g_file_make_dir_callback': '%s'",
                        error->message);
            queue_popup_error_dialog ("Error while copying "
                                      "file: '%s' Ignore?",
                                      error->message);
            g_error_free (error);
        }
    }

    myfm_utils_for_each_child (source_file, MYFM_FILE_QUERY_ATTRIBUTES,
                               copy_canceller, io_pr,
                               g_file_foreach_func, dir_path);
}

static void
g_file_foreach_func (GFile *parent, GFileInfo *child_info,
                     gboolean finished, GError *error,
                     gpointer dest_dir_path)
{
    const gchar *source_name;
    const gchar *source_basename;
    GFile *source;
    GFile *dest;
    GError *error2 = NULL;

    /* we decrement ongoing_count in this error block or
     * in the 'finished' block below. both mean we're done
     * iterating over the children in a directory, and
     * are counterpart to/match the incrementing we do before
     * calling g_file_make_directory_async */
    if (error) {
        ongoing_count --;
        if (!ongoing_count)
            myfm_file_operations_copy_finish ();

        g_critical (error->message);
        queue_popup_error_dialog ("error when getting "
                                  "children of directory in "
                                  "myfm_file_operations: %s",
                                  error->message);
        g_error_free (error);
        g_free (dest_dir_path);
        g_object_unref (parent);
        return;
    }
    if (finished) {
        /* done iterating over all (direct) children */
        ongoing_count --;
        if (!ongoing_count)
            myfm_file_operations_copy_finish ();

        g_free (dest_dir_path);
        g_object_unref (parent);
        return;
    }

    g_debug (g_file_info_get_display_name (child_info));
    source_name = g_file_info_get_display_name (child_info);
    source = g_file_get_child_for_display_name (parent, source_name,
                                                &error2);
    if (error2) {
        g_critical ("error in myfm_file_operations"
                    "function 'g_file_foreach_func': %s",
                    error2->message);
        queue_popup_error_dialog ("Error while copying "
                                  "file: '%s' Ignore?",
                                  error->message);
        g_error_free (error2);
        g_object_unref (child_info);
        return;
    }

    source_basename = g_file_get_basename (source);
    dest = g_file_new_build_filename (dest_dir_path, source_basename, NULL);
    g_free (source_basename);

    if (g_file_info_get_file_type (child_info) == G_FILE_TYPE_DIRECTORY) {
        /* make_dir_callback initiates recursion (it calls
         * myfm_utils_for_each_child with this function as
         * a callback). so we increment count here, then
         * decrement in the 'if (finished)' or 'if (error)'
         * blocks */
        ongoing_count ++;
        g_file_make_directory_async (dest, io_pr,
                                     copy_canceller,
                                     g_file_make_dir_callback,
                                     source);
    }
    else {
        /* increment before call to g_file_copy, then decrement in callback */
        ongoing_count ++;
        if (overwrite)
            g_file_copy_async (source, dest,
                               G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_OVERWRITE,
                               io_pr, copy_canceller,
                               NULL, NULL, g_file_copy_callback,
                               dest);
        else
            g_file_copy_async (source, dest, G_FILE_COPY_NOFOLLOW_SYMLINKS, /* TODO: more/other copy flags */
                               io_pr, copy_canceller,
                               NULL, NULL, g_file_copy_callback,
                               dest);
        /* NOTE: we keep dest alive until g_file_copy_callback */
    }
    g_object_unref (child_info);
}

/* copy operation for files in the file manager. initiates an
 * async, depth-first, recursive copy if the given file is a
 * directory. otherwise just calls g_file_copy_async. */
/* TODO: measure disk usage beforehand? */
/* TODO: check if would recurse? */
void
myfm_file_operations_copy_async (MyFMFile *file, gchar *dest_dir_path,
                                 MyFMWindow *active_win, gboolean overwrite_dest)
{
    GFile *source;

    /* we will be incrementing and decrementing
     * this as we go. when it is zero, we know
     * all async copy operations are finished,
     * and the recursive copy is done */
    ongoing_count ++;
    /* active window is passed so error
     * dialogs know where to pop up */
    win = active_win;
    copy_canceller = g_cancellable_new ();

    if (overwrite_dest)
        overwrite = TRUE;

    app = MYFM_APPLICATION (gtk_window_get_application (GTK_WINDOW (win)));
    myfm_application_set_copy_in_progress (app, TRUE);

    source = myfm_file_get_g_file (file);
    g_object_ref (source);

    if (myfm_file_get_filetype (file) != G_FILE_TYPE_DIRECTORY) {
        GFile *dest = g_file_new_build_filename (dest_dir_path,
                                                 g_file_get_basename (source),
                                                 NULL);
        g_file_copy_async (source, dest, G_FILE_COPY_NONE,
                           io_pr, copy_canceller,
                           NULL, NULL, g_file_copy_callback,
                           NULL);
    }
    else {
        myfm_utils_for_each_child (source,
                                   MYFM_FILE_QUERY_ATTRIBUTES, copy_canceller,
                                   io_pr, g_file_foreach_func,
                                   g_strdup (dest_dir_path));
    }
}
