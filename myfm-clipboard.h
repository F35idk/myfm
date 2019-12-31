/*
 * Created by f35 on 23.12.19.
*/

#ifndef __MYFM_CLIPBOARD_H
#define __MYFM_CLIPBOARD_H

#include <gtk/gtk.h>
#include <glib.h>

#include "myfm-file.h"

#define MYFM_TYPE_CLIPBOARD (myfm_clipboard_get_type ())

typedef struct {
    GPtrArray *copied_files;
    GHashTable *cut_files;
} MyFMClipBoard;

void          myfm_clipboard_free           (MyFMClipBoard *self);
MyFMClipBoard *myfm_clipboard_copy          (MyFMClipBoard *self);
MyFMClipBoard *myfm_clipboard_new           (void);
void          myfm_clipboard_add_to_copied  (MyFMClipBoard *self, MyFMFile **files, gint n_files);
void          myfm_clipboard_add_to_cut     (MyFMClipBoard *self, MyFMFile **files, gint n_files);
gboolean      myfm_clipboard_is_empty       (MyFMClipBoard *self);
MyFMFile      **myfm_clipboard_get_contents (MyFMClipBoard *self, gint *out_n_files, gboolean *out_copied);

#endif /* __MYFM_CLIPBOARD_H */
