/*
 * gedit-document-saver.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Borelli and Paolo Maggi
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
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gfileutils.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-document-saver.h"
#include "gedit-debug.h"
#include "gedit-convert.h"
#include "gedit-metadata-manager.h"
#include "gedit-prefs-manager.h"
#include "gedit-marshal.h"
#include "gedit-utils.h"

#ifdef HAVE_LIBATTR
#include <attr/libattr.h>
#else
#define attr_copy_fd(x1, x2, x3, x4, x5, x6) (errno = ENOSYS, -1)
#endif

#define GEDIT_DOCUMENT_SAVER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						 GEDIT_TYPE_DOCUMENT_SAVER, \
						 GeditDocumentSaverPrivate))

struct _GeditDocumentSaverPrivate
{
	GeditDocument		 *document;

	gchar			 *uri;
	const GeditEncoding      *encoding;

	GeditDocumentSaveFlags    flags;

	gboolean		  keep_backup;
	gchar			 *backup_ext;
	gboolean                  backups_in_curr_dir;

	time_t                    doc_mtime;
	gchar                    *mime_type; //CHECK use FileInfo instead?

	GnomeVFSFileSize	  size;
	GnomeVFSFileSize	  bytes_written;

	/* temp data for local files */
	gint			  fd;
	gchar			 *local_path;

	/* temp data for remote files */
	GnomeVFSURI		 *vfs_uri;
	GnomeVFSAsyncHandle	 *handle;
	GnomeVFSAsyncHandle      *info_handle;
	gint			  tmpfd;
	gchar			 *tmp_fname;
	GnomeVFSFileInfo	 *orig_info; /* used to restore permissions */

	GError                   *error;
};

G_DEFINE_TYPE(GeditDocumentSaver, gedit_document_saver, G_TYPE_OBJECT)

/* Signals */

enum {
	SAVING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
gedit_document_saver_finalize (GObject *object)
{
	GeditDocumentSaverPrivate *priv = GEDIT_DOCUMENT_SAVER (object)->priv;

	g_free (priv->uri);

	if (priv->vfs_uri)
		gnome_vfs_uri_unref (priv->vfs_uri);

	g_free (priv->backup_ext);

	g_free (priv->local_path);
	g_free (priv->mime_type);
	g_free (priv->tmp_fname);

	if (priv->orig_info)
		gnome_vfs_file_info_unref (priv->orig_info);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_document_saver_parent_class)->finalize (object);
}

static void 
gedit_document_saver_class_init (GeditDocumentSaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_document_saver_finalize;

	signals[SAVING] =
   		g_signal_new ("saving",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentSaverClass, saving),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);

	g_type_class_add_private (object_class, sizeof(GeditDocumentSaverPrivate));
}

static void
gedit_document_saver_init (GeditDocumentSaver *saver)
{
	saver->priv = GEDIT_DOCUMENT_SAVER_GET_PRIVATE (saver);

	saver->priv->fd = -1;

	saver->priv->tmpfd = -1;

	saver->priv->error = NULL;
}

GeditDocumentSaver *
gedit_document_saver_new (GeditDocument *doc)
{
	GeditDocumentSaver *saver;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	saver = GEDIT_DOCUMENT_SAVER (g_object_new (GEDIT_TYPE_DOCUMENT_SAVER, NULL));

	saver->priv->document = doc;

	return saver;
}

/*
 * Write the document contents in fd.
 */
