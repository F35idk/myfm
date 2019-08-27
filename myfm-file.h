//
// Created by f35 on 21.08.19.
//

#ifndef MYFM_MYFM_FILE_H
#define MYFM_MYFM_FILE_H

#include <gtk/gtk.h>

typedef struct MyFMFile {
    GFile *g_file;
    GFileInfo *g_file_info; // TODO: REMOVE
    GFileMonitor *g_file_monitor;
    const char *display_name;
    gboolean is_open;
    gboolean is_directory;
} MyFMFile;

void myfm_file_from_g_file_async (MyFMFile *myfm_file, GFile *g_file);

void myfm_file_from_path_async (MyFMFile *myfm_file, const char *path);

void myfm_file_free (MyFMFile *myfm_file);

#endif //MYFM_MYFM_FILE_H
