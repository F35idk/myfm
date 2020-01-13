/*
 * Created by f35 on 11.01.20.
*/

#ifndef __MYFM_ICON_RENDERER_H
#define __MYFM_ICON_RENDERER_H

#include <gtk/gtk.h>

#define MYFM_TYPE_ICON_RENDERER (myfm_icon_renderer_get_type ())
G_DECLARE_FINAL_TYPE            (MyFMIconRenderer, myfm_icon_renderer, MYFM, ICON_RENDERER, GtkCellRendererPixbuf)

GtkCellRenderer *myfm_icon_renderer_new           (void);
void            myfm_icon_renderer_set_icon_faded (MyFMIconRenderer *self, gboolean faded);

#endif /* __MYFM_ICON_RENDERER_H */
