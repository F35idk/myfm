/*
 * Created by f35 on 26.12.19.
*/

#include <gio/gio.h>

#include "myfm-file.h"
#include "myfm-utils.h"
#include "myfm-file-operations.h"
#include "myfm-file-operations-private.h"
#define G_LOG_DOMAIN "myfm-file-operations-create"

static void
create_dir_callback (GObject* src_object,
                     GAsyncResult *res,
                     gpointer _cb)
{
    GError *error = NULL;
    MyFMFileOpCallback cb;
    GtkWindow *win;
    gpointer data;

    cb = _cb;
    win = g_object_get_data (src_object, "win");
    data = g_object_get_data (src_object, "user_data");

    g_file_make_directory_finish (G_FILE (src_object),
                                  res, &error);
    if (error) {
        g_critical ("error in myfm_file_operations_new functi"
                    "on 'create_dir_callback': %s", error->message);

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            GFile *new = myfm_utils_new_renamed_g_file (G_FILE (src_object));
            _create_file_async (G_FILE_TYPE_DIRECTORY, new, win, cb, data);
        }
        else {
            myfm_utils_run_error_dialog (win, "Error when creati"
                                         "ng directory", error->message);
        }
        g_error_free (error);
    }
    else if (cb) {
        cb (data);
    }

    g_object_unref (src_object);
}

static void
create_file_callback (GObject* src_object,
                      GAsyncResult *res,
                      gpointer _cb)
{
    GFileOutputStream *stream;
    MyFMFileOpCallback cb;
    GError *error = NULL;
    GtkWindow *win;
    gpointer data;

    cb = _cb;
    win = g_object_get_data (src_object, "win");
    data = g_object_get_data (src_object, "user_data");
    stream = g_file_create_finish (G_FILE (src_object),
                                   res, &error);
    if (error) {
        g_critical ("error in myfm_file_operations_new functi"
                    "on 'create_file_callback': %s", error->message);

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
            GFile *new = myfm_utils_new_renamed_g_file (G_FILE (src_object));
            _create_file_async (G_FILE_TYPE_REGULAR, new, win, cb, data);
        }
        else {
            myfm_utils_run_error_dialog (win, "Error when creati"
                                         "ng file", error->message);
        }
        g_error_free (error);
    }
    else if (cb) {
        cb (data);
    }

    g_object_unref (src_object);

    if (stream)
        g_object_unref (stream);
}

void
_create_file_async (GFileType type, GFile *new,
                    GtkWindow *active, gpointer cb,
                    gpointer data)
{
    g_object_set_data (G_OBJECT (new), "user_data", data);
    g_object_set_data (G_OBJECT (new), "win", active);

    if (type != G_FILE_TYPE_DIRECTORY) {
        g_file_create_async (new, G_FILE_CREATE_NONE,
                             G_PRIORITY_DEFAULT, NULL,
                             create_file_callback, cb);
    }
    else {
        g_file_make_directory_async (new, G_PRIORITY_DEFAULT,
                                     NULL, create_dir_callback, cb);
    }
}
