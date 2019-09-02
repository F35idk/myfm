//
// Created by f35 on 21.08.19.
//

#ifndef MYFM_MYFM_FILE_H
#define MYFM_MYFM_FILE_H

#include <gtk/gtk.h>

/* struct for associating g_files with their respective g_file_monitors, g_file_infos and
 * other convenient properties, as well as ensuring that these update and refresh
 * themselves when needed */
/* TODO: make boxed type? not sure */
typedef struct MyFMFile {
    GFile *g_file;
    GFileMonitor *g_file_monitor;
    gboolean is_open;
    gboolean is_directory;
    /* these fields require IO to be initialized */
    GFileInfo *IO_g_file_info;
    const char *IO_display_name;
} MyFMFile;

void myfm_file_from_g_file_async (MyFMFile *myfm_file, GFile *g_file);

void myfm_file_from_path_async (MyFMFile *myfm_file, const char *path);

void myfm_file_free (MyFMFile *myfm_file);

MyFMFile *myfm_file_new_without_io (GFile *g_file, GFileInfo *file_info, const char *display_name);

#endif //MYFM_MYFM_FILE_H