static gboolean
write_document_contents (gint                  fd,
			 GtkTextBuffer        *doc,
			 const GeditEncoding  *encoding,
			 GError              **error)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *contents;
	gsize len;
	gboolean add_cr;
	ssize_t written;
	gboolean res;

	gedit_debug (DEBUG_SAVER);

	gtk_text_buffer_get_bounds (doc, &start_iter, &end_iter);
	contents = gtk_text_buffer_get_slice (doc, &start_iter, &end_iter, TRUE);

	len = strlen (contents);

	if (len >= 1)
		add_cr = (*(contents + len - 1) != '\n');
	else
		add_cr = FALSE;

	if (encoding != gedit_encoding_get_utf8 ())
	{
		gchar *converted_contents;
		gsize new_len;

		converted_contents = gedit_convert_from_utf8 (contents, 
							      len, 
							      encoding,
							      &new_len,
							      error);
		g_free (contents);

		if (*error != NULL)
		{
			/* Conversion error */
			return FALSE;
		}
		else
		{
			contents = converted_contents;
			len = new_len;
		}
	}

	/* make sure we are at the start */
	res = (lseek (fd, 0, SEEK_SET) != -1);

	/* Truncate the file to 0, in case it was not empty */
	if (res)
		res = (ftruncate (fd, 0) == 0);

	/* Save the file content */
	if (res)
	{
		written = write (fd, contents, len);
		res = ((written != -1) && ((gsize) written == len));
	}

	/* Add \n at the end if needed */
	if (res && add_cr)
	{
		if (encoding != gedit_encoding_get_utf8 ())
		{
			gchar *converted_n = NULL;
			gsize n_len;

			converted_n = gedit_convert_from_utf8 ("\n", 
							       -1, 
							       encoding,
							       &n_len,
							       NULL);

			if (converted_n == NULL)
			{
				/* we do not error out for this */
				g_warning ("Cannot add '\\n' at the end of the file.");
			}
			else
			{
				written = write (fd, converted_n, n_len);
				res = ((written != -1) && ((gsize) written == n_len));
				g_free (converted_n);
			}
		}
		else
		{
			res = (write (fd, "\n", 1) == 1);
		}
	}

	g_free (contents);

	if (!res)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));
	}

	return res;
}

static void
save_completed_or_failed (GeditDocumentSaver *saver)
{
	/* the object will be unrefed in the callback of the saving
	 * signal, so we need to prevent finalization.
	 */
	g_object_ref (saver);

	g_signal_emit (saver, 
		       signals[SAVING],
		       0,
		       TRUE, /* completed */
		       saver->priv->error);

	if (saver->priv->error == NULL)
		gedit_debug_message (DEBUG_SAVER, "save completed");
	else
		gedit_debug_message (DEBUG_SAVER, "save failed");

	g_object_unref (saver);
}

static gchar *
get_backup_filename (GeditDocumentSaver *saver)
{
	gchar *fname;
	gchar *bak_ext = NULL;

	if ((saver->priv->backup_ext != NULL) &&
	    (strlen (saver->priv->backup_ext) > 0))
		bak_ext = saver->priv->backup_ext;
	else
		bak_ext = "~";

	fname = g_strconcat (saver->priv->local_path, bak_ext, NULL);

	/* If we are not going to keep the backup file and fname
	 * already exists, try to use another name.
	 * Change one character, just before the extension.
	 */
	if (!saver->priv->keep_backup &&
	    g_file_test (fname, G_FILE_TEST_EXISTS))
	{
		gchar *wp;

		wp = fname + strlen (fname) - 1 - strlen (bak_ext);
		g_return_val_if_fail (wp > fname, NULL);

		*wp = 'z';
		while ((*wp > 'a') && g_file_test (fname, G_FILE_TEST_EXISTS))
			--*wp;

		/* They all exist??? Must be something wrong. */
		if (*wp == 'a')
		{
			g_free (fname);
			fname = NULL;
		}
	}

	return fname;
}

/* like unlink, but doesn't fail if the file wasn't there at all */
static gboolean
remove_file (const gchar *name)
{
	gint res;

	res = unlink (name);

	return (res == 0) || ((res == -1) && (errno == ENOENT));
}

#define BUFSIZE	8192 /* size of normal write buffer */

