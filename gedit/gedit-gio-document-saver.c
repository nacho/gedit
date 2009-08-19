/*
 * gedit-gio-document-saver.c
 * This file is part of gedit
 *
 * Copyright (C) 2005-2006 - Paolo Borelli and Paolo Maggi
 * Copyright (C) 2007 - Paolo Borelli, Paolo Maggi, Steve Frécinaux
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
 * Modified by the gedit Team, 2005-2006. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>

#include "gedit-convert.h"
#include "gedit-gio-document-saver.h"
#include "gedit-debug.h"

typedef struct
{
	GeditGioDocumentSaver *saver;
	gchar 		      *buffer;
	GCancellable 	      *cancellable;
	gboolean	       tried_mount;
} AsyncData;

#define WRITE_CHUNK_SIZE 8192
#define REMOTE_QUERY_ATTRIBUTES G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE "," \
				G_FILE_ATTRIBUTE_TIME_MODIFIED
				
#define GEDIT_GIO_DOCUMENT_SAVER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
							  GEDIT_TYPE_GIO_DOCUMENT_SAVER, \
							  GeditGioDocumentSaverPrivate))

static void	     gedit_gio_document_saver_save		    (GeditDocumentSaver *saver,
								     time_t              old_mtime);
static const gchar *gedit_gio_document_saver_get_content_type	    (GeditDocumentSaver *saver);
static time_t       gedit_gio_document_saver_get_mtime		    (GeditDocumentSaver *saver);
static goffset	     gedit_gio_document_saver_get_file_size	    (GeditDocumentSaver *saver);
static goffset	     gedit_gio_document_saver_get_bytes_written	    (GeditDocumentSaver *saver);


static void 	    check_modified_async 			    (AsyncData          *async);

struct _GeditGioDocumentSaverPrivate
{
	time_t                    doc_mtime;
	gchar                    *content_type;

	goffset			  size;
	goffset			  bytes_written;

	GFile			 *gfile;
	GCancellable		 *cancellable;
	GFileOutputStream	 *stream;
	
	GError                   *error;
};

G_DEFINE_TYPE(GeditGioDocumentSaver, gedit_gio_document_saver, GEDIT_TYPE_DOCUMENT_SAVER)

static void
gedit_gio_document_saver_finalize (GObject *object)
{
	GeditGioDocumentSaverPrivate *priv = GEDIT_GIO_DOCUMENT_SAVER (object)->priv;

	if (priv->cancellable)
	{
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
	}

	if (priv->gfile)
		g_object_unref (priv->gfile);

	g_free (priv->content_type);

	if (priv->error)
		g_error_free (priv->error);
		
	if (priv->stream)
		g_object_unref (priv->stream);

	G_OBJECT_CLASS (gedit_gio_document_saver_parent_class)->finalize (object);
}

static AsyncData *
async_data_new (GeditGioDocumentSaver *gvsaver)
{
	AsyncData *async;
	
	async = g_slice_new (AsyncData);
	async->saver = gvsaver;
	async->buffer = NULL;
	async->cancellable = g_object_ref (gvsaver->priv->cancellable);
	async->tried_mount = FALSE;
	
	return async;
}

static void
async_data_free (AsyncData *async)
{
	g_object_unref (async->cancellable);
	g_free (async->buffer);
	g_slice_free (AsyncData, async);
}

static void 
gedit_gio_document_saver_class_init (GeditGioDocumentSaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditDocumentSaverClass *saver_class = GEDIT_DOCUMENT_SAVER_CLASS (klass);

	object_class->finalize = gedit_gio_document_saver_finalize;

	saver_class->save = gedit_gio_document_saver_save;
	saver_class->get_content_type = gedit_gio_document_saver_get_content_type;
	saver_class->get_mtime = gedit_gio_document_saver_get_mtime;
	saver_class->get_file_size = gedit_gio_document_saver_get_file_size;
	saver_class->get_bytes_written = gedit_gio_document_saver_get_bytes_written;

	g_type_class_add_private (object_class, sizeof(GeditGioDocumentSaverPrivate));
}

static void
gedit_gio_document_saver_init (GeditGioDocumentSaver *gvsaver)
{
	gvsaver->priv = GEDIT_GIO_DOCUMENT_SAVER_GET_PRIVATE (gvsaver);

	gvsaver->priv->cancellable = g_cancellable_new ();
	gvsaver->priv->error = NULL;
}

static void
remote_save_completed_or_failed (GeditGioDocumentSaver *gvsaver, 
				 AsyncData 	       *async)
{
	if (async)
		async_data_free (async);

	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (gvsaver),
				     TRUE,
				     gvsaver->priv->error);
}

static void
async_failed (AsyncData *async,
	      GError    *error)
{
	g_propagate_error (&async->saver->priv->error, error);
	
	remote_save_completed_or_failed (async->saver, async);
}

/* prototype, because they call each other... isn't C lovely */
static void write_file_chunk (AsyncData *async);

