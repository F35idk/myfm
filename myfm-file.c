/*
 * Created by f35 on 21.08.19.
*/

#include <glib.h>
#include <gio/gio.h>

#include "myfm-file.h"
#define G_LOG_DOMAIN "myfm-file"

struct MyFMFilePrivate {
    GFile *g_file;
    gboolean is_open_dir;
    GFileType filetype;
    guint refcount;
    GCancellable *cancellable;
    const char *IO_display_name;
    const char *IO_content_type;
    GIcon *IO_g_icon;
};

G_DEFINE_BOXED_TYPE (MyFMFile, myfm_file, myfm_file_ref, myfm_file_unref)

static void myfm_file_setup_g_icon (MyFMFile *self, GFileInfo *info)
{
    self->priv->IO_g_icon = g_file_info_get_icon (info);
    if (!self->priv->IO_g_icon) {
        g_critical ("unable to set icon on myfm_file: '%s' \n",
                    myfm_file_get_display_name (self));
        /* use unknown file icon if icon can't be set */
        self->priv->IO_g_icon = g_themed_icon_new ("unknown");
    }

    g_object_ref (self->priv->IO_g_icon);
}

static void myfm_file_clear_io_fields (MyFMFile *self)
{
    if (self->priv->IO_display_name) {
        g_free (self->priv->IO_display_name);
        self->priv->IO_display_name = NULL;
    }
    if (self->priv->IO_content_type) {
        g_free (self->priv->IO_content_type);
        self->priv->IO_content_type = NULL;
    }
    if (self->priv->IO_g_icon) {
        g_object_unref (self->priv->IO_g_icon);
        self->priv->IO_g_icon = NULL;
    }
}

static void myfm_file_init_io_fields_with_info (MyFMFile *self, GFileInfo *info)
{
    self->priv->IO_display_name = g_strdup (g_file_info_get_display_name (info));
    self->priv->IO_content_type = g_strdup (g_file_info_get_content_type (info));
    myfm_file_setup_g_icon (self, info);
}

/* convenience struct used in myfm_file_IO_fields_callback () and
 * myfm_file_init_IO_fields_async () to allow setting custom callback functions
 * to be executed once a myfm_file has been initialized async. */
struct callback_data {
    MyFMFile *self;
    MyFMFileCallback callback;
    gpointer user_data;
};

static void myfm_file_IO_fields_callback (GObject *g_file, GAsyncResult *res, gpointer callback_data)
{
    GFileInfo_autoptr info = NULL;
    GError_autoptr error = NULL;
    MyFMFile *self;
    struct callback_data *cb_data;

    cb_data = (struct callback_data *) callback_data;
    self = cb_data->self;
    info = g_file_query_info_finish (G_FILE (g_file), res, &error);

    if (error) {
        g_critical ("unable to initialize myfm_file: %s \n", error->message);
        self->priv->IO_display_name = NULL;

        /* unref myfm_file if the error is caused by the file no longer existing */
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
            myfm_file_unref (self);
        else /* only unref info if file still exists. might still emit a warning for being null though */
            g_object_unref (info);
    }
    else {
        myfm_file_clear_io_fields (self);
        myfm_file_init_io_fields_with_info (self, info);

        /* execute custom provided callback */
        if (cb_data->callback) {
            cb_data->callback (self, cb_data->user_data);
            cb_data->callback = NULL;
            cb_data->user_data = NULL;
        }
    }

    /* decrement refcount of file when we're done */
    myfm_file_unref (self);
    free (cb_data);
}

static void myfm_file_init_io_fields_async (MyFMFile *self, MyFMFileCallback callback, gpointer user_data)
{
    struct callback_data *cb_data;

    cb_data = malloc (sizeof (struct callback_data));
    cb_data->self = self;
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    /* TODO: only query for the info we need */
    g_file_query_info_async (self->priv->g_file, "*",
                             G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, G_PRIORITY_DEFAULT,
                             self->priv->cancellable, myfm_file_IO_fields_callback, cb_data);

    /* keep file alive until callback is invoked to prevent potential use after free */
    myfm_file_ref (self);
}

/* main constructor - most other constructors call this to start. creates a new myfm_file
 * without initializing the fields that require async IO. refs the g_file passed into it */
static MyFMFile *myfm_file_new_without_io_fields (GFile *g_file)
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
    myfm_file->priv->IO_g_icon = NULL;
    myfm_file->priv->IO_display_name = NULL;
    myfm_file->priv->IO_content_type = NULL;

    return myfm_file;
}

/* asynchronous constructor. should be used instead of the sync alternative to avoid blocking
 * the main loop. takes a callback that will be executed once the myfm_file has been initialized */
void myfm_file_from_g_file_async (GFile *g_file, MyFMFileCallback callback, gpointer user_data)
{
    MyFMFile *myfm_file;
    myfm_file = myfm_file_new_without_io_fields (g_file);
    myfm_file_init_io_fields_async (myfm_file, callback, user_data);
}

/* synchronous constructor. isn't used except for at startup */
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

    info = g_file_query_info (g_file, "*",
                              G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);

    if (error)
        g_critical ("unable to get info on file: %s \n", error->message);
    else
        myfm_file_init_io_fields_with_info (myfm_file, info);

    /* valgrind tells me unref'ing is needed, despite
    * info being an auto_ptr */
    g_object_unref (info); /* TODO: why? */

    return myfm_file;
}

/* initializes a myfm_file completely without doing any async IO. requires a g_file_info.
 * refs the g_file passed into it. */
MyFMFile *myfm_file_new_with_info (GFile *g_file, GFileInfo *info)
{
    MyFMFile *myfm_file;

    myfm_file = myfm_file_new_without_io_fields (g_file);

    if (myfm_file == NULL)
        return myfm_file;

    myfm_file_init_io_fields_with_info (myfm_file, info);

    return myfm_file;
}

/* synchronous constructor. isn't used except for at startup */
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

void myfm_file_update_async (MyFMFile *self, GFile *new_g_file,
                             MyFMFileCallback callback, gpointer user_data)
{
    g_object_unref (self->priv->g_file);
    g_object_ref (new_g_file);
    self->priv->g_file = new_g_file;
    self->priv->filetype = g_file_query_file_type (new_g_file,
                                                   G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
  
    myfm_file_clear_io_fields (self);
    myfm_file_init_io_fields_async (self, callback, user_data);
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

const char *myfm_file_get_content_type (MyFMFile *self)
{
    if (self->priv->IO_content_type)
        return self->priv->IO_content_type;
    else
        return " ";
}

GIcon *myfm_file_get_icon (MyFMFile *self)
{
    return self->priv->IO_g_icon;
}

void myfm_file_free (MyFMFile *self)
{
    g_debug ("freeing : %s", myfm_file_get_display_name (self));

    if (self->priv->g_file) {
        g_object_unref (self->priv->g_file);
        self->priv->g_file = NULL;
    }

    myfm_file_clear_io_fields (self);

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
    g_debug ("unrefing: %s, current refcount: %u \n",
             myfm_file_get_display_name (self), self->priv->refcount);
    self->priv->refcount--;

    if (!self->priv->refcount)
       myfm_file_free (self);
}

MyFMFile *myfm_file_ref (MyFMFile *self)
{
    g_debug ("refing: %s, current refcount: %u \n",
             myfm_file_get_display_name (self), self->priv->refcount);
    self->priv->refcount++;

    return self;
}
