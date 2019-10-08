//
// Created by f35 on 21.08.19.
//

#include <glib.h>
#include <gio/gio.h>

#include "myfm-file.h"

// TODO: expand func to initialize all IO dependent fields when these are added to myfm_file
static void display_name_callback (GObject *g_file, GAsyncResult *res, gpointer myfm_file)
{
    GFileInfo_autoptr info = NULL;
    GError_autoptr error = NULL;

    info = g_file_query_info_finish (G_FILE (g_file), res, &error);

    // if (((MyFMFile*) myfm_file)->IO_display_name) {
    //    g_free (((MyFMFile *) myfm_file)->IO_display_name);
    // }
    if (error) {
        g_object_unref (info); // info is often null in case of error, so this might emit a warning
        g_critical ("unable to set display name on file: %s \n", error->message);
        // ((MyFMFile*) myfm_file)->IO_display_name = g_strdup ("");
    }
    else {
        if (((MyFMFile *) myfm_file)->IO_display_name)
            g_free (((MyFMFile *) myfm_file)->IO_display_name);
        ((MyFMFile *) myfm_file)->IO_display_name = g_strdup (g_file_info_get_display_name (info));
    }

    myfm_file_unref (myfm_file);
}

void myfm_file_init_io_fields_async (MyFMFile *myfm_file)
{
    /* currently the only field that requires IO to be initialized is the display name */
    g_file_query_info_async (myfm_file->g_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                             G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, G_PRIORITY_DEFAULT,
                             myfm_file->_cancellable, display_name_callback, myfm_file);

    /* set the display name to be empty before it is initialized properly */
    myfm_file->IO_display_name = g_strdup ("");

    /* keep file alive until callback is invoked to prevent potential use after free */
    myfm_file_ref (myfm_file);
}

MyFMFile *myfm_file_from_g_file (GFile *g_file)
{
    MyFMFile *myfm_file;
    GFileInfo_autoptr info = NULL;
    GError_autoptr error = NULL;

    myfm_file = myfm_file_new_without_io_fields (g_file);

    if (myfm_file == NULL) {
        /* valgrind tells me unref'ing is needed, despite
         * info being an auto_ptr */
        g_object_unref (info);
        return myfm_file;
    }

    info = g_file_query_info (g_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                              G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);

    if (error) {
        g_critical ("unable to set display name on file: %s \n", error->message);
        myfm_file->IO_display_name = g_strdup ("");
    }
    else {
        myfm_file->IO_display_name = g_strdup (g_file_info_get_display_name (info));
    }

    /* valgrind tells me unref'ing is needed, despite
    * info being an auto_ptr */
    g_object_unref (info); /* TODO: why? */

    return myfm_file;
}

// creates a new myfm_file without initializing the fields that require async IO
MyFMFile *myfm_file_new_without_io_fields (GFile *g_file)
{
    MyFMFile *myfm_file;

    myfm_file = malloc (sizeof (MyFMFile));

    if (myfm_file == NULL) {
        g_critical ("malloc returned NULL, unable to create myfm_file \n");
        g_object_unref (g_file);
        return myfm_file;
    }

    myfm_file->g_file = g_file;
    myfm_file->is_open_dir = FALSE; // TODO: thonk
    myfm_file->_refcount = 1;
    myfm_file->_cancellable = g_cancellable_new ();
    myfm_file->filetype = g_file_query_file_type (g_file,
                                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    myfm_file->IO_display_name = NULL;

    return myfm_file;
}

// TODO: in the future, this function should take a g_file_info instead of just the display name
// initializes a myfm_file completely without doing any async IO. requires the ... (FIXME)
MyFMFile *myfm_file_new_with_info (GFile *g_file, const char *display_name)
{
    MyFMFile *myfm_file;

    myfm_file = myfm_file_new_without_io_fields (g_file);

    if (myfm_file == NULL) {
        g_free (display_name);
        return myfm_file;
    }

    myfm_file->IO_display_name = display_name;

    return myfm_file;
}

MyFMFile *myfm_file_from_path (const char *path)
{
    return myfm_file_from_g_file (g_file_new_for_path (path));
}

void myfm_file_free (MyFMFile *myfm_file)
{
    // FIXME: check for null
    printf ("freeing: ");
    puts (myfm_file->IO_display_name);

    g_object_unref (myfm_file->g_file);
    myfm_file->g_file = NULL;

    g_free (myfm_file->IO_display_name);
    myfm_file->IO_display_name = NULL;

    /* cancel all IO on file */
    g_cancellable_cancel (myfm_file->_cancellable);
    g_object_unref (myfm_file->_cancellable);
    myfm_file->_cancellable = NULL;

    free (myfm_file);
}

/* as of 2.58, glib provides its own refcounting api. however, for portability, we
 * use our own dead simple refcounting system instead (literally just a counter).
 * it should be more than enough. */
void myfm_file_unref (MyFMFile *myfm_file)
{
    printf ("unrefing: %s", myfm_file->IO_display_name);
    printf (", current refcount: %u \n", myfm_file->_refcount);
    myfm_file->_refcount--;

    printf ("refcount: %u \n", myfm_file->_refcount);

    if (!myfm_file->_refcount)
       myfm_file_free (myfm_file);
}

void myfm_file_ref (MyFMFile *myfm_file)
{
    printf ("refing: %s", myfm_file->IO_display_name);
    printf (", current refcount: %u \n", myfm_file->_refcount);
    myfm_file->_refcount++;
}
