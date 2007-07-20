/*
 * gedit-gnomevfs-document-loader.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
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
 * Modified by the gedit Team, 2005-2007. See the AUTHORS file for a
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
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-gnomevfs-document-loader.h"
#include "gedit-debug.h"
#include "gedit-metadata-manager.h"
#include "gedit-utils.h"

#define READ_CHUNK_SIZE 8192

#define GEDIT_GNOMEVFS_DOCUMENT_LOADER_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
				 GEDIT_TYPE_GNOMEVFS_DOCUMENT_LOADER,   \
				 GeditGnomeVFSDocumentLoaderPrivate))

static void		 gedit_gnomevfs_document_loader_load		(GeditDocumentLoader *loader);
static gboolean		 gedit_gnomevfs_document_loader_cancel		(GeditDocumentLoader *loader);
static const gchar	*gedit_gnomevfs_document_loader_get_mime_type	(GeditDocumentLoader *loader);
static time_t		 gedit_gnomevfs_document_loader_get_mtime	(GeditDocumentLoader *loader);
static GnomeVFSFileSize	 gedit_gnomevfs_document_loader_get_file_size	(GeditDocumentLoader *loader);
static GnomeVFSFileSize	 gedit_gnomevfs_document_loader_get_bytes_read	(GeditDocumentLoader *loader);
static gboolean		 gedit_gnomevfs_document_loader_get_readonly	(GeditDocumentLoader *loader);


static void	async_close_cb (GnomeVFSAsyncHandle *handle,
				GnomeVFSResult       result,
				gpointer             data);


struct _GeditGnomeVFSDocumentLoaderPrivate
{
	/* Info on the current file */
	GnomeVFSURI          *vfs_uri;

	GnomeVFSFileInfo     *info;
	GnomeVFSFileSize      bytes_read;

	/* Handle for remote files */
	GnomeVFSAsyncHandle  *handle;
	GnomeVFSAsyncHandle  *info_handle;

	gchar                *buffer;

	GError               *error;
};

G_DEFINE_TYPE(GeditGnomeVFSDocumentLoader, gedit_gnomevfs_document_loader, GEDIT_TYPE_DOCUMENT_LOADER)

static void
gedit_gnomevfs_document_loader_finalize (GObject *object)
{
	GeditGnomeVFSDocumentLoaderPrivate *priv;

	priv = GEDIT_GNOMEVFS_DOCUMENT_LOADER (object)->priv;

	if (priv->handle != NULL)
	{
		if (priv->info_handle != NULL)
		{
			gnome_vfs_async_cancel (priv->info_handle);
			gnome_vfs_async_close (priv->info_handle,
					       async_close_cb,
					       NULL);
		}

		gnome_vfs_async_cancel (priv->handle);
		gnome_vfs_async_close (priv->handle,
				       async_close_cb, 
				       NULL);
	}

	if (priv->info)
		gnome_vfs_file_info_unref (priv->info);

	g_free (priv->buffer);

	if (priv->vfs_uri)
		gnome_vfs_uri_unref (priv->vfs_uri);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_gnomevfs_document_loader_parent_class)->finalize (object);
}

static void
gedit_gnomevfs_document_loader_class_init (GeditGnomeVFSDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditDocumentLoaderClass *loader_class = GEDIT_DOCUMENT_LOADER_CLASS (klass);

	object_class->finalize = gedit_gnomevfs_document_loader_finalize;

	loader_class->load = gedit_gnomevfs_document_loader_load;
	loader_class->cancel = gedit_gnomevfs_document_loader_cancel;
	loader_class->get_mime_type = gedit_gnomevfs_document_loader_get_mime_type;
	loader_class->get_mtime = gedit_gnomevfs_document_loader_get_mtime;
	loader_class->get_file_size = gedit_gnomevfs_document_loader_get_file_size;
	loader_class->get_bytes_read = gedit_gnomevfs_document_loader_get_bytes_read;
	loader_class->get_readonly = gedit_gnomevfs_document_loader_get_readonly;

	g_type_class_add_private (object_class, sizeof(GeditGnomeVFSDocumentLoaderPrivate));
}

static void
gedit_gnomevfs_document_loader_init (GeditGnomeVFSDocumentLoader *gvloader)
{
	gvloader->priv = GEDIT_GNOMEVFS_DOCUMENT_LOADER_GET_PRIVATE (gvloader);
	gvloader->priv->error = NULL;
}

static void
async_close_cb (GnomeVFSAsyncHandle *handle,
		GnomeVFSResult       result,
		gpointer             data)
{
	/* nothing to do... no point in reporting an error */
}

