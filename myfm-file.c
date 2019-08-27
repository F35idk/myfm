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
// TODO: put in myfm_file_initialize func or something
static void myfm_file_init_monitor (MyFMFile *myfm_file, GFile *g_file)
{
    GFileMonitor *monitor;
    GError *error = NULL;

    if (myfm_file->is_directory) {
        monitor = g_file_monitor_directory (g_file, G_FILE_MONITOR_WATCH_MOVES, NULL, &error);
    }
    else {
        monitor = g_file_monitor_file (g_file, G_FILE_MONITOR_NONE, NULL, &error);
    }

    if (error) {
     /* do something */  }
    else {
        myfm_file->g_file_monitor = monitor;
        // TODO: signalsssssssssssssssssssssssssss
    }

}

static void myfm_file_from_g_file_callback (GObject *g_file, GAsyncResult *res, gpointer myfm_file)
{
    GFileInfo *info;
    GError *error = NULL;

    info = g_file_query_info_finish (G_FILE (g_file), res, &error);

    if (error) {
        /* do something */  }
    else {
        ((MyFMFile*) myfm_file)->g_file_info = info;
        ((MyFMFile*) myfm_file)->display_name = g_file_info_get_display_name (info);

        // TODO: EMIT SOME SIGNAL TO INFORM THAT FILE IS INITIALIZED AND CAN BE TOUCHED

    }
}

/* despite the function being async, only the fields g_file_info and display_name
 * require asynchronous IO to be initialized. the other fields can thus be accessed
 * safely instantly after this function is called. */
void myfm_file_from_g_file_async (MyFMFile *myfm_file, GFile *g_file)
{
    GFileType filetype;

    myfm_file->g_file = g_file;
    myfm_file->is_open = FALSE; // TODO: thonk
    myfm_file->g_file_monitor = NULL; // TODO: get a g_file_monitor up and going, signals connected and all

    filetype = g_file_query_file_type (G_FILE (g_file), G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (filetype == G_FILE_TYPE_DIRECTORY)
        ((MyFMFile*) myfm_file)->is_directory = TRUE;
    else
        ((MyFMFile*) myfm_file)->is_directory = FALSE;

    /* set fields that require async IO to be initialized to NULL before we initialize them */
    myfm_file->g_file_info = NULL;
    myfm_file->display_name = NULL;

    g_file_query_info_async (g_file, "*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                             G_PRIORITY_DEFAULT, NULL, myfm_file_from_g_file_callback, myfm_file);
}

void myfm_file_from_path_async (MyFMFile *myfm_file, const char *path)
{
    myfm_file_from_g_file_async (myfm_file, g_file_new_for_path (path));
}

void myfm_file_free (MyFMFile *myfm_file)
{

}