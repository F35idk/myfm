/*
 * Created by f35 on 08.01.20.
*/

#ifndef __MYFM_FILE_OPERATIONS_PRIVATE_H
#define __MYFM_FILE_OPERATIONS_PRIVATE_H

#include <gtk/gtk.h>

#include "myfm-utils.h"
#include "myfm-file.h"

/* TODO: make our file-op flags a combinable
 * bitflag enum instead of this bool struct.
 * then we could return them directly from
 * our convenience dialog functions and thus
 * avoid having to manually set them every
 * time we handle dialog responses. */
typedef struct {
    gboolean use_fallback;
    gboolean merge_all;
    gboolean merge_once;
    gboolean make_copy_once;
    gboolean make_copy_all;
    gboolean ignore_merges;
    gboolean ignore_warn_errors;
} _FileOpFlags;

typedef enum {
    FILE_OPERATION_COPY,
    FILE_OPERATION_MOVE,
    FILE_OPERATION_DELETE,
} _FileOpType;

/* struct for passing g_files
 * and their type to
 * _copy_files_thread () */
struct _file_w_type {
    GFile *g_file;
    GFileType type;
};

/* FIXME: don't declare this here */
typedef void (*MyFMFileOpCallback)(gpointer);


MyFMDialogResponse _run_merge_dialog     (GTask *operation,
                                          const gchar *format_msg, ...);
MyFMDialogResponse _run_warn_err_dialog  (GTask *operation, _FileOpType type,
                                          const gchar *format_msg, ...);
MyFMDialogResponse _run_fatal_err_dialog (GTask *operation, _FileOpType type,
                                          const gchar *format_msg, ...);
void               _copy_files_thread    (GTask *task, gpointer src_object,
                                          gpointer task_data, GCancellable *cancellable);
void               _copy_files_finish    (GObject *src_object, GAsyncResult *res,
                                          gpointer _cb);
void               _move_files_thread    (GTask *task, gpointer src_object,
                                          gpointer task_data, GCancellable *cancellable);
void               _move_files_finish    (GObject *src_object, GAsyncResult *res,
                                          gpointer _cb);
void               _delete_files_thread  (GTask *task, gpointer src_object,
                                          gpointer task_data, GCancellable *cancellable);
void               _delete_files_finish  (GObject *src_object, GAsyncResult *res,
                                          gpointer _cb);
void               _delete_file_single   (GFile *file, GtkWindow *active,
                                          GCancellable *cancellable, GError **error);

#endif /* __MYFM_FILE_OPERATIONS_PRIVATE_H */