static gboolean
copy_file_data (gint     sfd,
		gint     dfd,
		GError **error)
{
	gboolean ret = TRUE;
	GError *err = NULL;
	gpointer buffer;
	const gchar *write_buffer;
	ssize_t bytes_read;
	ssize_t bytes_to_write;
	ssize_t bytes_written;

	gedit_debug (DEBUG_SAVER);

	buffer = g_malloc (BUFSIZE);

	do
	{
		bytes_read = read (sfd, buffer, BUFSIZE);
		if (bytes_read == -1)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			g_set_error (&err,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			ret = FALSE;

			break;
		}

		bytes_to_write = bytes_read;
		write_buffer = buffer;

		do
		{
			bytes_written = write (dfd, write_buffer, bytes_to_write);
			if (bytes_written == -1)
			{
				GnomeVFSResult result = gnome_vfs_result_from_errno ();

				g_set_error (&err,
					     GEDIT_DOCUMENT_ERROR,
					     result,
					     gnome_vfs_result_to_string (result));

				ret = FALSE;

				break;
			}

			bytes_to_write -= bytes_written;
			write_buffer += bytes_written;
		}
		while (bytes_to_write > 0);

	} while ((bytes_read != 0) && (ret == TRUE));

	if (error)
		*error = err;

	return ret;
}

/* FIXME: this is ugly for multple reasons: it refetches all the info,
 * it doesn't use fd etc... we need something better, possibly in gnome-vfs
 * public api.
 */
