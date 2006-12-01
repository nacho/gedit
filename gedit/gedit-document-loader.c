/*
 * gedit-document-loader.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-document-loader.h"
#include "gedit-convert.h"
#include "gedit-debug.h"
#include "gedit-metadata-manager.h"
#include "gedit-utils.h"

#include "gedit-marshal.h"

#define READ_CHUNK_SIZE 8192

#define GEDIT_DOCUMENT_LOADER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						  GEDIT_TYPE_DOCUMENT_LOADER,            \
						  GeditDocumentLoaderPrivate))

static void	async_close_cb (GnomeVFSAsyncHandle *handle,
				GnomeVFSResult       result,
				gpointer             data);


struct _GeditDocumentLoaderPrivate
{
	GeditDocument		 *document;

	gboolean 		  used;
	
	/* Info on the current file */
	gchar			 *uri;
	const GeditEncoding      *encoding;

	GnomeVFSURI              *vfs_uri;

	GnomeVFSFileInfo	 *info;
	GnomeVFSFileSize	  bytes_read;

	/* Handle for local files */
	gint                      fd;
	gchar			 *local_file_name;
	
	/* Handle for remote files */
	GnomeVFSAsyncHandle 	 *handle;
	GnomeVFSAsyncHandle      *info_handle;

	gchar                    *buffer;

	const GeditEncoding      *auto_detected_encoding;

	GError                   *error;
};

G_DEFINE_TYPE(GeditDocumentLoader, gedit_document_loader, G_TYPE_OBJECT)

/* Signals */

enum {
	LOADING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Properties */

enum
{
	PROP_0,
	PROP_DOCUMENT,
};

static void
gedit_document_loader_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GeditDocumentLoader *dl = GEDIT_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_return_if_fail (dl->priv->document == NULL);
			dl->priv->document = g_value_get_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_loader_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GeditDocumentLoader *dl = GEDIT_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value,
					    dl->priv->document);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;		
	}
}

static void
gedit_document_loader_finalize (GObject *object)
{
	GeditDocumentLoaderPrivate *priv = GEDIT_DOCUMENT_LOADER (object)->priv;

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
	
	g_free (priv->uri);

	if (priv->info)
		gnome_vfs_file_info_unref (priv->info);


	g_free (priv->local_file_name);
	g_free (priv->buffer);

	if (priv->vfs_uri)
		gnome_vfs_uri_unref (priv->vfs_uri);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_document_loader_parent_class)->finalize (object);
}

static void
gedit_document_loader_class_init (GeditDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_document_loader_finalize;
	object_class->get_property = gedit_document_loader_get_property;
	object_class->set_property = gedit_document_loader_set_property;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							 "Document",
							 "The GeditDocument this GeditDocumentLoader is associated with",
							 GEDIT_TYPE_DOCUMENT,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT_ONLY));

	signals[LOADING] =
   		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentLoaderClass, loading),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);

	g_type_class_add_private (object_class, sizeof(GeditDocumentLoaderPrivate));
}

static void
gedit_document_loader_init (GeditDocumentLoader *loader)
{
	loader->priv = GEDIT_DOCUMENT_LOADER_GET_PRIVATE (loader);

	loader->priv->used = FALSE;

	loader->priv->fd = -1;

	loader->priv->error = NULL;
}

GeditDocumentLoader *
gedit_document_loader_new (GeditDocument *doc)
{
	GeditDocumentLoader *dl;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	dl = GEDIT_DOCUMENT_LOADER (g_object_new (GEDIT_TYPE_DOCUMENT_LOADER, 
						  "document", doc,
						  NULL));

	return dl;						  
}

static void
insert_text_in_document (GeditDocumentLoader *loader,
			 const gchar         *text,
			 gint                 len)
{
	gtk_source_buffer_begin_not_undoable_action (
				GTK_SOURCE_BUFFER (loader->priv->document));

	/* If the last char is a newline, don't add it to the buffer
	(otherwise GtkTextView shows it as an empty line). See bug #324942. */
	if (text[len-1] == '\n')
		len--;

	/* Insert text in the buffer */
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (loader->priv->document), 
				  text, 
				  len);

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (loader->priv->document),
				      FALSE);

	gtk_source_buffer_end_not_undoable_action (
				GTK_SOURCE_BUFFER (loader->priv->document));
}

