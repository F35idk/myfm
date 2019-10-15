//
// Created by f35 on 13.10.19.
//

#include <gtk/gtk.h>

#include "myfm-multi-paned.h"

struct _MyFMMultiPaned {
    GtkPaned parent_instance;
    GtkPaned *inner_pane;
    GtkBox *empty;
    guint n_panes;
    guint pane_offset;
    guint default_pane_size;
    guint min_pane_size;
};

G_DEFINE_TYPE (MyFMMultiPaned, myfm_multi_paned, GTK_TYPE_PANED)

void myfm_multi_paned_add (MyFMMultiPaned *self, GtkWidget *child)
{
    // FIXME: this almost.. no, it works
    self->pane_offset = self->default_pane_size * (self->n_panes + 1);

    if (self->n_panes == 0) {
        gtk_paned_pack1 (GTK_PANED (self), GTK_WIDGET (child), FALSE, FALSE);
        g_object_ref (self->empty);
        gtk_container_remove (GTK_CONTAINER (self), GTK_WIDGET (self->empty));
        gtk_paned_pack1 (GTK_PANED (self), GTK_WIDGET (child), FALSE, FALSE);
        gtk_paned_pack2 (GTK_PANED (self), GTK_WIDGET (self->empty), TRUE, TRUE);
        gtk_widget_set_size_request (GTK_WIDGET (self), self->default_pane_size /* self->pane_offset */, -1);
        gtk_paned_set_position (GTK_PANED (self), self->default_pane_size);
        gtk_widget_show (GTK_WIDGET (self));

        self->inner_pane = GTK_PANED (self);

    }
    else {
        GtkPaned *new_inner_pane;

        self->pane_offset -= self->min_pane_size;

        new_inner_pane = GTK_PANED (gtk_paned_new (GTK_ORIENTATION_HORIZONTAL));
        gtk_widget_set_size_request (child, self->min_pane_size, -1);
        gtk_widget_set_size_request (GTK_WIDGET (new_inner_pane), self->pane_offset, -1);
        gtk_paned_set_position (new_inner_pane, self->default_pane_size);

        g_object_ref (self->empty);
        gtk_container_remove (GTK_CONTAINER (self->inner_pane), GTK_WIDGET (self->empty));

        gtk_paned_pack1 (new_inner_pane, child,  FALSE, FALSE);
        gtk_paned_pack2 (new_inner_pane, GTK_WIDGET (self->empty),  TRUE, TRUE);
        gtk_paned_pack2 (self->inner_pane, GTK_WIDGET (new_inner_pane),  TRUE, TRUE);
        gtk_widget_show (GTK_WIDGET (new_inner_pane));
        // gtk_widget_show (GTK_WIDGET (self->empty));

        self->inner_pane = new_inner_pane;
    }

    self->n_panes += 1;
}

/* removes all children in self from index and onwards */
void myfm_multi_paned_truncate_by_index (MyFMMultiPaned *self, guint index)
{
    if (index >= self->n_panes)
        return;

    GtkWidget *current = GTK_WIDGET (self);

    /* iterate and set current to be the pane/widget referenced by index */
    for (int i = 0; i < index; i++)
        current = gtk_paned_get_child2 (GTK_PANED (current));

    /* change self->inner_pane to point to parent of current, destroy current and move
     * self->empty into self->inner_pane */
    self->inner_pane = GTK_PANED (gtk_widget_get_parent (current));
    g_object_ref (self->empty);
    gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (GTK_WIDGET (self->empty))),
                                         GTK_WIDGET (self->empty));
    gtk_widget_destroy (current);
    gtk_paned_pack2 (self->inner_pane, GTK_WIDGET (self->empty), TRUE, TRUE);

    self->n_panes = index;
}

void myfm_multi_paned_set_pane_sizes (MyFMMultiPaned *self, guint default_size, guint min_size)
{
    self->default_pane_size = default_size;
    self->min_pane_size = min_size;
}

static void myfm_multi_paned_constructed (GObject *object)
{
    MyFMMultiPaned *self;

    G_OBJECT_CLASS (myfm_multi_paned_parent_class)->constructed (object);

    self = MYFM_MULTI_PANED (object);

    gtk_paned_pack2 (GTK_PANED (self), GTK_WIDGET (self->empty), TRUE, TRUE);
    gtk_widget_show (GTK_WIDGET (self->empty));
}

static void myfm_multi_paned_class_init (MyFMMultiPanedClass *cls)
{
    GObjectClass *object_cls = G_OBJECT_CLASS (cls);

    object_cls->constructed = myfm_multi_paned_constructed;
}

static void myfm_multi_paned_init (MyFMMultiPaned *self)
{
    self->inner_pane = NULL;
    self->empty = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    self->n_panes = 0;
    self->pane_offset = 0;
    self->default_pane_size = 200;
    self->min_pane_size = 60;
}

MyFMMultiPaned *myfm_multi_paned_new (void)
{
    return g_object_new (MYFM_MULTI_PANED_TYPE, NULL);
}