static void
remote_load_completed_or_failed (GeditGnomeVFSDocumentLoader *gvloader)
{
	/* free the buffer and close the handle */
	gnome_vfs_async_close (gvloader->priv->handle,
			       async_close_cb,
			       NULL);

	gvloader->priv->handle = NULL;

	g_free (gvloader->priv->buffer);
	gvloader->priv->buffer = NULL;

	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (gvloader),
				       TRUE,
				       gvloader->priv->error);
}

/* prototype, because they call each other... isn't C lovely */
static void	read_file_chunk		(GeditGnomeVFSDocumentLoader *loader);

static void
async_read_cb (GnomeVFSAsyncHandle         *handle,
	       GnomeVFSResult               result,
	       gpointer                     buffer,
	       GnomeVFSFileSize             bytes_requested,
	       GnomeVFSFileSize             bytes_read,
	       GeditGnomeVFSDocumentLoader *gvloader)
{
	gedit_debug (DEBUG_LOADER);

	/* reality checks. */
	g_return_if_fail (bytes_requested == READ_CHUNK_SIZE);
	g_return_if_fail (gvloader->priv->handle == handle);
	g_return_if_fail (gvloader->priv->buffer + gvloader->priv->bytes_read == buffer);
	g_return_if_fail (bytes_read <= bytes_requested);

	/* error occurred */
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF)
	{
		g_set_error (&gvloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		remote_load_completed_or_failed (gvloader);

		return;
	}

	/* Check for the extremely unlikely case where the file size overflows. */
	if (gvloader->priv->bytes_read + bytes_read < gvloader->priv->bytes_read)
	{
		g_set_error (&gvloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_TOO_BIG,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_TOO_BIG));

		remote_load_completed_or_failed (gvloader);

		return;
	}

	/* Bump the size. */
	gvloader->priv->bytes_read += bytes_read;

	/* end of the file, we are done! */
	if (bytes_read == 0 || result != GNOME_VFS_OK)
	{
		gedit_document_loader_update_document_contents (
						GEDIT_DOCUMENT_LOADER (gvloader),
						gvloader->priv->buffer,
						gvloader->priv->bytes_read,
						&gvloader->priv->error);

		remote_load_completed_or_failed (gvloader);

		return;
	}

	/* otherwise emit progress and read some more */

	/* note that this signal blocks the read... check if it isn't
	 * a performance problem
	 */
	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (gvloader),
				       FALSE,
				       NULL);

	read_file_chunk (gvloader);
}

static void
read_file_chunk (GeditGnomeVFSDocumentLoader *gvloader)
{
	gvloader->priv->buffer = g_realloc (gvloader->priv->buffer,
					    gvloader->priv->bytes_read + READ_CHUNK_SIZE);

	gnome_vfs_async_read (gvloader->priv->handle,
			      gvloader->priv->buffer + gvloader->priv->bytes_read,
			      READ_CHUNK_SIZE,
			      (GnomeVFSAsyncReadCallback) async_read_cb,
			      gvloader);
}

static void
remote_get_info_cb (GnomeVFSAsyncHandle         *handle,
		    GList                       *results,
		    GeditGnomeVFSDocumentLoader *gvloader)
{
	GnomeVFSGetFileInfoResult *info_result;

	gedit_debug (DEBUG_LOADER);

	/* assert that the list has one and only one item */
	g_return_if_fail (results != NULL && results->next == NULL);

	gvloader->priv->info_handle = NULL;

	info_result = (GnomeVFSGetFileInfoResult *) results->data;
	g_return_if_fail (info_result != NULL);

	if (info_result->result != GNOME_VFS_OK)
	{
		g_set_error (&gvloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     info_result->result,
			     gnome_vfs_result_to_string (info_result->result));

		remote_load_completed_or_failed (gvloader);

		return;
	}

	/* CHECK: ref is necessary, right? or the info will go away... */
	gvloader->priv->info = info_result->file_info;
	gnome_vfs_file_info_ref (gvloader->priv->info);

	/* if it's not a regular file, error out... */
	if (info_result->file_info->type != GNOME_VFS_FILE_TYPE_REGULAR)
	{
		g_set_error (&gvloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE,
			     "Not a regular file");

		remote_load_completed_or_failed (gvloader);

		return;
	}

	/* start reading */
	read_file_chunk (gvloader);
}

static void
async_open_callback (GnomeVFSAsyncHandle         *handle,
		     GnomeVFSResult               result,
		     GeditGnomeVFSDocumentLoader *gvloader)
{
	GList *uri_list = NULL;

	gedit_debug (DEBUG_LOADER);

	g_return_if_fail (gvloader->priv->handle == handle);

	if (result != GNOME_VFS_OK)
	{
		g_set_error (&gvloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		/* in this case we don't need to close the handle */
		gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (gvloader),
					       TRUE,
					       gvloader->priv->error);

		return;
	}

	/* get the file info after open to avoid races... this really
	 * should be async_get_file_info_from_handle (fstat equivalent)
	 * but gnome-vfs lacks that.
	 */

	uri_list = g_list_prepend (uri_list, gvloader->priv->vfs_uri);

	gnome_vfs_async_get_file_info (&gvloader->priv->info_handle,
				       uri_list,
				       GNOME_VFS_FILE_INFO_DEFAULT |
				       GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
				       GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
				       GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
				       GNOME_VFS_PRIORITY_MAX,
				       (GnomeVFSAsyncGetFileInfoCallback) remote_get_info_cb,
				       gvloader);

	g_list_free (uri_list);
}

