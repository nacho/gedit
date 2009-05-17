/*
 * gedit-gio-document-loader.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
 * Copyright (C) 2008 - Jesse van den Kieboom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 2005-2008. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "gedit-gio-document-loader.h"
#include "gedit-debug.h"
#include "gedit-metadata-manager.h"
#include "gedit-utils.h"

#define READ_CHUNK_SIZE 8192
#define REMOTE_QUERY_ATTRIBUTES G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE "," \
				G_FILE_ATTRIBUTE_STANDARD_TYPE "," \
				G_FILE_ATTRIBUTE_TIME_MODIFIED "," \
				G_FILE_ATTRIBUTE_STANDARD_SIZE "," \
				G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE

#define GEDIT_GIO_DOCUMENT_LOADER_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
				 GEDIT_TYPE_GIO_DOCUMENT_LOADER,   \
				 GeditGioDocumentLoaderPrivate))

static void	    gedit_gio_document_loader_load		(GeditDocumentLoader *loader);
static gboolean     gedit_gio_document_loader_cancel		(GeditDocumentLoader *loader);
static const gchar *gedit_gio_document_loader_get_content_type	(GeditDocumentLoader *loader);
static time_t	    gedit_gio_document_loader_get_mtime		(GeditDocumentLoader *loader);
static goffset	    gedit_gio_document_loader_get_file_size	(GeditDocumentLoader *loader);
static goffset	    gedit_gio_document_loader_get_bytes_read	(GeditDocumentLoader *loader);
static gboolean	    gedit_gio_document_loader_get_readonly	(GeditDocumentLoader *loader);

static void open_async_read (GeditGioDocumentLoader *loader);

struct _GeditGioDocumentLoaderPrivate
{
	GFile            *gfile;
	GFileInfo        *info;
	goffset           bytes_read;
	gboolean          tried_mount;

	GCancellable     *cancellable;
	GFileInputStream *stream;

	gchar            *buffer;

	GError           *error;
};

G_DEFINE_TYPE(GeditGioDocumentLoader, gedit_gio_document_loader, GEDIT_TYPE_DOCUMENT_LOADER)

static void
gedit_gio_document_loader_finalize (GObject *object)
{
	GeditGioDocumentLoaderPrivate *priv;

	priv = GEDIT_GIO_DOCUMENT_LOADER (object)->priv;

	if (priv->cancellable != NULL)
	{
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
	}

	if (priv->info)
		g_object_unref (priv->info);
	
	if (priv->stream)
		g_object_unref (priv->stream);

	g_free (priv->buffer);

	if (priv->gfile)
		g_object_unref (priv->gfile);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_gio_document_loader_parent_class)->finalize (object);
}

static void
gedit_gio_document_loader_class_init (GeditGioDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditDocumentLoaderClass *loader_class = GEDIT_DOCUMENT_LOADER_CLASS (klass);

	object_class->finalize = gedit_gio_document_loader_finalize;

	loader_class->load = gedit_gio_document_loader_load;
	loader_class->cancel = gedit_gio_document_loader_cancel;
	loader_class->get_content_type = gedit_gio_document_loader_get_content_type;
	loader_class->get_mtime = gedit_gio_document_loader_get_mtime;
	loader_class->get_file_size = gedit_gio_document_loader_get_file_size;
	loader_class->get_bytes_read = gedit_gio_document_loader_get_bytes_read;
	loader_class->get_readonly = gedit_gio_document_loader_get_readonly;

	g_type_class_add_private (object_class, sizeof(GeditGioDocumentLoaderPrivate));
}

static void
gedit_gio_document_loader_init (GeditGioDocumentLoader *loader)
{
	GeditGioDocumentLoaderPrivate *priv;

	priv = GEDIT_GIO_DOCUMENT_LOADER_GET_PRIVATE (loader);

	priv->tried_mount = FALSE;
	priv->error = NULL;

	loader->priv = priv;
}

static void
remote_load_completed_or_failed (GeditGioDocumentLoader *loader)
{
	/* free the buffer */
	g_free (loader->priv->buffer);
	loader->priv->buffer = NULL;

	if (loader->priv->stream != NULL)
		g_input_stream_close_async (G_INPUT_STREAM (loader->priv->stream),
					    G_PRIORITY_HIGH,
					    NULL, NULL, NULL);

	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (loader),
				       TRUE,
				       loader->priv->error);
}

/* prototype, because they call each other... isn't C lovely */
static void	read_file_chunk		(GeditGioDocumentLoader *loader);

static void
async_failed (GeditGioDocumentLoader *loader, GError *error)
{
	g_propagate_error (&loader->priv->error, error);
	remote_load_completed_or_failed (loader);
}

