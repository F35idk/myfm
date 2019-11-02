//
// Created by f35 on 15.08.19.
//

#ifndef __MYFM_APPLICATION_H
#define __MYFM_APPLICATION_H

#include <gtk/gtk.h>

#define MYFM_APPLICATION_TYPE (myfm_application_get_type ())
G_DECLARE_FINAL_TYPE (MyFMApplication, myfm_application, MYFM, APPLICATION, GtkApplication)

MyFMApplication *myfm_application_new (void);

#endif //__MYFM_APPLICATION_H
