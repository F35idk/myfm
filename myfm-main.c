/*
 * Created by f35 on 15.08.19.
*/

#include <gtk/gtk.h>

#include "myfm-window.h"
#include "myfm-application.h"

int
main (int argc, char *argv[])
{
    return g_application_run (G_APPLICATION (myfm_application_new ()),
                              argc, argv);
}
