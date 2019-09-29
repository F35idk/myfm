//
// Created by f35 on 21.08.19.
//

#ifndef __MYFM_FILE_H
#define __MYFM_FILE_H

#include <gtk/gtk.h>

/* simple struct for associating g_files with their display names and other
 * convenient properties */
typedef struct MyFMFile {
    GFile *g_file;
    gboolean is_open; /* currently useless field */
    GFileType filetype;
    const char *IO_display_name;
    guint priv_refcount;
} MyFMFile;

void myfm_file_from_g_file_async   (MyFMFile *myfm_file, GFile *g_file);
void myfm_file_from_path_async     (MyFMFile *myfm_file, const char *path);
void myfm_file_unref               (MyFMFile *myfm_file);
void myfm_file_ref                 (MyFMFile *myfm_file);
void myfm_file_free                (MyFMFile *myfm_file);
MyFMFile *myfm_file_new_without_io (GFile *g_file, const char *display_name);
MyFMFile *myfm_file_from_path      (const char *path);
MyFMFile *myfm_file_from_g_file    (GFile *g_file);

#endif // __MYFM_FILE_H