static gchar *
get_slow_mime_type (const char *text_uri)
{
	GnomeVFSFileInfo *info;
	char *mime_type;
	GnomeVFSResult result;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (text_uri, info,
					  GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (info->mime_type == NULL || result != GNOME_VFS_OK) {
		mime_type = NULL;
	} else {
		mime_type = g_strdup (info->mime_type);
	}
	gnome_vfs_file_info_unref (info);

	return mime_type;
}

/* ----------- local files ----------- */

#ifdef HAVE_LIBATTR
/* Save everything: user/root xattrs, SELinux, ACLs. */
static int
all_xattrs (const char *xattr G_GNUC_UNUSED,
	    struct error_context *err G_GNUC_UNUSED)
{
	return 1;
}
#endif

static gboolean
save_existing_local_file (GeditDocumentSaver *saver)
{
	mode_t saved_umask;
	struct stat statbuf;
	struct stat new_statbuf;
	gchar *backup_filename = NULL;
	gboolean backup_created = FALSE;

	gedit_debug (DEBUG_SAVER);

	if (fstat (saver->priv->fd, &statbuf) != 0) 
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	/* not a regular file */
	if (!S_ISREG (statbuf.st_mode))
	{
		if (S_ISDIR (statbuf.st_mode))
		{
			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GNOME_VFS_ERROR_IS_DIRECTORY,
				     gnome_vfs_result_to_string (GNOME_VFS_ERROR_IS_DIRECTORY));
		}
		else
		{
			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE,
				     "Not a regular file");
		}

		goto out;
	}

	/* check if the file is actually writable */
	if ((statbuf.st_mode & 0222) == 0) //FIXME... check better what else vim does
	{
		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_READ_ONLY,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_READ_ONLY));

		goto out;
	}

	/* check if someone else modified the file externally,
	 * except when "saving as", when saving a new doc (mtime = 0)
	 * or when the mtime check is explicitely disabled
	 */
	if (saver->priv->doc_mtime > 0 &&
	    statbuf.st_mtime != saver->priv->doc_mtime &&
	    ((saver->priv->flags & GEDIT_DOCUMENT_SAVE_IGNORE_MTIME) == 0))
	{
		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED,
			     "Externally modified");

		goto out;
	}

	/* prepare the backup name */
	backup_filename = get_backup_filename (saver);
	if (backup_filename == NULL)
	{
		/* bad bad luck... */
		g_warning (_("Could not obtain backup filename"));

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_GENERIC,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_GENERIC));

		goto out;
	}

	/* We now use two backup strategies.
	 * The first one (which is faster) consist in saving to a
	 * tmp file then rename the original file to the backup and the
	 * tmp file to the original name. This is fast but doesn't work
	 * when the file is a link (hard or symbolic) or when we can't
	 * write to the current dir or can't set the permissions on the
	 * new file. We also do not use it when the backup is not in the
	 * current dir, since if it isn't on the same FS rename wont work.
	 * The second strategy consist simply in copying the old file
	 * to a backup file and rewrite the contents of the file.
	 */

	if (saver->priv->backups_in_curr_dir &&
	    !(statbuf.st_nlink > 1) &&
	    !g_file_test (saver->priv->local_path, G_FILE_TEST_IS_SYMLINK))
	{
		gchar *dirname;
		gchar *tmp_filename;
		gint tmpfd;

		gedit_debug_message (DEBUG_SAVER, "tmp file moving strategy");

		dirname = g_path_get_dirname (saver->priv->local_path);
		tmp_filename = g_build_filename (dirname, ".gedit-save-XXXXXX", NULL);
		g_free (dirname);

		/* We set the umask because some (buggy) implementations
		 * of mkstemp() use permissions 0666 and we want 0600.
		 */
		saved_umask = umask (0077);
		tmpfd = g_mkstemp (tmp_filename);
		umask (saved_umask);

		if (tmpfd == -1)
		{
			gedit_debug_message (DEBUG_SAVER, "could not create tmp file");

			g_free (tmp_filename);
			goto fallback_strategy;
		}

		/* try to set permissions */
		if (fchown (tmpfd, statbuf.st_uid, statbuf.st_gid) == -1 ||
		    fchmod (tmpfd, statbuf.st_mode) == -1)
		{
			gedit_debug_message (DEBUG_SAVER, "could not set perms");

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto fallback_strategy;
		}

		/* copy the xattrs, like user.mime_type, over. Also ACLs and
		 * SELinux context. */
		if ((attr_copy_fd (saver->priv->local_path,
				   saver->priv->fd,
				   tmp_filename,
				   tmpfd,
				   all_xattrs,
				   NULL) == -1) &&
		    (errno != EOPNOTSUPP) && (errno != ENOSYS))
		{
			gedit_debug_message (DEBUG_SAVER, "could not set xattrs");

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto fallback_strategy;
		}

		if (!write_document_contents (tmpfd,
					      GTK_TEXT_BUFFER (saver->priv->document),
			 	 	      saver->priv->encoding,
				 	      &saver->priv->error))
		{
			gedit_debug_message (DEBUG_SAVER, "could not write tmp file");

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto out;
		}

		/* original -> backup */
		if (rename (saver->priv->local_path, backup_filename) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			gedit_debug_message (DEBUG_SAVER, "could not rename original -> backup");

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto out;
		}

		/* tmp -> original */
		if (rename (tmp_filename, saver->priv->local_path) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			gedit_debug_message (DEBUG_SAVER, "could not rename tmp -> original");

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			/* try to restore... no error checking */
			rename (backup_filename, saver->priv->local_path);

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto out;
		}

		g_free (tmp_filename);

		/* restat and get the mime type */
		if (fstat (tmpfd, &new_statbuf) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			close (tmpfd);
			goto out;
		}

		saver->priv->doc_mtime = new_statbuf.st_mtime;

		saver->priv->mime_type = get_slow_mime_type (saver->priv->uri);

		if (!saver->priv->keep_backup)
			unlink (backup_filename);

		close (tmpfd);

		goto out;
	}

 fallback_strategy:

	gedit_debug_message (DEBUG_SAVER, "fallback strategy");

	/* try to copy the old contents in a backup for safety
	 * unless we are explicetely told not to.
	 */
	if ((saver->priv->flags & GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP) == 0)
	{
		gint bfd;

		gedit_debug_message (DEBUG_SAVER, "copying to backup");

		/* move away old backups */
		if (!remove_file (backup_filename))
		{
			gedit_debug_message (DEBUG_SAVER, "could not remove old backup");

			/* we don't care about which was the problem, just
			 * that a backup was not possible.
			 */
			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			goto out;
		}

		bfd = open (backup_filename,
			    O_WRONLY | O_CREAT | O_EXCL,
			    statbuf.st_mode & 0777);

		if (bfd == -1)
		{
			gedit_debug_message (DEBUG_SAVER, "could not create backup");

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			goto out;
		}

		/* Try to set the group of the backup same as the
		 * original file. If this fails, set the protection
		 * bits for the group same as the protection bits for
		 * others. */
		if (fchown (bfd, (uid_t) -1, statbuf.st_gid) != 0)
		{
			gedit_debug_message (DEBUG_SAVER, "could not restore group");

			if (fchmod (bfd,
			            (statbuf.st_mode& 0707) |
			            ((statbuf.st_mode & 07) << 3)) != 0)
			{
				gedit_debug_message (DEBUG_SAVER, "could not even clear group perms");

				g_set_error (&saver->priv->error,
					     GEDIT_DOCUMENT_ERROR,
					     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
					     "No backup created");

				unlink (backup_filename);
				close (bfd);

				goto out;
			}
		}

		/* copy the xattrs, like user.mime_type, over. Also ACLs and
		 * SELinux context. */
		if ((attr_copy_fd (saver->priv->local_path,
				   saver->priv->fd,
				   backup_filename,
				   bfd,
				   all_xattrs,
				   NULL) == -1) &&
		    (errno != EOPNOTSUPP) && (errno != ENOSYS))
		{
			gedit_debug_message (DEBUG_SAVER, "could not set xattrs");

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			unlink (backup_filename);
			close (bfd);

			goto out;
		}

		if (!copy_file_data (saver->priv->fd, bfd, NULL))
		{
				gedit_debug_message (DEBUG_SAVER, "could not copy data into the backup");

				g_set_error (&saver->priv->error,
					     GEDIT_DOCUMENT_ERROR,
					     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
					     "No backup created");

				unlink (backup_filename);
				close (bfd);

				goto out;
		}

		backup_created = TRUE;
		close (bfd);
	}

	/* finally overwrite the original */
	if (!write_document_contents (saver->priv->fd,
				      GTK_TEXT_BUFFER (saver->priv->document),
			 	      saver->priv->encoding,
				      &saver->priv->error))
	{
		goto out;
	}

	/* remove the backup if we don't want to keep it */
	if (backup_created && !saver->priv->keep_backup)
	{
		unlink (backup_filename);
	}

	/* re stat the file and refetch the mime type */
	if (fstat (saver->priv->fd, &new_statbuf) != 0)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	saver->priv->doc_mtime = new_statbuf.st_mtime;

	g_free (saver->priv->mime_type);
	saver->priv->mime_type = get_slow_mime_type (saver->priv->uri);

 out:
	if (close (saver->priv->fd))
		g_warning ("File '%s' has not been correctly closed: %s",
			   saver->priv->uri,
			   strerror (errno));
	saver->priv->fd = -1;

	g_free (backup_filename);

	save_completed_or_failed (saver);

	/* stop the timeout */
	return FALSE;
}