static void
handle_file_info (AsyncData *async,
		  GFileInfo *info)
{
	GeditGioDocumentSaver *saver;
	GTimeVal mtime;

	saver = async->saver;

	gedit_debug_message (DEBUG_SAVER, "Query info finished");

	if (g_file_info_has_attribute (info,
				       G_FILE_ATTRIBUTE_TIME_MODIFIED))
	{
		g_file_info_get_modification_time (info, &mtime);
		saver->priv->doc_mtime = mtime.tv_sec;
	}

	if (g_file_info_has_attribute (info,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
	{
		saver->priv->content_type = g_strdup (g_file_info_get_content_type (info));
	}
}

static void
remote_reget_info_cb (GFile        *source,
		      GAsyncResult *res,
		      AsyncData    *async)
{
	GeditGioDocumentSaver *saver;
	GFileInfo *info;
	GError *error = NULL;

	saver = async->saver;

	/* check cancelled state manually */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}
	
	gedit_debug_message (DEBUG_SAVER, "Finished query info on file");
	info = g_file_query_info_finish (source, res, &error);

	if (info != NULL)
	{
		handle_file_info (async, info);
		g_object_unref (info);
	}
	else
	{
		gedit_debug_message (DEBUG_SAVER, "Query info failed: %s", error->message);
		g_propagate_error (&saver->priv->error, error);
	}

	remote_save_completed_or_failed (saver, async);
}

static void
close_async_ready_get_info_cb (GOutputStream *stream,
			       GAsyncResult  *res,
			       AsyncData     *async)
{
	GError *error = NULL;
	
	/* check cancelled state manually */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}
	
	gedit_debug_message (DEBUG_SAVER, "Finished closing stream");
	
	if (!g_output_stream_close_finish (stream, res, &error))
	{
		gedit_debug_message (DEBUG_SAVER, "Closing stream error: %s", error->message);

		async_failed (async, error);
		return;
	}
	
	gedit_debug_message (DEBUG_SAVER, "Query info on file");
	g_file_query_info_async (async->saver->priv->gfile,
			         REMOTE_QUERY_ATTRIBUTES,
			         G_FILE_QUERY_INFO_NONE,
			         G_PRIORITY_HIGH,
			         async->cancellable,
			         (GAsyncReadyCallback) remote_reget_info_cb,
			         async);
}

static void
close_async_ready_cb (GOutputStream *stream,
		      GAsyncResult  *res,
		      AsyncData     *async)
{
	GError *error = NULL;

	/* check cancelled state manually */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}

	if (!g_output_stream_close_finish (stream, res, &error))
	{
		g_propagate_error (&async->saver->priv->error, error);
	}

	remote_save_completed_or_failed (async->saver, async);
}

