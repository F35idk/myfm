/*
 * Created by f35 on 15.08.19.
*/

#ifndef __MYFM_APPLICATION_H
#define __MYFM_APPLICATION_H

#include <gtk/gtk.h>

#include "myfm-file.h"

#define MYFM_TYPE_APPLICATION (myfm_application_get_type ())
G_DECLARE_FINAL_TYPE          (MyFMApplication, myfm_application, MYFM, APPLICATION, GtkApplication)

typedef enum MyFMActionType {
    MYFM_OPEN_FILE_ACTION,
    MYFM_RENAME_FILE_ACTION,
    MYFM_MOVE_FILE_ACTION,
    MYFM_COPY_FILE_ACTION,
    MYFM_DELETE_FILE_ACTION,
} MyFMActionType;

/* convenience structs for passing multiple parameters to specific
 * application and window actions (g_simple_actions) */
struct MyFMOpenFileArgs {
    MyFMFile *target_file;
    int dirview_index;
};
struct MyFMRenameFileArgs {
    gpointer *something;
};
struct MyFMMoveFileArgs {
    gpointer *something;
};
struct MyFMCopyFileArgs {
    gpointer *something;
};
struct MyFMDeleteFileArgs {
    gpointer *something;
};

MyFMApplication *myfm_application_new            (void);
GtkIconSize     myfm_application_get_icon_size   (MyFMApplication *self);
gpointer        myfm_application_get_action_args (MyFMApplication *self, MyFMActionType action_type);
void            myfm_application_set_action_args (MyFMApplication *self, gpointer src_struct, MyFMActionType action_type);

#endif /* __MYFM_APPLICATION_H */
