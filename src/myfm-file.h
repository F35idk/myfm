/*
 * Created by f35 on 21.08.19.
*/

#ifndef __MYFM_FILE_H
#define __MYFM_FILE_H

#include <gtk/gtk.h>
#include <gio/gio.h>

/* boxed type for representing files in the file manager. is a mostly simple extension of
 * g_file which keeps some specific g_file/g_file_info properties cached (things like display
 * name, icon, filesize, etc.) that would otherwise require IO to obtain for each request.
 * a myfm_file does not refresh itself, so to keep it updated we monitor its g_file with a
 * g_file_monitor and call myfm_file_update accordingly */

#define MYFM_TYPE_FILE (myfm_file_get_type ())
#define MYFM_FILE_QUERY_ATTRIBUTES "standard::display-name,standard::icon,"\
                                   "standard::content-type,standard::size,"\
                                   "standard::is-symlink,standard::is-hidden,"\
                                   "standard::type,time::modified"

struct MyFMFilePrivate;
typedef struct {
    struct MyFMFilePrivate *priv;
} MyFMFile;

/*                                file,   old g_file, error,    user_data */
typedef void (*MyFMFileUpdateCallback)(MyFMFile *, GFile *, GError *, gpointer);

GFile      *myfm_file_get_g_file                (MyFMFile *self);
void       myfm_file_set_g_file                 (MyFMFile *self, GFile *g_file);
GFileType  myfm_file_get_filetype               (MyFMFile *self);
GType      myfm_file_get_type                   (void);
GIcon      *myfm_file_get_icon                  (MyFMFile *self);
goffset    myfm_file_get_size                   (MyFMFile *self);
GDateTime  *myfm_file_get_modification_time     (MyFMFile *self);
gboolean   myfm_file_is_open                    (MyFMFile *self);
void       myfm_file_set_is_open                (MyFMFile *self, gboolean is_open);
const char *myfm_file_get_display_name          (MyFMFile *self);
const char *myfm_file_get_content_type          (MyFMFile *self);
void       myfm_file_from_g_file_async          (GFile *g_file, MyFMFileUpdateCallback callback, gpointer user_data);
void       myfm_file_update_async               (MyFMFile *self, GFile *new_g_file,
                                                 MyFMFileUpdateCallback callback, gpointer user_data);
void       myfm_file_set_display_name_async     (MyFMFile *self, char *display_name,
                                                 MyFMFileUpdateCallback callback, gpointer user_data);
void       myfm_file_unref                      (MyFMFile *self);
MyFMFile   *myfm_file_ref                       (MyFMFile *self); /* mustn't be void to fit glib boxed api */
MyFMFile   *myfm_file_new_with_info             (GFile *g_file, GFileInfo *info);
MyFMFile   *myfm_file_from_path                 (const char *path);
MyFMFile   *myfm_file_from_g_file               (GFile *g_file);
MyFMFile   *myfm_file_new_empty                 (void);

#endif /* __MYFM_FILE_H */