static void
load_file (GeditGnomeVFSDocumentLoader *gvloader)
{
	gedit_debug (DEBUG_LOADER);

	g_return_if_fail (gvloader->priv->handle == NULL);

	/* loading start */
	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (gvloader),
				       FALSE,
				       NULL);

	gnome_vfs_async_open_uri (&gvloader->priv->handle,
				  gvloader->priv->vfs_uri,
				  GNOME_VFS_OPEN_READ,
				  GNOME_VFS_PRIORITY_MAX,
				  (GnomeVFSAsyncOpenCallback) async_open_callback,
				  gvloader);
}

static gboolean
vfs_uri_new_failed (GeditGnomeVFSDocumentLoader *gvloader)
{
	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (gvloader),
				       TRUE,
				       gvloader->priv->error);

	/* stop the timeout */
	return FALSE;
}

static void
gedit_gnomevfs_document_loader_load (GeditDocumentLoader *loader)
{
	GeditGnomeVFSDocumentLoader *gvloader = GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader);

	/* vfs_uri may be NULL for some valid but unsupported uris */
	gvloader->priv->vfs_uri = gnome_vfs_uri_new (loader->uri);
	if (gvloader->priv->vfs_uri == NULL)
	{
		g_set_error (&gvloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_NOT_SUPPORTED,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_NOT_SUPPORTED));

		g_timeout_add_full (G_PRIORITY_HIGH,
				    0,
				    (GSourceFunc) vfs_uri_new_failed,
				    loader,
				    NULL);

		return;
	}

	load_file (gvloader);
}

static const gchar *
gedit_gnomevfs_document_loader_get_mime_type (GeditDocumentLoader *loader)
{
	GeditGnomeVFSDocumentLoader *gvloader = GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info &&
	    (gvloader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE))
		return gvloader->priv->info->mime_type;
	else
		return NULL;
}

static time_t
gedit_gnomevfs_document_loader_get_mtime (GeditDocumentLoader *loader)
{
	GeditGnomeVFSDocumentLoader *gvloader = GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info &&
	    (gvloader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME))
		return gvloader->priv->info->mtime;
	else
		return 0;
}

/* Returns 0 if file size is unknown */
static GnomeVFSFileSize
gedit_gnomevfs_document_loader_get_file_size (GeditDocumentLoader *loader)
{
	GeditGnomeVFSDocumentLoader *gvloader = GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info == NULL)
		return (GnomeVFSFileSize) 0;

	return (GnomeVFSFileSize) gvloader->priv->info->size;
}

static GnomeVFSFileSize
gedit_gnomevfs_document_loader_get_bytes_read (GeditDocumentLoader *loader)
{
	return GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader)->priv->bytes_read;
}

static gboolean
gedit_gnomevfs_document_loader_cancel (GeditDocumentLoader *loader)
{
	GeditGnomeVFSDocumentLoader *gvloader = GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader);

	if (gvloader->priv->handle == NULL)
		return FALSE;

	if (gvloader->priv->info_handle != NULL)
	{
		gnome_vfs_async_cancel (gvloader->priv->info_handle);
		gnome_vfs_async_close (gvloader->priv->info_handle,
				       async_close_cb, 
				       NULL);
	}

	gnome_vfs_async_cancel (gvloader->priv->handle);

	g_set_error (&gvloader->priv->error,
		     GEDIT_DOCUMENT_ERROR,
		     GNOME_VFS_ERROR_CANCELLED,
		     gnome_vfs_result_to_string (GNOME_VFS_ERROR_CANCELLED));

	remote_load_completed_or_failed (gvloader);

	return TRUE;
}

/* In the case the loader does not know if the file is readonly, for example 
   for most remote files, the function returns FALSE, so that we can try writing
   and if needed handle the error. */
static gboolean
gedit_gnomevfs_document_loader_get_readonly (GeditDocumentLoader *loader)
{
	GeditGnomeVFSDocumentLoader *gvloader = GEDIT_GNOMEVFS_DOCUMENT_LOADER (loader);

	if (gvloader->priv->info &&
	    (gvloader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_ACCESS))
		return (gvloader->priv->info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) ? FALSE : TRUE;
	else
		return FALSE;
}