static void
remote_get_info_cb (GFileOutputStream *stream,
		    GAsyncResult      *res,
		    AsyncData         *async)
{
	GeditGioDocumentSaver *saver;
	GFileInfo *info;
	GError *error = NULL;
	GAsyncReadyCallback next_callback;

	saver = async->saver;

	/* check cancelled state manually */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}
	
	gedit_debug_message (DEBUG_SAVER, "Finishing info query");
	info = g_file_output_stream_query_info_finish (stream, res, &error);
	gedit_debug_message (DEBUG_SAVER, "Query info result: %s", info ? "ok" : "fail");

	if (info == NULL)
	{
		if (error->code == G_IO_ERROR_NOT_SUPPORTED || error->code == G_IO_ERROR_CLOSED)
		{
			gedit_debug_message (DEBUG_SAVER, "Query info not supported on stream, trying on file");

			/* apparently the output stream does not support query info.
			 * we're forced to restat on the file. But first we make sure
			 * to close the stream
			 */
			g_error_free (error);
			next_callback = (GAsyncReadyCallback) close_async_ready_get_info_cb;
		}
		else
		{
			gedit_debug_message (DEBUG_SAVER, "Query info failed: %s", error->message);
			g_propagate_error (&saver->priv->error, error);

			next_callback = (GAsyncReadyCallback) close_async_ready_cb;
		}
	}
	else
	{
		handle_file_info (async, info);
		g_object_unref (info);

		next_callback = (GAsyncReadyCallback) close_async_ready_cb;
	}

	g_output_stream_close_async (G_OUTPUT_STREAM (saver->priv->stream),
				     G_PRIORITY_HIGH,
				     async->cancellable,
				     next_callback,
				     async);
}

static void
write_complete (AsyncData *async)
{
	/* document is succesfully saved. we know requery for the mime type and
	 * the mtime. I'm not sure this is actually necessary, can't we just use
	 * g_content_type_guess (since we have the file name and the data)
	 */
	gedit_debug_message (DEBUG_SAVER, "Write complete, query info on stream");
	
	/* query info on the stream */
	g_file_output_stream_query_info_async (async->saver->priv->stream, 
					       REMOTE_QUERY_ATTRIBUTES,
					       G_PRIORITY_HIGH,
					       async->cancellable,
					       (GAsyncReadyCallback) remote_get_info_cb,
					       async);
}

static void
async_write_cb (GOutputStream *stream,
	        GAsyncResult  *res,
	        AsyncData     *async)
{
	GError *error = NULL;
	GeditGioDocumentSaver *gvsaver;
	gssize bytes_written;
	
	gedit_debug (DEBUG_SAVER);
	
	/* manually check cancelled state */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}

	gvsaver = async->saver;
	bytes_written = g_output_stream_write_finish (stream, res, &error);

	gedit_debug_message (DEBUG_SAVER, "Written: %" G_GSSIZE_FORMAT, bytes_written);

	if (bytes_written == -1)
	{
		gedit_debug_message (DEBUG_SAVER, "Write error: %s", error->message);
		async_failed (async, error);
		return;
	}
	
	gvsaver->priv->bytes_written += bytes_written;
	
	/* if nothing is written we're done */
	if (gvsaver->priv->bytes_written == gvsaver->priv->size)
	{
		write_complete (async);
		return;
	}
	
	/* otherwise emit progress and write some more */

	/* note that this signal blocks the write... check if it isn't
	 * a performance problem
	 */
	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (gvsaver),
				     FALSE,
				     NULL);

	write_file_chunk (async);
}

static void
write_file_chunk (AsyncData *async)
{
	GeditGioDocumentSaver *gvsaver;

	gvsaver = async->saver;

	gedit_debug_message (DEBUG_SAVER,
			     "Writing next chunk: %" G_GINT64_FORMAT "/%" G_GINT64_FORMAT,
			     gvsaver->priv->bytes_written, gvsaver->priv->size);

	g_output_stream_write_async (G_OUTPUT_STREAM (gvsaver->priv->stream),
				     async->buffer + gvsaver->priv->bytes_written,
				     MIN (WRITE_CHUNK_SIZE, gvsaver->priv->size - 
				     			    gvsaver->priv->bytes_written),
				     G_PRIORITY_HIGH,
				     async->cancellable,
				     (GAsyncReadyCallback) async_write_cb,
				     async);
}