static gboolean
save_new_local_file (GeditDocumentSaver *saver)
{
	struct stat statbuf;

	gedit_debug (DEBUG_SAVER);

	if (!write_document_contents (saver->priv->fd,
				      GTK_TEXT_BUFFER (saver->priv->document),
			 	      saver->priv->encoding,
				      &saver->priv->error))
	{
		goto out;
	}

	/* stat the file and fetch the mime type */
	if (fstat (saver->priv->fd, &statbuf) != 0)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	saver->priv->doc_mtime = statbuf.st_mtime;

	g_free (saver->priv->mime_type);
	saver->priv->mime_type = get_slow_mime_type (saver->priv->uri);

 out:
	if (close (saver->priv->fd))
		g_warning ("File '%s' has not been correctly closed: %s",
			   saver->priv->uri,
			   strerror (errno));

	saver->priv->fd = -1;

	save_completed_or_failed (saver);

	/* stop the timeout */
	return FALSE;
}

static gboolean
open_local_failed (GeditDocumentSaver *saver)
{
	save_completed_or_failed (saver);

	/* stop the timeout */
	return FALSE;
}

static void
save_local_file (GeditDocumentSaver *saver)
{
	GSourceFunc next_phase;
	GnomeVFSResult result;

	gedit_debug (DEBUG_SAVER);

	/* saving start */
	g_signal_emit (saver,
		       signals[SAVING],
		       0,
		       FALSE,
		       NULL);

	/* the file doesn't exist, create it */
	saver->priv->fd = open (saver->priv->local_path,
			        O_CREAT | O_EXCL | O_WRONLY,
			        0666);
	if (saver->priv->fd != -1)
	{
		next_phase = (GSourceFunc) save_new_local_file;
		goto out;
	}

	/* the file already exist */
	else if (errno == EEXIST)
	{
		saver->priv->fd = open (saver->priv->local_path, O_RDWR);
		if (saver->priv->fd != -1)
		{
			next_phase = (GSourceFunc) save_existing_local_file;
			goto out;
		}
	}

	/* else error */
	result = gnome_vfs_result_from_errno (); //may it happen that no errno?

	g_set_error (&saver->priv->error,
		     GEDIT_DOCUMENT_ERROR,
		     result,
		     gnome_vfs_result_to_string (result));

	next_phase = (GSourceFunc) open_local_failed;

 out:
	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    next_phase,
			    saver,
			    NULL);
}