static void
async_read_cb (GInputStream           *stream,
	       GAsyncResult           *res,
	       GeditGioDocumentLoader *loader)
{
	GError *error = NULL;
	gssize bytes_read;

	gedit_debug (DEBUG_LOADER);

	/* manually check cancelled state */
	if (g_cancellable_is_cancelled (loader->priv->cancellable))
	{
		g_input_stream_close_async (stream,
					    G_PRIORITY_HIGH,
					    NULL, NULL, NULL);
		return;
	}

	bytes_read = g_input_stream_read_finish (stream, res, &error);

	/* error occurred */
	if (bytes_read == -1)
	{
		async_failed (loader, error);
		return;
	}

	/* Check for the extremely unlikely case where the file size overflows. */
	if (loader->priv->bytes_read + bytes_read < loader->priv->bytes_read)
	{
		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GEDIT_DOCUMENT_ERROR_TOO_BIG,
			     "File too big");

		remote_load_completed_or_failed (loader);

		return;
	}

	/* Bump the size. */
	loader->priv->bytes_read += bytes_read;

	/* end of the file, we are done! */
	if (bytes_read == 0)
	{
		gedit_document_loader_update_document_contents (
						GEDIT_DOCUMENT_LOADER (loader),
						loader->priv->buffer,
						loader->priv->bytes_read,
						&loader->priv->error);

		remote_load_completed_or_failed (loader);

		return;
	}

	/* otherwise emit progress and read some more */

	/* note that this signal blocks the read... check if it isn't
	 * a performance problem
	 */
	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (loader),
				       FALSE,
				       NULL);

	read_file_chunk (loader);
}

static void
read_file_chunk (GeditGioDocumentLoader *loader)
{
	loader->priv->buffer = g_realloc (loader->priv->buffer,
					  loader->priv->bytes_read + READ_CHUNK_SIZE);

	g_input_stream_read_async (G_INPUT_STREAM (loader->priv->stream),
				   loader->priv->buffer + loader->priv->bytes_read,
				   READ_CHUNK_SIZE,
				   G_PRIORITY_HIGH,
				   loader->priv->cancellable,
				   (GAsyncReadyCallback) async_read_cb,
				   loader);
}

static void
finish_query_info (GeditGioDocumentLoader *loader)
{
	/* if it's not a regular file, error out... */
	if (g_file_info_has_attribute (loader->priv->info, G_FILE_ATTRIBUTE_STANDARD_TYPE) &&
	    g_file_info_get_file_type (loader->priv->info) != G_FILE_TYPE_REGULAR)
	{
		g_set_error (&loader->priv->error,
			     G_IO_ERROR,
			     G_IO_ERROR_NOT_REGULAR_FILE,
			     "Not a regular file");

		remote_load_completed_or_failed (loader);

		return;
	}

	/* start reading */
	read_file_chunk (loader);
}

static void
remote_get_file_info_cb (GFile                  *source,
			 GAsyncResult           *res,
			 GeditGioDocumentLoader *loader)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_LOADER);
	
	/* manually check the cancelled state */
	if (g_cancellable_is_cancelled (loader->priv->cancellable))
	{
		return;
	}

	/* finish the info query */
	loader->priv->info = g_file_query_info_finish (loader->priv->gfile,
	                                               res,
	                                               &error);

	if (!loader->priv->info)
	{
		/* propagate the error and clean up */
		async_failed (loader, error);
		return;
	}

	finish_query_info (loader);
}

static void
remote_get_info_cb (GFileInputStream       *source,
		    GAsyncResult           *res,
		    GeditGioDocumentLoader *loader)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_LOADER);

	/* manually check the cancelled state */
	if (g_cancellable_is_cancelled (loader->priv->cancellable))
	{
		return;
	}

	/* finish the info query */
	loader->priv->info = g_file_input_stream_query_info_finish (loader->priv->stream,
	                                                            res,
	                                                            &error);

	if (loader->priv->info == NULL)
	{
		if (error->code == G_IO_ERROR_NOT_SUPPORTED)
		{
			gedit_debug_message (DEBUG_LOADER, "Query info not supported on stream, trying on file");

			/* then try to query the info using g_file_query_info */
			g_file_query_info_async (loader->priv->gfile,
						 REMOTE_QUERY_ATTRIBUTES,
						 G_FILE_QUERY_INFO_NONE,
						 G_PRIORITY_HIGH,
						 loader->priv->cancellable,
						 (GAsyncReadyCallback)remote_get_file_info_cb,
						 loader);
			return;
		}

		/* propagate the error and clean up */
		async_failed (loader, error);
		return;
	}

	finish_query_info (loader);
}

static void
mount_ready_callback (GFile                  *file,
		      GAsyncResult           *res,
		      GeditGioDocumentLoader *loader)
{
	GError *error = NULL;
	gboolean mounted;

	/* manual check for cancelled state */
	if (g_cancellable_is_cancelled (loader->priv->cancellable))
	{
		return;
	}

	mounted = g_file_mount_enclosing_volume_finish (file, res, &error);

	if (!mounted)
	{
		async_failed (loader, error);
		return;
	}

	/* try again to open the file for reading */
	open_async_read (loader);
}