static void
async_replace_ready_callback (GFile        *source,
			      GAsyncResult *res,
			      AsyncData    *async)
{
	GeditGioDocumentSaver *gvsaver;
	GError *error = NULL;

	/* Check cancelled state manually */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}
	
	gvsaver = async->saver;
	gvsaver->priv->stream = g_file_replace_finish (source, res, &error);
	
	/* handle any error that might occur */
	if (!gvsaver->priv->stream)
	{
		gedit_debug_message (DEBUG_SAVER, "Opening file failed: %s", error->message);
		async_failed (async, error);
		return;
	}

	/* now that we have the stream, start writing async to it */
	write_file_chunk (async);
}

/* returns a pointer to the reallocated buffer */
static gchar *
append_new_line (GeditGioDocumentSaver  *gvsaver, 
		 gchar 		        *buffer, 
		 gsize 		        *len, 
		 GError 	       **error)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (gvsaver);
	gsize n_len;
	gchar *n_buffer;
	gchar *res;

	res = buffer;

	n_buffer = gedit_document_saver_get_end_newline (saver, &n_len);
	if (n_buffer != NULL)
	{
		gchar *new_buffer;

		new_buffer = g_try_realloc (buffer, *len + n_len + 1);

		if (new_buffer == NULL)
		{
			/* we do not error out, just use the buffer
			   without the ending newline */
			g_free (n_buffer);
			g_warning ("Cannot add '\\n' at the end of the file.");

			return res;
		}

		/* Copy newline */
		memcpy (new_buffer + *len, n_buffer, n_len);
		g_free (n_buffer);
		*len += n_len;

		new_buffer[*len] = '\0';

		res = new_buffer;
	}

	return res;
}

static void
begin_write (AsyncData *async)
{
	GeditGioDocumentSaver *gvsaver;
	gchar *buffer;
	gsize len;
	GError *error = NULL;

	gedit_debug_message (DEBUG_SAVER, "Start replacing file contents");

	/* For remote files we simply use g_file_replace_async. There is no
	 * backup as of yet
	 */
	gvsaver = async->saver;
	buffer = gedit_document_saver_get_document_contents (GEDIT_DOCUMENT_SAVER (gvsaver), &len, &error);
	if (buffer != NULL && len > 0)
	{
		/* Append new line to buffer */
		buffer = append_new_line (gvsaver, buffer, &len, &error);
	}

	if (!buffer)
	{
		async_failed (async, error);
		return;
	}

	async->buffer = buffer;
	gvsaver->priv->size = len;

	gedit_debug_message (DEBUG_SAVER, "File contents size: %" G_GINT64_FORMAT, gvsaver->priv->size);
	gedit_debug_message (DEBUG_SAVER, "Calling replace_async");

	g_file_replace_async (gvsaver->priv->gfile, 
			      NULL,
			      FALSE,
			      G_FILE_CREATE_NONE,
			      G_PRIORITY_HIGH,
			      async->cancellable,
			      (GAsyncReadyCallback) async_replace_ready_callback,
			      async);
}

static void
mount_ready_callback (GFile        *file,
		      GAsyncResult *res,
		      AsyncData    *async)
{
	GError *error = NULL;
	gboolean mounted;
	
	/* manual check for cancelled state */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}

	mounted = g_file_mount_enclosing_volume_finish (file, res, &error);
	
	if (!mounted)
	{
		async_failed (async, error);
	}
	else
	{
		/* try again to get the modified state */
		check_modified_async (async);
	}
}

static void
recover_not_mounted (AsyncData *async)
{
	GeditDocument *doc;
	GMountOperation *mount_operation;
	
	gedit_debug (DEBUG_LOADER);

	doc = gedit_document_saver_get_document (GEDIT_DOCUMENT_SAVER (async->saver));
	mount_operation = _gedit_document_create_mount_operation (doc);

	async->tried_mount = TRUE;
	g_file_mount_enclosing_volume (async->saver->priv->gfile,
				       G_MOUNT_MOUNT_NONE,
				       mount_operation,
				       async->cancellable,
				       (GAsyncReadyCallback) mount_ready_callback,
				       async);	

	g_object_unref (mount_operation);			
}