static gboolean
update_document_contents (GeditDocumentLoader  *loader, 
		          const gchar          *file_contents,
			  gint                  file_size,
			  GError              **error)
{
	gedit_debug (DEBUG_LOADER);

	g_return_val_if_fail (file_size > 0, FALSE);
	g_return_val_if_fail (file_contents != NULL, FALSE);

	if (loader->priv->encoding == gedit_encoding_get_utf8 ())
	{
		if (g_utf8_validate (file_contents, file_size, NULL))
		{
			insert_text_in_document (loader,
						 file_contents,
						 file_size);
			
			return TRUE;
		}
		else
		{
			g_set_error (error,
				     G_CONVERT_ERROR, 
				     G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				     "The file you are trying to open contains an invalid byte sequence.");
				     
			return FALSE;
		}
	}
	else
	{
		GError *conv_error = NULL;
		gchar *converted_text = NULL;
		gsize new_len = file_size;

		if (loader->priv->encoding == NULL)
		{
			/* Autodetecting the encoding: first try with the encoding
			stored in the metadata, if any */

			const GeditEncoding *enc;
			gchar *charset;

			charset = gedit_metadata_manager_get (loader->priv->uri, "encoding");

			if (charset != NULL)
			{
				enc = gedit_encoding_get_from_charset (charset);

				if (enc != NULL)
				{	
					converted_text = gedit_convert_to_utf8 (
									file_contents,
									file_size,
									&enc,
									&new_len,
									NULL);

					if (converted_text != NULL)
						loader->priv->auto_detected_encoding = enc;
				}

				g_free (charset);
			}
		}

		if (converted_text == NULL)				
		{
			loader->priv->auto_detected_encoding = loader->priv->encoding;

			converted_text = gedit_convert_to_utf8 (
							file_contents,
							file_size,
							&loader->priv->auto_detected_encoding,
							&new_len,
							&conv_error);
		}

		if (converted_text == NULL)
		{
			g_return_val_if_fail (conv_error != NULL, FALSE);

			g_propagate_error (error, conv_error);
	
			return FALSE;
		}
		else
		{
			insert_text_in_document (loader,
						 converted_text,
						 new_len);

			g_free (converted_text);

			return TRUE;
		}
	}

	g_return_val_if_reached (FALSE);
}

/* The following function has been copied from gnome-vfs 
   (modules/file-method.c) file */
static void
get_access_info (GnomeVFSFileInfo *file_info,
              	 const gchar *full_name)
{
	/* FIXME: should check errno after calling access because we don't
	 * want to set valid_fields if something bad happened during one
	 * of the access calls
	 */
	if (g_access (full_name, W_OK) == 0) 
		file_info->permissions |= GNOME_VFS_PERM_ACCESS_WRITABLE;
		
	/*	
	 * We don't need to know if a local file is readable or 
	 * executable so I have commented the followig code to avoid 
	 * multiple g_access calls - Paolo (Oct. 18, 2005) 
	 */

	/*	 
	 *	if (g_access (full_name, R_OK) == 0) 
	 *		file_info->permissions |= GNOME_VFS_PERM_ACCESS_READABLE;
	 *
	 * #ifdef G_OS_WIN32
	 *	if (g_file_test (full_name, G_FILE_TEST_IS_EXECUTABLE))
	 *		file_info->permissions |= GNOME_VFS_PERM_ACCESS_EXECUTABLE;
	 * #else 
	 *	if (g_access (full_name, X_OK) == 0)
	 *		file_info->permissions |= GNOME_VFS_PERM_ACCESS_EXECUTABLE;
	 * #endif 
	 */

	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_ACCESS;
}

/* The following function has been copied from gnome-vfs 
   (gnome-vfs-module-shared.c) file */
