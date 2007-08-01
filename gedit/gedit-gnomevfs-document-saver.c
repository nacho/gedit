/*
 * gedit-gnomevfs-document-saver.c
 * This file is part of gedit
 *
 * Copyright (C) 2005-2006 - Paolo Borelli and Paolo Maggi
 * Copyright (C) 2007 - Paolo Borelli, Paolo Maggi, Steve Frécinaux
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
#include <glib/gfileutils.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-gnomevfs-document-saver.h"
#include "gedit-debug.h"

#define GEDIT_GNOMEVFS_DOCUMENT_SAVER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
							  GEDIT_TYPE_GNOMEVFS_DOCUMENT_SAVER, \
							  GeditGnomeVFSDocumentSaverPrivate))

static void		 gedit_gnomevfs_document_saver_save			(GeditDocumentSaver *saver,
										 time_t              old_mtime);
static const gchar 	*gedit_gnomevfs_document_saver_get_mime_type		(GeditDocumentSaver *saver);
static time_t		 gedit_gnomevfs_document_saver_get_mtime		(GeditDocumentSaver *saver);
static GnomeVFSFileSize	 gedit_gnomevfs_document_saver_get_file_size		(GeditDocumentSaver *saver);
static GnomeVFSFileSize	 gedit_gnomevfs_document_saver_get_bytes_written	(GeditDocumentSaver *saver);

struct _GeditGnomeVFSDocumentSaverPrivate
{
	time_t                    doc_mtime;
	gchar                    *mime_type; //CHECK use FileInfo instead?

	GnomeVFSFileSize	  size;
	GnomeVFSFileSize	  bytes_written;

	/* temp data for remote files */
	GnomeVFSURI		 *vfs_uri;
	GnomeVFSAsyncHandle	 *handle;
	GnomeVFSAsyncHandle      *info_handle;
	gint			  tmpfd;
	gchar			 *tmp_fname;
	GnomeVFSFileInfo	 *orig_info; /* used to restore permissions */

	GError                   *error;
};

G_DEFINE_TYPE(GeditGnomeVFSDocumentSaver, gedit_gnomevfs_document_saver, GEDIT_TYPE_DOCUMENT_SAVER)

static void
gedit_gnomevfs_document_saver_finalize (GObject *object)
{
	GeditGnomeVFSDocumentSaverPrivate *priv = GEDIT_GNOMEVFS_DOCUMENT_SAVER (object)->priv;

	if (priv->vfs_uri)
		gnome_vfs_uri_unref (priv->vfs_uri);

	g_free (priv->mime_type);
	g_free (priv->tmp_fname);

	if (priv->orig_info)
		gnome_vfs_file_info_unref (priv->orig_info);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_gnomevfs_document_saver_parent_class)->finalize (object);
}

static void 
gedit_gnomevfs_document_saver_class_init (GeditGnomeVFSDocumentSaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditDocumentSaverClass *saver_class = GEDIT_DOCUMENT_SAVER_CLASS (klass);

	object_class->finalize = gedit_gnomevfs_document_saver_finalize;

	saver_class->save = gedit_gnomevfs_document_saver_save;
	saver_class->get_mime_type = gedit_gnomevfs_document_saver_get_mime_type;
	saver_class->get_mtime = gedit_gnomevfs_document_saver_get_mtime;
	saver_class->get_file_size = gedit_gnomevfs_document_saver_get_file_size;
	saver_class->get_bytes_written = gedit_gnomevfs_document_saver_get_bytes_written;

	g_type_class_add_private (object_class, sizeof(GeditGnomeVFSDocumentSaverPrivate));
}

static void
gedit_gnomevfs_document_saver_init (GeditGnomeVFSDocumentSaver *gvsaver)
{
	gvsaver->priv = GEDIT_GNOMEVFS_DOCUMENT_SAVER_GET_PRIVATE (gvsaver);

	gvsaver->priv->tmpfd = -1;

	gvsaver->priv->error = NULL;
}

static void
remote_save_completed_or_failed (GeditGnomeVFSDocumentSaver *gvsaver)
{
	/* we can now close and unlink the tmp file */
	close (gvsaver->priv->tmpfd);
	unlink (gvsaver->priv->tmp_fname);

	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (gvsaver),
				     TRUE,
				     gvsaver->priv->error);
}

