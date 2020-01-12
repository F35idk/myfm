/*
 * Created by f35 on 15.08.19.
*/

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "myfm-icon-renderer.h"

struct _MyFMIconRenderer {
    GtkCellRendererText parent_instance;
    gboolean icon_faded;
};

G_DEFINE_TYPE (MyFMIconRenderer, myfm_icon_renderer, GTK_TYPE_CELL_RENDERER_PIXBUF)

void
myfm_icon_renderer_set_icon_faded (MyFMIconRenderer *self,
                                   gboolean faded)
{
    self->icon_faded = faded;
}

static void
myfm_icon_renderer_render (GtkCellRenderer *renderer,
                           cairo_t *cr, GtkWidget *widget,
                           const GdkRectangle *background_area,
                           const GdkRectangle *cell_area,
                           GtkCellRendererState flags)
{
    MyFMIconRenderer *self;
    GtkCellRendererClass *renderer_cls;

    self = MYFM_ICON_RENDERER (renderer);
    renderer_cls = GTK_CELL_RENDERER_CLASS (myfm_icon_renderer_parent_class);

    if (!self->icon_faded) {
        renderer_cls->render (renderer, cr, widget,
                              background_area,
                              cell_area, flags);
    }
    else { /* add white tint to icon */
        /* redirect drawing to group
         * (avoid messing up cell background) */
        cairo_push_group (cr);
        /* clear destination (in group) */
        cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (cr);

        /* call gtk_cell_renderer base
         * implementation to draw the icon for us */
        renderer_cls->render (renderer, cr, widget,
                              background_area,
                              cell_area, flags);

        /* apply white tint to opaque parts of group
         * (i.e the icon and not the background) */
        cairo_surface_t *mask_surface = cairo_get_group_target (cr);
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.4);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_mask_surface (cr, mask_surface, 0, 0);

        /* pop group and draw it onto the background */
        cairo_pattern_t *pattern = cairo_pop_group (cr);
        cairo_set_source (cr, pattern);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_paint (cr);

        self->icon_faded = FALSE;
    }
}

static void
myfm_icon_renderer_init (MyFMIconRenderer *self)
{
    self->icon_faded = FALSE;
}

static void
myfm_icon_renderer_class_init (MyFMIconRendererClass *cls)
{
    GtkCellRendererClass *cell_cls = GTK_CELL_RENDERER_CLASS (cls);

    cell_cls->render = myfm_icon_renderer_render;
}

GtkCellRenderer *
myfm_icon_renderer_new (void)
{
    return g_object_new (MYFM_TYPE_ICON_RENDERER, NULL);
}