static void
stat_to_file_info (GnomeVFSFileInfo *file_info,
		   const struct stat *statptr)
{
	if (S_ISDIR (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
	else if (S_ISCHR (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE;
	else if (S_ISBLK (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_BLOCK_DEVICE;
	else if (S_ISFIFO (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_FIFO;
	else if (S_ISSOCK (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_SOCKET;
	else if (S_ISREG (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	else if (S_ISLNK (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK;
	else
		file_info->type = GNOME_VFS_FILE_TYPE_UNKNOWN;

	file_info->permissions
		= statptr->st_mode & (GNOME_VFS_PERM_USER_ALL
				      | GNOME_VFS_PERM_GROUP_ALL
				      | GNOME_VFS_PERM_OTHER_ALL
				      | GNOME_VFS_PERM_SUID
				      | GNOME_VFS_PERM_SGID
				      | GNOME_VFS_PERM_STICKY);

	file_info->device = statptr->st_dev;
	file_info->inode = statptr->st_ino;

	file_info->link_count = statptr->st_nlink;

	file_info->uid = statptr->st_uid;
	file_info->gid = statptr->st_gid;

	file_info->size = statptr->st_size;
	file_info->block_count = statptr->st_blocks;
	file_info->io_block_size = statptr->st_blksize;
	if (file_info->io_block_size > 0 &&
	    file_info->io_block_size < 4096) {
		/* Never use smaller block than 4k,
		   should probably be pagesize.. */
		file_info->io_block_size = 4096;
	}

	file_info->atime = statptr->st_atime;
	file_info->ctime = statptr->st_ctime;
	file_info->mtime = statptr->st_mtime;

	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE |
	  GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS | GNOME_VFS_FILE_INFO_FIELDS_FLAGS |
	  GNOME_VFS_FILE_INFO_FIELDS_DEVICE | GNOME_VFS_FILE_INFO_FIELDS_INODE |
	  GNOME_VFS_FILE_INFO_FIELDS_LINK_COUNT | GNOME_VFS_FILE_INFO_FIELDS_SIZE |
	  GNOME_VFS_FILE_INFO_FIELDS_BLOCK_COUNT | GNOME_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE |
	  GNOME_VFS_FILE_INFO_FIELDS_ATIME | GNOME_VFS_FILE_INFO_FIELDS_MTIME |
	  GNOME_VFS_FILE_INFO_FIELDS_CTIME;
}

static void
load_completed_or_failed (GeditDocumentLoader *loader)
{
	/* the object will be unrefed in the callback of the loading
	 * signal, so we need to prevent finalization.
	 */
	g_object_ref (loader);
	
	g_signal_emit (loader, 
		       signals[LOADING],
		       0,
		       TRUE, /* completed */
		       loader->priv->error);

	if (loader->priv->error == NULL)
		gedit_debug_message (DEBUG_LOADER, "load completed");
	else
		gedit_debug_message (DEBUG_LOADER, "load failed");

	g_object_unref (loader);		       
}

/* ----------- local files ----------- */

#define MAX_MIME_SNIFF_SIZE 4096

static gboolean
load_local_file_real (GeditDocumentLoader *loader)
{
	struct stat statbuf;
	GnomeVFSResult result;
	gint ret;

	g_return_val_if_fail (loader->priv->fd != -1, FALSE);

	if (fstat (loader->priv->fd, &statbuf) != 0) 
	{
		result = gnome_vfs_result_from_errno ();

		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto done;
	}

	/* not a regular file */
	if (!S_ISREG (statbuf.st_mode))
	{
		if (S_ISDIR (statbuf.st_mode))
		{
			g_set_error (&loader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GNOME_VFS_ERROR_IS_DIRECTORY,
				     gnome_vfs_result_to_string (GNOME_VFS_ERROR_IS_DIRECTORY));
		}
		else
		{
			g_set_error (&loader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE,
				     "Not a regular file");
		}

		goto done;
	}

	loader->priv->info = gnome_vfs_file_info_new ();
	stat_to_file_info (loader->priv->info, &statbuf);
	GNOME_VFS_FILE_INFO_SET_LOCAL (loader->priv->info, TRUE);
	get_access_info (loader->priv->info, loader->priv->local_file_name);
	
	if (loader->priv->info->size == 0)
	{
		if (loader->priv->encoding == NULL)
			loader->priv->auto_detected_encoding = gedit_encoding_get_current ();

		/* guessing the mime from the filename is up to the caller */
	}
	else
	{
		gchar *mapped_file;
		const gchar *mime_type;

		/* CHECK: should we lock the file */		
		mapped_file = mmap (0, /* start */
				    loader->priv->info->size, 
				    PROT_READ,
				    MAP_PRIVATE, /* flags */
				    loader->priv->fd,
				    0 /* offset */);
				    
		if (mapped_file == MAP_FAILED)
		{
			gedit_debug_message (DEBUG_LOADER, "mmap failed");

			result = gnome_vfs_result_from_errno ();

			g_set_error (&loader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			goto done;
		}
		
		loader->priv->bytes_read = loader->priv->info->size;
		
		if (!update_document_contents (loader,
					       mapped_file,
					       loader->priv->info->size,
					       &loader->priv->error))
		{
			ret = munmap (mapped_file, loader->priv->info->size);
			if (ret != 0)
				g_warning ("File '%s' has not been correctly unmapped: %s",
					   loader->priv->uri,
					   strerror (errno));

			goto done;
		}

		mime_type = gnome_vfs_get_mime_type_for_name_and_data (loader->priv->local_file_name,
				mapped_file,
				MIN (loader->priv->bytes_read, MAX_MIME_SNIFF_SIZE));

		if ((mime_type != NULL) &&
		     strcmp (mime_type, GNOME_VFS_MIME_TYPE_UNKNOWN) != 0)
		{
			loader->priv->info->mime_type = g_strdup (mime_type);
			loader->priv->info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		}

		ret = munmap (mapped_file, loader->priv->info->size);

		if (ret != 0)
			g_warning ("File '%s' has not been correctly unmapped: %s",
				   loader->priv->uri,
				   strerror (errno));
	}

	ret = close (loader->priv->fd);

	if (ret != 0)
		g_warning ("File '%s' has not been correctly closed: %s",
			   loader->priv->uri,
			   strerror (errno));

	loader->priv->fd = -1;

 done:
	load_completed_or_failed (loader);
	
	return FALSE;
}

static gboolean
open_local_failed (GeditDocumentLoader *loader)
{
	load_completed_or_failed (loader);

	/* stop the timeout */
	return FALSE;
}

static void
load_local_file (GeditDocumentLoader *loader,
		 const gchar         *fname)
{
	gedit_debug (DEBUG_LOADER);

	g_signal_emit (loader,
		       signals[LOADING],
		       0,
		       FALSE,
		       NULL);

	loader->priv->fd = open (fname, O_RDONLY);
	if (loader->priv->fd == -1)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		g_timeout_add_full (G_PRIORITY_HIGH,
				    0,
				    (GSourceFunc) open_local_failed,
				    loader,
				    NULL);
				    
		return;			    
	}

	g_free (loader->priv->local_file_name);
	loader->priv->local_file_name = g_strdup (fname);

	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    (GSourceFunc) load_local_file_real,
			    loader,
			    NULL);
}

/* ----------- remote files ----------- */

static void
async_close_cb (GnomeVFSAsyncHandle *handle,
		GnomeVFSResult       result,
		gpointer             data)
{
	/* nothing to do... no point in reporting an error */
}

static void
remote_load_completed_or_failed (GeditDocumentLoader *loader)
{
	/* free the buffer and close the handle */
	gnome_vfs_async_close (loader->priv->handle,
			       async_close_cb,
			       NULL);
			       
	loader->priv->handle = NULL;

	g_free (loader->priv->buffer);
	loader->priv->buffer = NULL;

	load_completed_or_failed (loader);
}

/* prototype, because they call each other... isn't C lovely */
static void	read_file_chunk		(GeditDocumentLoader *loader);

static void
async_read_cb (GnomeVFSAsyncHandle *handle,
	       GnomeVFSResult       result,
	       gpointer             buffer,
	       GnomeVFSFileSize     bytes_requested,
	       GnomeVFSFileSize     bytes_read,
	       gpointer             data)
{
	GeditDocumentLoader *loader = GEDIT_DOCUMENT_LOADER (data);

	gedit_debug (DEBUG_LOADER);

	/* reality checks. */
	g_return_if_fail (bytes_requested == READ_CHUNK_SIZE);
	g_return_if_fail (loader->priv->handle == handle);
	g_return_if_fail (loader->priv->buffer + loader->priv->bytes_read == buffer);
	g_return_if_fail (bytes_read <= bytes_requested);

	/* error occurred */
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF)
	{
		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		remote_load_completed_or_failed (loader);

		return;
	}

	/* Check for the extremely unlikely case where the file size overflows. */
	if (loader->priv->bytes_read + bytes_read < loader->priv->bytes_read)
	{
		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_TOO_BIG,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_TOO_BIG));

		remote_load_completed_or_failed (loader);

		return;
	}

	/* Bump the size. */
	loader->priv->bytes_read += bytes_read;

	/* end of the file, we are done! */
	if (bytes_read == 0 || result != GNOME_VFS_OK)
	{
		if (loader->priv->bytes_read == 0)
		{
			if (loader->priv->encoding == NULL)
				loader->priv->auto_detected_encoding = gedit_encoding_get_current ();
		}
		else
		{
			update_document_contents (loader,
						  loader->priv->buffer,
						  loader->priv->bytes_read,
						  &loader->priv->error);
		}

		remote_load_completed_or_failed (loader);

		return;
	}

	/* otherwise emit progress and read some more */

	/* note that this signal blocks the read... check if it isn't
	 * a performance problem
	 */
	g_signal_emit (loader,
		       signals[LOADING],
		       0,
		       FALSE,
		       NULL);

	read_file_chunk (loader);
}

static void
read_file_chunk (GeditDocumentLoader *loader)
{
	loader->priv->buffer = g_realloc (loader->priv->buffer,
					  loader->priv->bytes_read + READ_CHUNK_SIZE);

	gnome_vfs_async_read (loader->priv->handle,
			      loader->priv->buffer + loader->priv->bytes_read,
			      READ_CHUNK_SIZE,
			      async_read_cb,
			      loader);
}

static void
remote_get_info_cb (GnomeVFSAsyncHandle *handle,
		    GList               *results,
		    gpointer             data)
{
	GeditDocumentLoader *loader = GEDIT_DOCUMENT_LOADER (data);
	GnomeVFSGetFileInfoResult *info_result;

	gedit_debug (DEBUG_LOADER);

	/* assert that the list has one and only one item */
	g_return_if_fail (results != NULL && results->next == NULL);

	loader->priv->info_handle = NULL;
	
	info_result = (GnomeVFSGetFileInfoResult *) results->data;
	g_return_if_fail (info_result != NULL);

	if (info_result->result != GNOME_VFS_OK)
	{
		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     info_result->result,
			     gnome_vfs_result_to_string (info_result->result));

		remote_load_completed_or_failed (loader);

		return;
	}

	/* CHECK: ref is necessary, right? or the info will go away... */
	loader->priv->info = info_result->file_info;
	gnome_vfs_file_info_ref (loader->priv->info);

	/* if it's not a regular file, error out... */
	if (info_result->file_info->type != GNOME_VFS_FILE_TYPE_REGULAR)
	{
		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_GENERIC, // FIXME
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_GENERIC));

		remote_load_completed_or_failed (loader);

		return;
	}

	/* start reading */
	read_file_chunk (loader);
}

static void
async_open_callback (GnomeVFSAsyncHandle *handle,
		     GnomeVFSResult       result,
		     GeditDocumentLoader *loader)
{
	GList *uri_list = NULL;

	gedit_debug (DEBUG_LOADER);

	g_return_if_fail (loader->priv->handle == handle);

	if (result != GNOME_VFS_OK)
	{
		g_set_error (&loader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		/* in this case we don't need to close the handle */
		load_completed_or_failed (loader);

		return;
	}

	/* get the file info after open to avoid races... this really
	 * should be async_get_file_info_from_handle (fstat equivalent)
	 * but gnome-vfs lacks that.
	 */

	uri_list = g_list_prepend (uri_list, loader->priv->vfs_uri);

	gnome_vfs_async_get_file_info (&loader->priv->info_handle,
				       uri_list,
				       GNOME_VFS_FILE_INFO_DEFAULT |
				       GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
				       GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
				       GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
				       GNOME_VFS_PRIORITY_MAX,
				       remote_get_info_cb,
				       loader);

	g_list_free (uri_list);
}

static void
load_remote_file (GeditDocumentLoader *loader)
{
	gedit_debug (DEBUG_LOADER);

	g_return_if_fail (loader->priv->handle == NULL);

	/* loading start */
	g_signal_emit (loader,
		       signals[LOADING],
		       0,
		       FALSE,
		       NULL);

	gnome_vfs_async_open_uri (&loader->priv->handle,
				  loader->priv->vfs_uri,
				  GNOME_VFS_OPEN_READ,
				  GNOME_VFS_PRIORITY_MAX,
				  (GnomeVFSAsyncOpenCallback) async_open_callback,
				  loader);
}

/* ---------- public api ---------- */

static gboolean
vfs_uri_new_failed (GeditDocumentLoader *loader)
{
	load_completed_or_failed (loader);

	/* stop the timeout */
	return FALSE;
}

/* If enconding == NULL, the encoding will be autodetected */
void
gedit_document_loader_load (GeditDocumentLoader *loader,
			    const gchar         *uri,
			    const GeditEncoding *encoding)
{
	gchar *local_path;

	gedit_debug (DEBUG_LOADER);

	g_return_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader));
	g_return_if_fail (uri != NULL);

	/* the loader can be used just once, then it must be thrown away */
	g_return_if_fail (loader->priv->used == FALSE);
	loader->priv->used = TRUE;

	/* vfs_uri may be NULL for some valid but unsupported uris */
	loader->priv->vfs_uri = gnome_vfs_uri_new (uri);
	if (loader->priv->vfs_uri == NULL)
	{
		g_set_error (&loader->priv->error,
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

	loader->priv->encoding = encoding;

	loader->priv->uri = g_strdup (uri);

	local_path = gnome_vfs_get_local_path_from_uri (uri);
	if (local_path != NULL)
	{
		load_local_file (loader, local_path);
		g_free (local_path);
	}
	else
	{
		load_remote_file (loader);
	}
}

/* Returns STDIN_URI if loading from stdin */
const gchar *
gedit_document_loader_get_uri (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	return loader->priv->uri;
}

/* it may return NULL, it's up to gedit-document handle it */
const gchar *
gedit_document_loader_get_mime_type (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	if (loader->priv->info &&
	    (loader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE))
		return loader->priv->info->mime_type;
	else
		return NULL;
}

time_t
gedit_document_loader_get_mtime (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);

	if (loader->priv->info &&
	    (loader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME))
		return loader->priv->info->mtime;
	else
		return 0;
}

/* Returns 0 if file size is unknown */
GnomeVFSFileSize
gedit_document_loader_get_file_size (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);

	if (loader->priv->info == NULL)
		return 0;

	return loader->priv->info->size;
}									 

GnomeVFSFileSize
gedit_document_loader_get_bytes_read (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);

	return loader->priv->bytes_read;
}

gboolean 
gedit_document_loader_cancel (GeditDocumentLoader *loader)
{
	gedit_debug (DEBUG_LOADER);

	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), FALSE);

	if (loader->priv->handle == NULL)
		return FALSE;

	if (loader->priv->info_handle != NULL)
	{
		gnome_vfs_async_cancel (loader->priv->info_handle);
		gnome_vfs_async_close (loader->priv->info_handle,
				       async_close_cb, 
				       NULL);
	}

	gnome_vfs_async_cancel (loader->priv->handle);

	g_set_error (&loader->priv->error,
		     GEDIT_DOCUMENT_ERROR,
		     GNOME_VFS_ERROR_CANCELLED,
		     gnome_vfs_result_to_string (GNOME_VFS_ERROR_CANCELLED));

	remote_load_completed_or_failed (loader);

	return TRUE;
}

/* In the case the loader does not know if the file is readonly, for example 
   for most remote files, the function returns FALSE, so that we can try writing
   and if needed handle the error. */
gboolean
gedit_document_loader_get_readonly (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), FALSE);

	if (!gedit_utils_uri_has_writable_scheme (loader->priv->uri))
		return TRUE;

	if (loader->priv->info &&
	    (loader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_ACCESS))
		return (loader->priv->info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) ? FALSE : TRUE;
	else
		return FALSE;
}

const GeditEncoding *
gedit_document_loader_get_encoding (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	if (loader->priv->encoding != NULL)
		return loader->priv->encoding;
		
	g_return_val_if_fail (loader->priv->auto_detected_encoding != NULL, 
			      gedit_encoding_get_current ());
			  
	return loader->priv->auto_detected_encoding;
}