static void
remote_get_info_cb (GnomeVFSAsyncHandle        *handle,
		    GList                      *results,
		    GeditGnomeVFSDocumentSaver *gvsaver)
{
	GnomeVFSGetFileInfoResult *info_result;

	gedit_debug (DEBUG_SAVER);

	/* assert that the list has one and only one item */
	g_return_if_fail (results != NULL && results->next == NULL);

	info_result = (GnomeVFSGetFileInfoResult *) results->data;
	g_return_if_fail (info_result != NULL);

	if (info_result->result != GNOME_VFS_OK)
	{
		g_set_error (&gvsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     info_result->result,
			     gnome_vfs_result_to_string (info_result->result));

		remote_save_completed_or_failed (gvsaver);

		return;
	}

	if (info_result->file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME)
		gvsaver->priv->doc_mtime = info_result->file_info->mtime;

	if (info_result->file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE)
	{
		g_free (gvsaver->priv->mime_type);
		gvsaver->priv->mime_type = g_strdup (info_result->file_info->mime_type);
	}

	remote_save_completed_or_failed (gvsaver);
}

static void
manage_completed_phase (GeditGnomeVFSDocumentSaver *gvsaver)
{
	/* In the complete phase we emit the "saving" signal before managing
	   the phase itself since the saver could be destroyed during these
	   operations */
	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (gvsaver), FALSE, NULL);

	if (gvsaver->priv->error != NULL)
	{
		/* We aborted the xfer after an error */
		remote_save_completed_or_failed (gvsaver);
	}
	else
	{
		/* Transfer done!
		 * Restore the permissions if needed and then refetch
		 * info on our newly written file to get the mime etc */

		GList *uri_list = NULL;

		/* Try is not as paranoid as the local version (GID)... it would take
		 * yet another stat to do it...
		 */
		if (gvsaver->priv->orig_info != NULL &&
		    (gvsaver->priv->orig_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS))
		{
			gnome_vfs_set_file_info_uri (gvsaver->priv->vfs_uri,
		     				     gvsaver->priv->orig_info,
		     				     GNOME_VFS_SET_FILE_INFO_PERMISSIONS);

			// FIXME: for now is a blind try... do we want to error check?
		}

		uri_list = g_list_prepend (uri_list, gvsaver->priv->vfs_uri);

		gnome_vfs_async_get_file_info (&gvsaver->priv->info_handle,
					       uri_list,
					       GNOME_VFS_FILE_INFO_DEFAULT |
					       GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					       GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
					       GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
					       GNOME_VFS_PRIORITY_MAX,
					       (GnomeVFSAsyncGetFileInfoCallback) remote_get_info_cb,
					       gvsaver);
		g_list_free (uri_list);
	}
}