static void
recover_not_mounted (GeditGioDocumentLoader *loader)
{
	GeditDocument *doc;
	GMountOperation *mount_operation;

	gedit_debug (DEBUG_LOADER);

	doc = gedit_document_loader_get_document (GEDIT_DOCUMENT_LOADER (loader));
	mount_operation = _gedit_document_create_mount_operation (doc);

	loader->priv->tried_mount = TRUE;
	g_file_mount_enclosing_volume (loader->priv->gfile,
				       G_MOUNT_MOUNT_NONE,
				       mount_operation,
				       loader->priv->cancellable,
				       (GAsyncReadyCallback) mount_ready_callback,
				       loader);

	g_object_unref (mount_operation);
}

static void
async_read_ready_callback (GObject                *source,
			   GAsyncResult           *res,
		           GeditGioDocumentLoader *loader)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_LOADER);

	/* manual check for cancelled state */
	if (g_cancellable_is_cancelled (loader->priv->cancellable))
	{
		return;
	}
	
	loader->priv->stream = g_file_read_finish (loader->priv->gfile,
	                                           res,
	                                           &error);

	if (!loader->priv->stream)
	{
		if (error->code == G_IO_ERROR_NOT_MOUNTED && !loader->priv->tried_mount)
		{
			recover_not_mounted (loader);
			g_error_free (error);
			return;
		}

		/* Propagate error */
		g_propagate_error (&loader->priv->error, error);
		gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (loader),
					       TRUE,
					       loader->priv->error);
	}
	else
	{
		/* get the file info from the input stream */
		g_file_input_stream_query_info_async (loader->priv->stream,
						      REMOTE_QUERY_ATTRIBUTES,
						      G_PRIORITY_HIGH,
						      loader->priv->cancellable,
						      (GAsyncReadyCallback) remote_get_info_cb,
						      loader);
	}
}

static void
open_async_read (GeditGioDocumentLoader *loader)
{
	g_file_read_async (loader->priv->gfile, 
	                   G_PRIORITY_HIGH,
	                   loader->priv->cancellable,
	                   (GAsyncReadyCallback) async_read_ready_callback,
	                   loader);
}

static void
gedit_gio_document_loader_load (GeditDocumentLoader *loader)
{
	GeditGioDocumentLoader *gvloader = GEDIT_GIO_DOCUMENT_LOADER (loader);

	gedit_debug (DEBUG_LOADER);

	/* make sure no load operation is currently running */
	g_return_if_fail (gvloader->priv->cancellable == NULL);

	gvloader->priv->gfile = g_file_new_for_uri (loader->uri);

	/* loading start */
	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (gvloader),
				       FALSE,
				       NULL);

	gvloader->priv->cancellable = g_cancellable_new ();

	open_async_read (gvloader);
}

static const gchar *
gedit_gio_document_loader_get_content_type (GeditDocumentLoader *loader)
{
	GeditGioDocumentLoader *gvloader = GEDIT_GIO_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info != NULL &&
	    g_file_info_has_attribute (gvloader->priv->info,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
	{
		return g_file_info_get_content_type (gvloader->priv->info);
	}

	return NULL;
}

static time_t
gedit_gio_document_loader_get_mtime (GeditDocumentLoader *loader)
{
	GeditGioDocumentLoader *gvloader = GEDIT_GIO_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info != NULL &&
	    g_file_info_has_attribute (gvloader->priv->info,
				       G_FILE_ATTRIBUTE_TIME_MODIFIED))
	{
		GTimeVal timeval;

		g_file_info_get_modification_time (gvloader->priv->info, &timeval);

		return timeval.tv_sec;
	}

	return 0;
}

/* Returns 0 if file size is unknown */
static goffset
gedit_gio_document_loader_get_file_size (GeditDocumentLoader *loader)
{
	GeditGioDocumentLoader *gvloader = GEDIT_GIO_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info != NULL &&
	    g_file_info_has_attribute (gvloader->priv->info,
				       G_FILE_ATTRIBUTE_STANDARD_SIZE))
	{
		return g_file_info_get_size (gvloader->priv->info);
	}

	return 0;
}

static goffset
gedit_gio_document_loader_get_bytes_read (GeditDocumentLoader *loader)
{
	return GEDIT_GIO_DOCUMENT_LOADER (loader)->priv->bytes_read;
}

static gboolean
gedit_gio_document_loader_cancel (GeditDocumentLoader *loader)
{
	GeditGioDocumentLoader *gvloader = GEDIT_GIO_DOCUMENT_LOADER (loader);

	if (gvloader->priv->cancellable == NULL)
		return FALSE;

	g_cancellable_cancel (gvloader->priv->cancellable);

	g_set_error (&gvloader->priv->error,
		     G_IO_ERROR,
		     G_IO_ERROR_CANCELLED,
		     "Operation cancelled");

	remote_load_completed_or_failed (gvloader);

	return TRUE;
}

/* In the case the loader does not know if the file is readonly, for example 
   for most remote files, the function returns FALSE, so that we can try writing
   and if needed handle the error. */
static gboolean
gedit_gio_document_loader_get_readonly (GeditDocumentLoader *loader)
{
	GeditGioDocumentLoader *gvloader = GEDIT_GIO_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info != NULL &&
	    g_file_info_has_attribute (gvloader->priv->info,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
	{
		return !g_file_info_get_attribute_boolean (gvloader->priv->info,
							   G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
	}

	return FALSE;
}
