//
// Created by f35 on 21.08.19.
//

#include <glib.h>
#include <gio/gio.h>

#include "myfm-file.h"

static void on_file_change (GFileMonitor *monitor, GFile *file, GFile *other_file,
                                GFileMonitorEvent event_type, gpointer user_data)
{
/*
 *  *   if (one flag)
 *          one func
 *      else if (other flag)
 *          other func
 *      etc.
 *      etc.
 * */
}

/* function for setting up a g_file_monitor on a myfm_file->g_file */
static void init_file_monitor (MyFMFile *myfm_file, GFile *g_file)
{
    /*
    GFileMonitor *monitor;
    GError_autoptr error = NULL;

    if (myfm_file->is_directory) {
        monitor = g_file_monitor_directory (g_file, G_FILE_MONITOR_WATCH_MOVES, NULL, &error);
    }
    else {
        monitor = g_file_monitor_file (g_file, G_FILE_MONITOR_NONE, NULL, &error);
    }

    if (error) {
        free stuff  }
    else {
        // myfm_file->g_file_monitor = monitor;
    }
    */
}

static void io_fields_callback (GObject *g_file, GAsyncResult *res, gpointer myfm_file)
{
    GFileInfo_autoptr info;
    GError_autoptr error = NULL;

    info = g_file_query_info_finish (G_FILE (g_file), res, &error);

    if (error) {
        g_object_unref (info);
        myfm_file_free (myfm_file);
        /* TODO: warn */
    }
    else {
        ((MyFMFile*) myfm_file)->IO_display_name = g_strdup (g_file_info_get_display_name (info));
    }
}

static void init_fields_without_io (MyFMFile *myfm_file, GFile *g_file)
{
    GFileType filetype;

    myfm_file->g_file = g_file;
    myfm_file->is_open = FALSE; // TODO: thonk
    // myfm_file->g_file_monitor = NULL; //

    filetype = g_file_query_file_type (G_FILE (g_file), G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    myfm_file->filetype = filetype;
}

/* despite the function being async, only the fields IO_display_name and (more???)
 * require asynchronous IO to be initialized. thus, the other fields can be accessed
 * safely instantly after this function is called. */
void myfm_file_from_g_file_async (MyFMFile *myfm_file, GFile *g_file)
{
    init_fields_without_io (myfm_file, g_file);

    /* set fields that require async IO to NULL before we initialize them */
    myfm_file->IO_display_name = NULL;

    g_file_query_info_async (g_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, G_PRIORITY_DEFAULT,
                            NULL, io_fields_callback, myfm_file);
}

MyFMFile *myfm_file_new_without_io (GFile *g_file, const char* display_name)
{
    MyFMFile *myfm_file;

    myfm_file = malloc (sizeof (MyFMFile));

    if (myfm_file == NULL) {
        g_critical ("malloc returned NULL, unable to create myfm_file");
        g_object_unref (g_file);
        g_free (display_name);
        return myfm_file;
    }

    init_fields_without_io (myfm_file, g_file);
    myfm_file->IO_display_name = display_name;

    return myfm_file;
}

void myfm_file_from_path_async (MyFMFile *myfm_file, const char *path)
{
    myfm_file_from_g_file_async (myfm_file, g_file_new_for_path (path));
}

void myfm_file_free (MyFMFile *myfm_file)
{

}