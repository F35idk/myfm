/*
 * Created by f35 on 15.08.19.
*/

#ifndef __MYFM_APPLICATION_H
#define __MYFM_APPLICATION_H

#include <gtk/gtk.h>

#include "myfm-file.h"
#include "myfm-clipboard.h"

#define MYFM_TYPE_APPLICATION (myfm_application_get_type ())
G_DECLARE_FINAL_TYPE          (MyFMApplication, myfm_application, MYFM, APPLICATION, GtkApplication)

MyFMApplication *myfm_application_new                 (void);
GtkIconSize     myfm_application_get_icon_size        (MyFMApplication *self);
gboolean        myfm_application_copy_in_progress     (MyFMApplication *self);
void            myfm_application_set_copy_in_progress (MyFMApplication *self, gboolean in_progress);
MyFMClipBoard  *myfm_application_get_file_clipboard   (MyFMApplication *self);

#endif /* __MYFM_APPLICATION_H */