static gint
async_xfer_ok (GnomeVFSXferProgressInfo   *progress_info,
	       GeditGnomeVFSDocumentSaver *gvsaver)
{
	gedit_debug (DEBUG_SAVER);

	switch (progress_info->phase)
	{
	case GNOME_VFS_XFER_PHASE_INITIAL:
		break;
		
	case GNOME_VFS_XFER_CHECKING_DESTINATION:
		{
			GnomeVFSFileInfo *orig_info;
			GnomeVFSResult res;

			/* we need to retrieve info ourselves too, since xfer
			 * doesn't allow to access it :(
			 * If that was not enough we need to do it sync or we are going
			 * to mess everything up
			 */
			orig_info = gnome_vfs_file_info_new ();
			res = gnome_vfs_get_file_info_uri (gvsaver->priv->vfs_uri,
							   orig_info,
							   GNOME_VFS_FILE_INFO_DEFAULT |
							   GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

			if (res == GNOME_VFS_ERROR_NOT_FOUND)
			{
				/* ok, we are not overwriting, go on with the xfer */
				break;
			}

			if (res != GNOME_VFS_OK)
			{
				// CHECK: do we want to ignore the error and try to go on anyway?
				g_set_error (&gvsaver->priv->error,
					     GEDIT_DOCUMENT_ERROR,
					     res,
					     gnome_vfs_result_to_string (res));

				/* abort xfer */
				return 0;
			}


			/* check if someone else modified the file externally,
			 * except when "saving as", when saving a new doc (mtime = 0)
			 * or when the mtime check is explicitely disabled
			 */
			if (orig_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME)
			{
				if (gvsaver->priv->doc_mtime > 0 &&
				    orig_info->mtime != gvsaver->priv->doc_mtime &&
				    (GEDIT_DOCUMENT_SAVER (gvsaver)->flags & GEDIT_DOCUMENT_SAVE_IGNORE_MTIME) == 0)
				{
					g_set_error (&gvsaver->priv->error,
						     GEDIT_DOCUMENT_ERROR,
						     GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED,
						     "Externally modified");

					/* abort xfer */
					return 0;
				}
			}

			/* store the original file info, so that we can restore permissions */
			// FIXME: what about the case where we are usin "Save as" but overwriting a file... we don't want to restore perms
			if (orig_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS)
				gvsaver->priv->orig_info = orig_info;
		}
		break;
		
	case GNOME_VFS_XFER_PHASE_COLLECTING:
	case GNOME_VFS_XFER_PHASE_DELETESOURCE: // why do we get this phase??
		break;
		
	case GNOME_VFS_XFER_PHASE_READYTOGO:
		gvsaver->priv->size = progress_info->bytes_total;
		break;
		
	case GNOME_VFS_XFER_PHASE_OPENSOURCE:
	case GNOME_VFS_XFER_PHASE_OPENTARGET:
	case GNOME_VFS_XFER_PHASE_COPYING:
	case GNOME_VFS_XFER_PHASE_WRITETARGET:
	case GNOME_VFS_XFER_PHASE_CLOSETARGET:
		if (progress_info->bytes_copied > 0)
			gvsaver->priv->bytes_written = MIN (progress_info->total_bytes_copied,
							    progress_info->bytes_total);
		break;
		
	case GNOME_VFS_XFER_PHASE_FILECOMPLETED:
	case GNOME_VFS_XFER_PHASE_CLEANUP:
		break;
		
	case GNOME_VFS_XFER_PHASE_COMPLETED:
		manage_completed_phase (gvsaver);
		
		/* We return here in order to not emit a "saving" signal on
		   a potentially invald saver */
		return 0;
		
	/* Phases we don't expect to see */
	case GNOME_VFS_XFER_PHASE_SETATTRIBUTES:
	case GNOME_VFS_XFER_PHASE_CLOSESOURCE:
	case GNOME_VFS_XFER_PHASE_MOVING:
	case GNOME_VFS_XFER_PHASE_READSOURCE:
	default:
		g_return_val_if_reached (0);
	}

	/* signal the progress */
	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (gvsaver), FALSE, NULL);

	return 1;
}

static gint
async_xfer_error (GnomeVFSXferProgressInfo   *progress_info,
		  GeditGnomeVFSDocumentSaver *gvsaver)
{
	if (gvsaver->priv->error == NULL)
	{
		/* We set the error and then abort.
		 * Note that remote_save_completed_or_failed ()
		 * will then be called when the xfer state machine goes
		 * in the COMPLETED state.
		 */
		gedit_debug_message (DEBUG_SAVER, 
				     "Set the error: \"%s\"",
				     gnome_vfs_result_to_string (progress_info->vfs_status));
		 
		g_set_error (&gvsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     progress_info->vfs_status,
			     gnome_vfs_result_to_string (progress_info->vfs_status));
	}
	else
	{
		gedit_debug_message (DEBUG_SAVER, 
				     "Error already set.\n"
				     "The new (skipped) error is: \"%s\"",
				     gnome_vfs_result_to_string (progress_info->vfs_status));

		g_return_val_if_fail (progress_info->vfs_status == GNOME_VFS_ERROR_INTERRUPTED,
				      GNOME_VFS_XFER_ERROR_ACTION_ABORT);
	}

	return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
}

static gint
async_xfer_progress (GnomeVFSAsyncHandle        *handle,
		     GnomeVFSXferProgressInfo   *progress_info,
		     GeditGnomeVFSDocumentSaver *gvsaver)
{
	gedit_debug_message (DEBUG_SAVER, "xfer phase: %d", progress_info->phase);
	gedit_debug_message (DEBUG_SAVER, "xfer status: %d", progress_info->status);
	
	switch (progress_info->status)
	{
	case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
		return async_xfer_ok (progress_info, gvsaver);
	case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		return async_xfer_error (progress_info, gvsaver);

	/* we should never go in these */
	case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
	case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
	default:
		g_return_val_if_reached (0);
	}
}

