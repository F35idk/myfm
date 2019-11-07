//
// Created by f35 on 21.08.19.
//

#ifndef __MYFM_FILE_H
#define __MYFM_FILE_H

#include <gtk/gtk.h>
#include <gio/gio.h>

/* boxed type for representing files in the file manager. is a mostly simple extension of
 * g_file which keeps some specific g_file/g_file_info properties cached (things like display
 * name, icon, filesize, etc.) that would otherwise require IO to obtain for each request.
 * a MyFMFile does not refresh itself, so if you wish to keep it updated you should monitor
 * its g_file with a g_file_monitor and call myfm_file_update accordingly */
/* TODO: add icons, filesize to myfm_file (as described in comment above) */

#define MYFM_TYPE_FILE (myfm_file_get_type ())

struct MyFMFilePrivate;
typedef struct MyFMFile {
    struct MyFMFilePrivate *priv;
} MyFMFile;

GFile      *myfm_file_get_g_file            (MyFMFile *self);
GFileType  myfm_file_get_filetype           (MyFMFile *self);
GType      myfm_file_get_type               (void);
GIcon     *myfm_file_get_icon               (MyFMFile *self);
gboolean   myfm_file_is_open                (MyFMFile *self);
void       myfm_file_set_is_open            (MyFMFile *self, gboolean is_open);
const char *myfm_file_get_display_name      (MyFMFile *self);
void       myfm_file_init_io_fields_async   (MyFMFile *myfm_file);
void       myfm_file_update_async           (MyFMFile *self, GFile *new_g_file);
void       myfm_file_free                   (MyFMFile *myfm_file);
void       myfm_file_unref                  (MyFMFile *myfm_file);
MyFMFile   *myfm_file_ref                   (MyFMFile *myfm_file); /* mustn't be void to fit glib boxed api */
MyFMFile   *myfm_file_new_without_io_fields (GFile *g_file);
MyFMFile   *myfm_file_new_with_info         (GFile *g_file, GFileInfo *info);
MyFMFile   *myfm_file_from_path             (const char *path);
MyFMFile   *myfm_file_from_g_file           (GFile *g_file);

#endif // __MYFM_FILE_H