/* ----------- remote files ----------- */

static void
remote_save_completed_or_failed (GeditDocumentSaver *saver)
{
	/* we can now close and unlink the tmp file */
	close (saver->priv->tmpfd);
	unlink (saver->priv->tmp_fname);

	save_completed_or_failed (saver);
}

static void
remote_get_info_cb (GnomeVFSAsyncHandle *handle,
		    GList               *results,
		    gpointer             data)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (data);
	GnomeVFSGetFileInfoResult *info_result;

	gedit_debug (DEBUG_SAVER);

	/* assert that the list has one and only one item */
	g_return_if_fail (results != NULL && results->next == NULL);

	info_result = (GnomeVFSGetFileInfoResult *) results->data;
	g_return_if_fail (info_result != NULL);

	if (info_result->result != GNOME_VFS_OK)
	{
		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     info_result->result,
			     gnome_vfs_result_to_string (info_result->result));

		remote_save_completed_or_failed (saver);

		return;
	}

	if (info_result->file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME)
		saver->priv->doc_mtime = info_result->file_info->mtime;

	if (info_result->file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE)
	{
		g_free (saver->priv->mime_type);
		saver->priv->mime_type = g_strdup (info_result->file_info->mime_type);
	}

	remote_save_completed_or_failed (saver);
}

