//
// Created by f35 on 21.08.19.
//

#ifndef __MYFM_FILE_H
#define __MYFM_FILE_H

#include <gtk/gtk.h>

/* struct for associating g_files with their respective g_file_monitors, g_file_infos and
 * other convenient properties, as well as ensuring that these update and refresh
 * themselves when needed */
/* TODO: make boxed type? not sure */
typedef struct MyFMFile {
    GFile *g_file;
    // GFileMonitor *g_file_monitor;
    gboolean is_open;
    GFileType filetype;
    const char *IO_display_name;
} MyFMFile;

void myfm_file_from_g_file_async   (MyFMFile *myfm_file, GFile *g_file);
void myfm_file_from_path_async     (MyFMFile *myfm_file, const char *path);
void myfm_file_free                (MyFMFile *myfm_file);
MyFMFile *myfm_file_new_without_io (GFile *g_file, const char *display_name);

#endif // __MYFM_FILE_H