static gboolean
save_remote_file_real (GeditGnomeVFSDocumentSaver *gvsaver)
{
	mode_t saved_umask;
	gchar *tmp_uri;
	GnomeVFSURI *tmp_vfs_uri;
	GList *source_uri_list = NULL;
	GList *dest_uri_list = NULL;
	GnomeVFSResult result;

	gedit_debug (DEBUG_SAVER);

	/* For remote files we use the following strategy:
	 * we save to a local temp file and then transfer it
	 * over to the requested location asyncronously.
	 * There is no backup of the original remote file.
	 */

	/* We set the umask because some (buggy) implementations
	 * of mkstemp() use permissions 0666 and we want 0600.
	 */
	saved_umask = umask (0077);
	gvsaver->priv->tmpfd = g_file_open_tmp (".gedit-save-XXXXXX",
						&gvsaver->priv->tmp_fname,
						&gvsaver->priv->error);
	umask (saved_umask);

	if (gvsaver->priv->tmpfd == -1)
	{
		result = gnome_vfs_result_from_errno ();

		g_set_error (&gvsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		/* in this case no need to close the tmp file */
		gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (gvsaver),
					     FALSE, NULL);

		return FALSE;
	}

	tmp_uri = g_filename_to_uri (gvsaver->priv->tmp_fname,
				     NULL,
				     &gvsaver->priv->error);
	if (tmp_uri == NULL)
	{
		goto error;
	}

	tmp_vfs_uri = gnome_vfs_uri_new (tmp_uri);
	//needs error checking?

	g_free (tmp_uri);

	source_uri_list = g_list_prepend (source_uri_list, tmp_vfs_uri);
	dest_uri_list = g_list_prepend (dest_uri_list, gvsaver->priv->vfs_uri);

	if (!gedit_document_saver_write_document_contents (
						GEDIT_DOCUMENT_SAVER (gvsaver),
						gvsaver->priv->tmpfd,
			 			&gvsaver->priv->error))
	{
		goto error;
	}

	gedit_debug_message (DEBUG_SAVER, "Saved local copy, starting xfer");

	result = gnome_vfs_async_xfer (&gvsaver->priv->handle,
				       source_uri_list,
				       dest_uri_list,
				       GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_TARGET_DEFAULT_PERMS, // CHECK needs more thinking, follow symlinks etc... options are undocumented :(
				       GNOME_VFS_XFER_ERROR_MODE_QUERY,       /* We need to use QUERY otherwise we don't get errors */
				       GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE, /* We have already asked confirm (even if it is racy) */
				       GNOME_VFS_PRIORITY_DEFAULT,
				       (GnomeVFSAsyncXferProgressCallback) async_xfer_progress,
				       gvsaver,
				       NULL, NULL);

	gnome_vfs_uri_unref (tmp_vfs_uri);
	g_list_free (source_uri_list);
	g_list_free (dest_uri_list);

	if (result != GNOME_VFS_OK)
	{
		g_set_error (&gvsaver->priv->error,
		    	     GEDIT_DOCUMENT_ERROR,
		     	     result,
			     gnome_vfs_result_to_string (result));

		goto error;
	}

	/* No errors: stop the timeout */
	return FALSE;

 error:
	remote_save_completed_or_failed (gvsaver);

	/* stop the timeout */
	return FALSE;
}

static void
gedit_gnomevfs_document_saver_save (GeditDocumentSaver *saver,
				    time_t              old_mtime)
{
	GeditGnomeVFSDocumentSaver *gvsaver = GEDIT_GNOMEVFS_DOCUMENT_SAVER (saver);

	gvsaver->priv->doc_mtime = old_mtime;
	gvsaver->priv->vfs_uri = gnome_vfs_uri_new (saver->uri);

	/* saving start */
	gedit_document_saver_saving (saver, FALSE, NULL);

	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    (GSourceFunc) save_remote_file_real,
			    gvsaver,
			    NULL);
}

static const gchar *
gedit_gnomevfs_document_saver_get_mime_type (GeditDocumentSaver *saver)
{
	return GEDIT_GNOMEVFS_DOCUMENT_SAVER (saver)->priv->mime_type;
}

static time_t
gedit_gnomevfs_document_saver_get_mtime (GeditDocumentSaver *saver)
{
	return GEDIT_GNOMEVFS_DOCUMENT_SAVER (saver)->priv->doc_mtime;
}

static GnomeVFSFileSize
gedit_gnomevfs_document_saver_get_file_size (GeditDocumentSaver *saver)
{
	return GEDIT_GNOMEVFS_DOCUMENT_SAVER (saver)->priv->size;
}

static GnomeVFSFileSize
gedit_gnomevfs_document_saver_get_bytes_written (GeditDocumentSaver *saver)
{
	return GEDIT_GNOMEVFS_DOCUMENT_SAVER (saver)->priv->bytes_written;
}
