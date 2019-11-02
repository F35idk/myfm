//
// Created by f35 on 21.08.19.
//

#include <glib.h>
#include <gio/gio.h>

#include "myfm-file.h"

struct MyFMFilePrivate {
    GFile *g_file;
    gboolean is_open_dir;
    GFileType filetype;
    const char *IO_display_name;
    guint refcount;
    GCancellable *cancellable;
};

G_DEFINE_BOXED_TYPE (MyFMFile, myfm_file, myfm_file_ref, myfm_file_unref)

/* TODO: expand func to initialize all IO dependent fields when these are added to myfm_file */
static void myfm_file_display_name_callback (GObject *g_file, GAsyncResult *res, gpointer self)
{
    GFileInfo_autoptr info = NULL;
    GError_autoptr error = NULL;

    info = g_file_query_info_finish (G_FILE (g_file), res, &error);

    if (error) {
        g_object_unref (info); /* info is often null in case of error, so this might emit a warning */
        g_critical ("unable to set display name on file: %s \n", error->message);
        ((MyFMFile*) self)->priv->IO_display_name = NULL;
    }
    else {
        if (((MyFMFile *) self)->priv->IO_display_name)
            g_free (((MyFMFile *) self)->priv->IO_display_name);
        ((MyFMFile *) self)->priv->IO_display_name = g_strdup (g_file_info_get_display_name (info));
    }

    myfm_file_unref (self);
}

void myfm_file_init_io_fields_async (MyFMFile *self)
{
    /* currently the only field that requires IO to be initialized is the display name */
    g_file_query_info_async (self->priv->g_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                             G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, G_PRIORITY_DEFAULT,
                             self->priv->cancellable, myfm_file_display_name_callback, self);

    /* keep file alive until callback is invoked to prevent potential use after free */
    myfm_file_ref (self);
}

MyFMFile *myfm_file_from_g_file (GFile *g_file)
{
    MyFMFile *myfm_file;
    GFileInfo_autoptr info = NULL;
    GError_autoptr error = NULL;

    myfm_file = myfm_file_new_without_io_fields (g_file);

    if (myfm_file == NULL) {
        /* g_object_unref (info); FIXME: this shouldn't be needed */
        return myfm_file;
    }

    info = g_file_query_info (g_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                              G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);

    if (error) {
        g_critical ("unable to set display name on file: %s \n", error->message);
    }
    else {
        myfm_file->priv->IO_display_name = g_strdup (g_file_info_get_display_name (info));
    }

    /* valgrind tells me unref'ing is needed, despite
    * info being an auto_ptr */
    g_object_unref (info); /* TODO: why? */

    return myfm_file;
}

/* creates a new myfm_file without initializing the fields that require async IO.
 * refs the g_file passed into it */
// TODO: static?
MyFMFile *myfm_file_new_without_io_fields (GFile *g_file)
{
    MyFMFile *myfm_file;

    g_object_ref (g_file);

    myfm_file = malloc (sizeof (MyFMFile));

    if (myfm_file == NULL) {
        g_critical ("malloc returned NULL, unable to create myfm_file \n");
        g_object_unref (g_file);
        return myfm_file;
    }

    myfm_file->priv = malloc (sizeof (struct MyFMFilePrivate));

    if (myfm_file->priv == NULL) {
        g_critical ("malloc returned NULL, unable to create myfm_file \n");
        g_object_unref (g_file);
        return myfm_file;
    }

    myfm_file->priv->g_file = g_file;
    myfm_file->priv->is_open_dir = FALSE;
    myfm_file->priv->refcount = 1;
    myfm_file->priv->cancellable = g_cancellable_new ();
    myfm_file->priv->filetype = g_file_query_file_type (g_file,
                                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    myfm_file->priv->IO_display_name = NULL;

    return myfm_file;
}

/* TODO: in the future, this function should take a g_file_info instead of just the display name */
/* initializes a myfm_file completely without doing any async IO. requires the ... (FIXME).
 * refs the g_file passed into it. */
MyFMFile *myfm_file_new_with_info (GFile *g_file, const char *display_name)
{
    MyFMFile *myfm_file;

    myfm_file = myfm_file_new_without_io_fields (g_file);

    if (myfm_file == NULL) {
        g_free (display_name);
        return myfm_file;
    }

    myfm_file->priv->IO_display_name = display_name;

    return myfm_file;
}

MyFMFile *myfm_file_from_path (const char *path)
{
    GFile *g_file;
    MyFMFile *myfm_file;

    g_file = g_file_new_for_path (path);
    myfm_file = myfm_file_from_g_file (g_file);
    /* myfm_file_from_g_file () refs the g_file, so we unref it here (could also use auto) */
    g_object_unref (g_file);

    return myfm_file;
}

GFile *myfm_file_get_g_file (MyFMFile *self)
{
    return self->priv->g_file;
}

/* TODO: might eventually separate into "update_display_name" and "update" */
void myfm_file_update (MyFMFile *self, GFile *new_g_file)
{
    g_object_unref (self->priv->g_file);
    g_object_ref (new_g_file);
    self->priv->g_file = new_g_file;
    self->priv->filetype = g_file_query_file_type (new_g_file,
                                                   G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);

    if (self->priv->IO_display_name) {
        g_free (self->priv->IO_display_name);
        self->priv->IO_display_name = NULL;
    }

    myfm_file_init_io_fields_async (self);
}

gboolean myfm_file_is_open (MyFMFile *self)
{
    return self->priv->is_open_dir;
}

void myfm_file_set_is_open (MyFMFile *self, gboolean is_open)
{
    if (self->priv->filetype != G_FILE_TYPE_DIRECTORY)
        return;

    self->priv->is_open_dir = is_open;
}

GFileType myfm_file_get_filetype (MyFMFile *self)
{
    return self->priv->filetype;
}

const char *myfm_file_get_display_name (MyFMFile *self)
{
    if (self->priv->IO_display_name)
        return self->priv->IO_display_name;
    else
        return " ";
}

void myfm_file_free (MyFMFile *self)
{
    printf ("freeing: ");
    puts (myfm_file_get_display_name (self));

    if (self->priv->g_file) {
        g_object_unref (self->priv->g_file);
        self->priv->g_file = NULL;
    }

    if (self->priv->IO_display_name) {
        g_free (self->priv->IO_display_name);
        self->priv->IO_display_name = NULL;
    }

    /* cancel all IO on file */
    if (self->priv->cancellable) {
        g_cancellable_cancel (self->priv->cancellable);
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }

    free (self->priv);
    free (self);
}

/* as of 2.58, glib provides its own refcounting api. however, for portability, we
 * use our own dead simple refcounting system instead (literally just a counter).
 * it should be more than enough. */
void myfm_file_unref (MyFMFile *self)
{
    printf ("unrefing: %s", myfm_file_get_display_name (self));
    printf (", current refcount: %u \n", self->priv->refcount);
    self->priv->refcount--;

    printf ("refcount: %u \n", self->priv->refcount);

    if (!self->priv->refcount)
       myfm_file_free (self);
}

MyFMFile *myfm_file_ref (MyFMFile *self)
{
    printf ("refing: %s", myfm_file_get_display_name (self));
    printf (", current refcount: %u \n", self->priv->refcount);
    self->priv->refcount++;

    return self;
}