static void
check_modification_callback (GFile        *source,
			     GAsyncResult *res,
			     AsyncData    *async)
{
	GeditGioDocumentSaver *gvsaver;
	GError *error = NULL;
	GFileInfo *info;
	
	/* manually check cancelled state */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}
	
	gvsaver = async->saver;
	info = g_file_query_info_finish (source, res, &error);
	if (info == NULL)
	{
		if (error->code == G_IO_ERROR_NOT_MOUNTED && !async->tried_mount)
		{
			recover_not_mounted (async);
			g_error_free (error);
			return;
		}
		
		/* it's perfectly fine if the file doesn't exist yet */
		if (error->code != G_IO_ERROR_NOT_FOUND)
		{
			gedit_debug_message (DEBUG_SAVER, "Error getting modification: %s", error->message);

			async_failed (async, error);
			return;
		}
	}

	/* check if the mtime is > what we know about it (if we have it) */
	if (info != NULL && g_file_info_has_attribute (info,
				       G_FILE_ATTRIBUTE_TIME_MODIFIED))
	{
		GTimeVal mtime;

		g_file_info_get_modification_time (info, &mtime);

		if (gvsaver->priv->doc_mtime > 0 &&
		    mtime.tv_sec != gvsaver->priv->doc_mtime &&
		    (GEDIT_DOCUMENT_SAVER (gvsaver)->flags & GEDIT_DOCUMENT_SAVE_IGNORE_MTIME) == 0)
		{
			gedit_debug_message (DEBUG_SAVER, "File is externally modified");
			g_set_error (&gvsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED,
				     "Externally modified");

			remote_save_completed_or_failed (gvsaver, async);
			g_object_unref (info);

			return;
		}
	}

	if (info != NULL)
		g_object_unref (info);

	/* modification check passed, start write */
	begin_write (async);
}

static void
check_modified_async (AsyncData *async)
{
	gedit_debug_message (DEBUG_SAVER, "Check externally modified");

	g_file_query_info_async (async->saver->priv->gfile, 
				 G_FILE_ATTRIBUTE_TIME_MODIFIED,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_HIGH,
				 async->cancellable,
				 (GAsyncReadyCallback) check_modification_callback,
				 async);
}

static gboolean
save_remote_file_real (GeditGioDocumentSaver *gvsaver)
{
	AsyncData *async;

	gedit_debug_message (DEBUG_SAVER, "Starting gio save");
	
	/* First find out if the file is modified externally. This requires
	 * a stat, but I don't think we can do this any other way
	 */
	async = async_data_new (gvsaver);
	
	check_modified_async (async);
	
	/* return false to stop timeout */
	return FALSE;
}

static void
gedit_gio_document_saver_save (GeditDocumentSaver *saver,
			       time_t              old_mtime)
{
	GeditGioDocumentSaver *gvsaver = GEDIT_GIO_DOCUMENT_SAVER (saver);

	gvsaver->priv->doc_mtime = old_mtime;
	gvsaver->priv->gfile = g_file_new_for_uri (saver->uri);

	/* saving start */
	gedit_document_saver_saving (saver, FALSE, NULL);

	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    (GSourceFunc) save_remote_file_real,
			    gvsaver,
			    NULL);
}

static const gchar *
gedit_gio_document_saver_get_content_type (GeditDocumentSaver *saver)
{
	return GEDIT_GIO_DOCUMENT_SAVER (saver)->priv->content_type;
}

static time_t
gedit_gio_document_saver_get_mtime (GeditDocumentSaver *saver)
{
	return GEDIT_GIO_DOCUMENT_SAVER (saver)->priv->doc_mtime;
}

static goffset
gedit_gio_document_saver_get_file_size (GeditDocumentSaver *saver)
{
	return GEDIT_GIO_DOCUMENT_SAVER (saver)->priv->size;
}

static goffset
gedit_gio_document_saver_get_bytes_written (GeditDocumentSaver *saver)
{
	return GEDIT_GIO_DOCUMENT_SAVER (saver)->priv->bytes_written;
}
