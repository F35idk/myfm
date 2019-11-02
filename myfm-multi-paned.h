//
// Created by f35 on 13.10.19.
//

#ifndef MYFM_MULTI_PANED_H
#define MYFM_MULTI_PANED_H

/* extension of gtk_paned that nests multiple gtk_paned widgets inside one another to allow
 * for free resizing of multiple widgets. emits the signals "expand" and "shrink" before
 * resizing which parent widgets can connect to to change their adjustment */

#define MYFM_TYPE_MULTI_PANED (myfm_multi_paned_get_type ())
G_DECLARE_FINAL_TYPE          (MyFMMultiPaned, myfm_multi_paned, MYFM, MULTI_PANED, GtkPaned)

MyFMMultiPaned *myfm_multi_paned_new              (void);
void           myfm_multi_paned_set_pane_sizes    (MyFMMultiPaned *self, guint default_size, guint min_size);
void           myfm_multi_paned_update_size       (MyFMMultiPaned *self);
void           myfm_multi_paned_add               (MyFMMultiPaned *self, GtkWidget *child);
void           myfm_multi_paned_truncate_by_index (MyFMMultiPaned *self, guint index);

#endif //MYFM_MULTI_PANED_H