static gint
async_xfer_ok (GnomeVFSXferProgressInfo *progress_info,
	       GeditDocumentSaver       *saver)
{
	gedit_debug_message (DEBUG_SAVER, "xfer phase: %d", progress_info->phase);

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
			res = gnome_vfs_get_file_info_uri (saver->priv->vfs_uri,
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
				g_set_error (&saver->priv->error,
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
				if (saver->priv->doc_mtime > 0 &&
				    orig_info->mtime != saver->priv->doc_mtime &&
				    ((saver->priv->flags & GEDIT_DOCUMENT_SAVE_IGNORE_MTIME) == 0))
				{
					g_set_error (&saver->priv->error,
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
				saver->priv->orig_info = orig_info;
		}
		break;
	case GNOME_VFS_XFER_PHASE_COLLECTING:
	case GNOME_VFS_XFER_PHASE_DELETESOURCE: // why do we get this phase??
		break;
	case GNOME_VFS_XFER_PHASE_READYTOGO:
		saver->priv->size = progress_info->bytes_total;
		break;
	case GNOME_VFS_XFER_PHASE_OPENSOURCE:
	case GNOME_VFS_XFER_PHASE_OPENTARGET:
	case GNOME_VFS_XFER_PHASE_COPYING:
	case GNOME_VFS_XFER_PHASE_WRITETARGET:
	case GNOME_VFS_XFER_PHASE_CLOSETARGET:
		if (progress_info->bytes_copied > 0)
			saver->priv->bytes_written = MIN (progress_info->total_bytes_copied,
							  progress_info->bytes_total);
		break;
	case GNOME_VFS_XFER_PHASE_FILECOMPLETED:
	case GNOME_VFS_XFER_PHASE_CLEANUP:
		break;
	case GNOME_VFS_XFER_PHASE_COMPLETED:
		/* Transfer done!
		 * Restore the permissions if needed and then refetch
		 * info on our newly written file to get the mime etc */
		{
			GList *uri_list = NULL;

			/* Try is not as paranoid as the local version (GID)... it would take
			 * yet another stat to do it...
			 */
			if (saver->priv->orig_info != NULL &&
			    (saver->priv->orig_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS))
			{
				gnome_vfs_set_file_info_uri (saver->priv->vfs_uri,
			     				     saver->priv->orig_info,
			     				     GNOME_VFS_SET_FILE_INFO_PERMISSIONS);

				// FIXME: for now is a blind try... do we want to error check?
			}

			uri_list = g_list_prepend (uri_list, saver->priv->vfs_uri);

			gnome_vfs_async_get_file_info (&saver->priv->info_handle,
						       uri_list,
						       GNOME_VFS_FILE_INFO_DEFAULT |
						       GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
						       GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
						       GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
						       GNOME_VFS_PRIORITY_MAX,
						       remote_get_info_cb,
						       saver);
			g_list_free (uri_list);
		}
		break;
	/* Phases we don't expect to see */
	case GNOME_VFS_XFER_PHASE_SETATTRIBUTES:
	case GNOME_VFS_XFER_PHASE_CLOSESOURCE:
	case GNOME_VFS_XFER_PHASE_MOVING:
	case GNOME_VFS_XFER_PHASE_READSOURCE:
	default:
		g_return_val_if_reached (0);
	}

	/* signal the progress */
	g_signal_emit (saver,
		       signals[SAVING],
		       0,
		       FALSE,
		       NULL);

	return 1;
}

static gint
async_xfer_error (GnomeVFSXferProgressInfo *progress_info,
		  GeditDocumentSaver       *saver)
{
	gedit_debug (DEBUG_SAVER);

	g_set_error (&saver->priv->error,
		     GEDIT_DOCUMENT_ERROR,
		     progress_info->vfs_status,
		     gnome_vfs_result_to_string (progress_info->vfs_status));

	remote_save_completed_or_failed (saver);

	return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
}

static gint
async_xfer_progress (GnomeVFSAsyncHandle      *handle,
		     GnomeVFSXferProgressInfo *progress_info,
		     gpointer                  data)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (data);

	switch (progress_info->status)
	{
	case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
		return async_xfer_ok (progress_info, saver);
	case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		return async_xfer_error (progress_info, saver);

	/* we should never go in these */
	case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
	case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
	default:
		g_return_val_if_reached (0);
	}
}

static gboolean
save_remote_file_real (GeditDocumentSaver *saver)
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
	saver->priv->tmpfd = g_file_open_tmp (".gedit-save-XXXXXX",
					      &saver->priv->tmp_fname,
					      &saver->priv->error);
	umask (saved_umask);

	if (saver->priv->tmpfd == -1)
	{
		result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		/* in this case no need to close the tmp file */
		save_completed_or_failed (saver);

		return FALSE;
	}

	tmp_uri = g_filename_to_uri (saver->priv->tmp_fname,
				     NULL,
				     &saver->priv->error);
	if (tmp_uri == NULL)
	{
		goto error;
	}

	tmp_vfs_uri = gnome_vfs_uri_new (tmp_uri);
	//needs error checking?

	g_free (tmp_uri);

	source_uri_list = g_list_prepend (source_uri_list, tmp_vfs_uri);
	dest_uri_list = g_list_prepend (dest_uri_list, saver->priv->vfs_uri);

	if (!write_document_contents (saver->priv->tmpfd,
				      GTK_TEXT_BUFFER (saver->priv->document),
		 	 	      saver->priv->encoding,
			 	      &saver->priv->error))
	{
		goto error;
	}

	gedit_debug_message (DEBUG_SAVER, "Saved local copy, starting xfer");

	result = gnome_vfs_async_xfer (&saver->priv->handle,
				       source_uri_list,
				       dest_uri_list,
				       GNOME_VFS_XFER_DEFAULT | GNOME_VFS_XFER_TARGET_DEFAULT_PERMS, // CHECK needs more thinking, follow symlinks etc... options are undocumented :(
				       GNOME_VFS_XFER_ERROR_MODE_ABORT,       /* keep it simple, abort on any error */
				       GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE, /* We have already asked confirm (even if it is racy) */
				       GNOME_VFS_PRIORITY_DEFAULT,
				       async_xfer_progress, saver,
				       NULL, NULL);

	gnome_vfs_uri_unref (tmp_vfs_uri);
	g_list_free (source_uri_list);
	g_list_free (dest_uri_list);

	if (result != GNOME_VFS_OK)
	{
		g_set_error (&saver->priv->error,
		    	     GEDIT_DOCUMENT_ERROR,
		     	     result,
			     gnome_vfs_result_to_string (result));

		goto error;
	}

	/* No errors: stop the timeout */
	return FALSE;

 error:
	remote_save_completed_or_failed (saver);

	/* stop the timeout */
	return FALSE;
}

