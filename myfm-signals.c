//
// Created by f35 on 29.08.19.
//

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "myfm-signals.h"

/* function for setting up application signals. is (WILL BE!!) called during startup */
void setup_signals (void)
{
    /* signal to emit when all files in dir are added (async) to store */
    g_signal_new ("children_added", GTK_TYPE_LIST_STORE,
                  G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
                  NULL, G_TYPE_NONE, 0, NULL);
}