static void
save_remote_file (GeditDocumentSaver *saver)
{
	gedit_debug (DEBUG_SAVER);

	/* saving start */
	g_signal_emit (saver,
		       signals[SAVING],
		       0,
		       FALSE,
		       NULL);

	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    (GSourceFunc) save_remote_file_real,
			    saver,
			    NULL);
}

/* ---------- public api ---------- */

void
gedit_document_saver_save (GeditDocumentSaver     *saver,
			   const gchar            *uri,
			   const GeditEncoding    *encoding,
			   time_t                  oldmtime,
			   GeditDocumentSaveFlags  flags)
{
	gchar *local_path;

	g_return_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver));
	g_return_if_fail ((uri != NULL) && (strlen (uri) > 0));

	gedit_debug (DEBUG_SAVER);

	// CHECK:
	// - sanity check a max len for the uri?
	// report async (in an idle handler) or sync (bool ret)
	// async is extra work here, sync is special casing in the caller

	saver->priv->uri = g_strdup (uri);

	/* fetch saving options */
	saver->priv->backup_ext = gedit_prefs_manager_get_backup_extension ();

	/* never keep backup of autosaves */
	if ((flags & GEDIT_DOCUMENT_SAVE_PRESERVE_BACKUP) != 0)
		saver->priv->keep_backup = FALSE;
	else
		saver->priv->keep_backup = gedit_prefs_manager_get_create_backup_copy ();

	/* TODO: add support for configurable backup dir */
	saver->priv->backups_in_curr_dir = TRUE;

	if (encoding != NULL)
		saver->priv->encoding = encoding;
	else
		saver->priv->encoding = gedit_encoding_get_utf8 ();

	saver->priv->doc_mtime = oldmtime;

	saver->priv->flags = flags;

	local_path = gnome_vfs_get_local_path_from_uri (uri);
	if (local_path != NULL)
	{
		saver->priv->local_path = local_path;
		save_local_file (saver);
	}
	else
	{
		saver->priv->vfs_uri = gnome_vfs_uri_new (uri);
		save_remote_file (saver);
	}
}

const gchar *
gedit_document_saver_get_uri (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), NULL);

	return saver->priv->uri;
}

const gchar *
gedit_document_saver_get_mime_type (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), NULL);

	return saver->priv->mime_type;
}

time_t
gedit_document_saver_get_mtime (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), 0);

	return saver->priv->doc_mtime;
}

/* Returns 0 if file size is unknown */
GnomeVFSFileSize
gedit_document_saver_get_file_size (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), 0);

	return saver->priv->size;
}

GnomeVFSFileSize
gedit_document_saver_get_bytes_written (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), 0);

	return saver->priv->bytes_written;
}
