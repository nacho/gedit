/*
 * gedit-local-document-saver.c
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
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gfileutils.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-local-document-saver.h"
#include "gedit-debug.h"

#ifdef HAVE_LIBATTR
#include <attr/libattr.h>
#else
#define attr_copy_fd(x1, x2, x3, x4, x5, x6) (errno = ENOSYS, -1)
#endif

#define BUFSIZE	8192 /* size of normal write buffer */

#define GEDIT_LOCAL_DOCUMENT_SAVER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						       GEDIT_TYPE_LOCAL_DOCUMENT_SAVER, \
						       GeditLocalDocumentSaverPrivate))

static void		 gedit_local_document_saver_save		(GeditDocumentSaver *saver,
									 time_t              old_mtime);
static gboolean		 gedit_local_document_saver_cancel		(GeditDocumentSaver *saver);
static const gchar 	*gedit_local_document_saver_get_mime_type	(GeditDocumentSaver *saver);
static time_t		 gedit_local_document_saver_get_mtime		(GeditDocumentSaver *saver);
static GnomeVFSFileSize	 gedit_local_document_saver_get_file_size	(GeditDocumentSaver *saver);
static GnomeVFSFileSize	 gedit_local_document_saver_get_bytes_written	(GeditDocumentSaver *saver);


struct _GeditLocalDocumentSaverPrivate
{
	GnomeVFSFileSize	  size;
	GnomeVFSFileSize	  bytes_written;

	/* temp data for local files */
	gint			  fd;
	gchar			 *local_path;
	gchar                    *mime_type; //CHECK use FileInfo instead?
	time_t                    doc_mtime;

	GError                   *error;
};

G_DEFINE_TYPE(GeditLocalDocumentSaver, gedit_local_document_saver, GEDIT_TYPE_DOCUMENT_SAVER)

static void
gedit_local_document_saver_finalize (GObject *object)
{
	GeditLocalDocumentSaverPrivate *priv = GEDIT_LOCAL_DOCUMENT_SAVER (object)->priv;

	g_free (priv->local_path);
	g_free (priv->mime_type);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_local_document_saver_parent_class)->finalize (object);
}

static void 
gedit_local_document_saver_class_init (GeditDocumentSaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditDocumentSaverClass *saver_class = GEDIT_DOCUMENT_SAVER_CLASS (klass);

	object_class->finalize = gedit_local_document_saver_finalize;

	saver_class->save = gedit_local_document_saver_save;
	saver_class->get_mime_type = gedit_local_document_saver_get_mime_type;
	saver_class->get_mtime = gedit_local_document_saver_get_mtime;
	saver_class->get_file_size = gedit_local_document_saver_get_file_size;
	saver_class->get_bytes_written = gedit_local_document_saver_get_bytes_written;

	saver_class->save = gedit_local_document_saver_save;

	g_type_class_add_private (object_class, sizeof(GeditLocalDocumentSaverPrivate));
}

static void
gedit_local_document_saver_init (GeditLocalDocumentSaver *lsaver)
{
	lsaver->priv = GEDIT_LOCAL_DOCUMENT_SAVER_GET_PRIVATE (lsaver);

	lsaver->priv->fd = -1;

	lsaver->priv->error = NULL;
}

static gchar *
get_backup_filename (GeditLocalDocumentSaver *lsaver)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (lsaver);
	gchar *fname;
	gchar *bak_ext = NULL;

	if (saver->backup_ext != NULL && strlen (saver->backup_ext) > 0)
		bak_ext = saver->backup_ext;
	else
		bak_ext = "~";

	fname = g_strconcat (lsaver->priv->local_path, bak_ext, NULL);

	/* If we are not going to keep the backup file and fname
	 * already exists, try to use another name.
	 * Change one character, just before the extension.
	 */
	if (!saver->keep_backup && g_file_test (fname, G_FILE_TEST_EXISTS))
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

	return res == 0 || (res == -1 && errno == ENOENT);
}

static gboolean
copy_file_data (gint     sfd,
		gint     dfd,
		GError **error)
{
	gboolean ret = TRUE;
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

			g_set_error (error,
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
				GnomeVFSResult result;

				if (errno == EINTR)
					continue;

				result = gnome_vfs_result_from_errno ();

				g_set_error (error,
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

	g_free (buffer);

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
	if (info->mime_type == NULL || result != GNOME_VFS_OK)
		mime_type = NULL;
	else
		mime_type = g_strdup (info->mime_type);

	gnome_vfs_file_info_unref (info);

	return mime_type;
}

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
save_existing_local_file (GeditLocalDocumentSaver *lsaver)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (lsaver);
	mode_t saved_umask;
	struct stat statbuf;
	struct stat new_statbuf;
	gchar *backup_filename = NULL;
	gboolean backup_created = FALSE;

	gedit_debug (DEBUG_SAVER);

	if (fstat (lsaver->priv->fd, &statbuf) != 0) 
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&lsaver->priv->error,
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
			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GNOME_VFS_ERROR_IS_DIRECTORY,
				     gnome_vfs_result_to_string (GNOME_VFS_ERROR_IS_DIRECTORY));
		}
		else
		{
			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE,
				     "Not a regular file");
		}

		goto out;
	}

	/* check if the file is actually writable */
	if ((statbuf.st_mode & 0222) == 0) //FIXME... check better what else vim does
	{
		g_set_error (&lsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_READ_ONLY,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_READ_ONLY));

		goto out;
	}

	/* check if someone else modified the file externally,
	 * except when "saving as", when saving a new doc (mtime = 0)
	 * or when the mtime check is explicitely disabled
	 */
	if (lsaver->priv->doc_mtime > 0 &&
	    statbuf.st_mtime != lsaver->priv->doc_mtime &&
	    ((saver->flags & GEDIT_DOCUMENT_SAVE_IGNORE_MTIME) == 0))
	{
		g_set_error (&lsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED,
			     "Externally modified");

		goto out;
	}

	/* prepare the backup name */
	backup_filename = get_backup_filename (lsaver);
	if (backup_filename == NULL)
	{
		/* bad bad luck... */
		g_warning (_("Could not obtain backup filename"));

		g_set_error (&lsaver->priv->error,
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

	if (saver->backups_in_curr_dir &&
	    !(statbuf.st_nlink > 1) &&
	    !g_file_test (lsaver->priv->local_path, G_FILE_TEST_IS_SYMLINK))
	{
		gchar *dirname;
		gchar *tmp_filename;
		gint tmpfd;

		gedit_debug_message (DEBUG_SAVER, "tmp file moving strategy");

		dirname = g_path_get_dirname (lsaver->priv->local_path);
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
			struct stat tmp_statbuf;
			gedit_debug_message (DEBUG_SAVER, "could not set perms");

			/* Check that we really needed to change something */
			if (fstat (lsaver->priv->fd, &tmp_statbuf) != 0 ||
			    statbuf.st_uid != tmp_statbuf.st_uid ||
			    statbuf.st_gid != tmp_statbuf.st_gid ||
			    statbuf.st_mode != tmp_statbuf.st_mode)
			{
				close (tmpfd);
				unlink (tmp_filename);
				g_free (tmp_filename);

				goto fallback_strategy;
			}
		}

		/* copy the xattrs, like user.mime_type, over. Also ACLs and
		 * SELinux context. */
		if ((attr_copy_fd (lsaver->priv->local_path,
				   lsaver->priv->fd,
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

		if (!gedit_document_saver_write_document_contents (
							saver,
							tmpfd,
							&lsaver->priv->error))
		{
			gedit_debug_message (DEBUG_SAVER, "could not write tmp file");

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto out;
		}

		/* original -> backup */
		if (rename (lsaver->priv->local_path, backup_filename) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			gedit_debug_message (DEBUG_SAVER, "could not rename original -> backup");

			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			close (tmpfd);
			unlink (tmp_filename);
			g_free (tmp_filename);

			goto out;
		}

		/* tmp -> original */
		if (rename (tmp_filename, lsaver->priv->local_path) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			gedit_debug_message (DEBUG_SAVER, "could not rename tmp -> original");

			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			/* try to restore... no error checking */
			rename (backup_filename, lsaver->priv->local_path);

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

			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			close (tmpfd);
			goto out;
		}

		lsaver->priv->doc_mtime = new_statbuf.st_mtime;

		lsaver->priv->mime_type = get_slow_mime_type (saver->uri);

		if (saver->keep_backup)
		{
			/* remove executable permissions from the backup 
			 * file */
			mode_t new_mode = statbuf.st_mode;

			new_mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);
			
			if (new_mode != statbuf.st_mode)
			{
				/* try to change permissions */
				chmod (backup_filename, new_mode);
			}
		}
		else
		{
			unlink (backup_filename);
		}

		close (tmpfd);

		goto out;
	}

 fallback_strategy:

	gedit_debug_message (DEBUG_SAVER, "fallback strategy");

	/* try to copy the old contents in a backup for safety
	 * unless we are explicetely told not to.
	 */
	if ((saver->flags & GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP) == 0)
	{
		gint bfd;
		struct stat tmp_statbuf;

		gedit_debug_message (DEBUG_SAVER, "copying to backup");

		/* move away old backups */
		if (!remove_file (backup_filename))
		{
			gedit_debug_message (DEBUG_SAVER, "could not remove old backup");

			/* we don't care about which was the problem, just
			 * that a backup was not possible.
			 */
			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			goto out;
		}

		/* open the backup file with the same permissions
		 * except the executable ones */
		bfd = open (backup_filename,
			    O_WRONLY | O_CREAT | O_EXCL,
			    statbuf.st_mode & 0666);

		if (bfd == -1)
		{
			gedit_debug_message (DEBUG_SAVER, "could not create backup");

			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			goto out;
		}

		if (fstat (lsaver->priv->fd, &tmp_statbuf) != 0)
		{
			gedit_debug_message (DEBUG_SAVER, "could not stat the backup file");

			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			unlink (backup_filename);
			close (bfd);
			goto out;
		}

		/* If needed, Try to set the group of the backup same as
		 * the original file. If this fails, set the protection
		 * bits for the group same as the protection bits for
		 * others. */
		if ((statbuf.st_gid != tmp_statbuf.st_gid) &&
		    fchown (bfd, (uid_t) -1, statbuf.st_gid) != 0)
		{
			gedit_debug_message (DEBUG_SAVER, "could not restore group");

			if (fchmod (bfd,
			            (statbuf.st_mode & 0606) |
			            ((statbuf.st_mode & 06) << 3)) != 0)
			{
				gedit_debug_message (DEBUG_SAVER, "could not even clear group perms");

				g_set_error (&lsaver->priv->error,
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
		if ((attr_copy_fd (lsaver->priv->local_path,
				   lsaver->priv->fd,
				   backup_filename,
				   bfd,
				   all_xattrs,
				   NULL) == -1) &&
		    (errno != EOPNOTSUPP) && (errno != ENOSYS))
		{
			gedit_debug_message (DEBUG_SAVER, "could not set xattrs");

			g_set_error (&lsaver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
				     "No backup created");

			unlink (backup_filename);
			close (bfd);

			goto out;
		}

		if (!copy_file_data (lsaver->priv->fd, bfd, NULL))
		{
				gedit_debug_message (DEBUG_SAVER, "could not copy data into the backup");

				g_set_error (&lsaver->priv->error,
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
	if (!gedit_document_saver_write_document_contents (saver,
							   lsaver->priv->fd,
							   &lsaver->priv->error))
	{
		/* FIXME: restore the backup? */
		goto out;
	}

	/* remove the backup if we don't want to keep it */
	if (backup_created && !saver->keep_backup)
	{
		unlink (backup_filename);
	}

	/* re stat the file and refetch the mime type */
	if (fstat (lsaver->priv->fd, &new_statbuf) != 0)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&lsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	lsaver->priv->doc_mtime = new_statbuf.st_mtime;

	g_free (lsaver->priv->mime_type);
	lsaver->priv->mime_type = get_slow_mime_type (saver->uri);

 out:
	if (close (lsaver->priv->fd))
		g_warning ("File '%s' has not been correctly closed: %s",
			   saver->uri,
			   strerror (errno));
	lsaver->priv->fd = -1;

	g_free (backup_filename);

	gedit_document_saver_saving (saver, TRUE, lsaver->priv->error);

	/* stop the timeout */
	return FALSE;
}

static gboolean
save_new_local_file (GeditLocalDocumentSaver *lsaver)
{
	struct stat statbuf;

	gedit_debug (DEBUG_SAVER);

	if (!gedit_document_saver_write_document_contents (
						GEDIT_DOCUMENT_SAVER (lsaver),
						lsaver->priv->fd,
						&lsaver->priv->error))
	{
		goto out;
	}

	/* stat the file and fetch the mime type */
	if (fstat (lsaver->priv->fd, &statbuf) != 0)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&lsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	lsaver->priv->doc_mtime = statbuf.st_mtime;

	g_free (lsaver->priv->mime_type);
	lsaver->priv->mime_type = get_slow_mime_type (GEDIT_DOCUMENT_SAVER (lsaver)->uri);

 out:
	if (close (lsaver->priv->fd))
		g_warning ("File '%s' has not been correctly closed: %s",
			   GEDIT_DOCUMENT_SAVER (lsaver)->uri,
			   strerror (errno));

	lsaver->priv->fd = -1;

	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (lsaver),
				     TRUE,
				     lsaver->priv->error);

	/* stop the timeout */
	return FALSE;
}

static gboolean
open_local_failed (GeditLocalDocumentSaver *lsaver)
{
	gedit_document_saver_saving (GEDIT_DOCUMENT_SAVER (lsaver),
				     TRUE,
				     lsaver->priv->error);

	/* stop the timeout */
	return FALSE;
}

static void
save_file (GeditLocalDocumentSaver *lsaver)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (lsaver);
	GSourceFunc next_phase;
	GnomeVFSResult result;

	gedit_debug (DEBUG_SAVER);

	/* saving start */
	gedit_document_saver_saving (saver, FALSE, NULL);

	/* the file doesn't exist, create it */
	lsaver->priv->fd = open (lsaver->priv->local_path,
			         O_CREAT | O_EXCL | O_WRONLY,
			         0666);
	if (lsaver->priv->fd != -1)
	{
		next_phase = (GSourceFunc) save_new_local_file;
		goto out;
	}

	/* the file already exist */
	else if (errno == EEXIST)
	{
		lsaver->priv->fd = open (lsaver->priv->local_path, O_RDWR);
		if (lsaver->priv->fd != -1)
		{
			next_phase = (GSourceFunc) save_existing_local_file;
			goto out;
		}
	}

	/* else error */
	result = gnome_vfs_result_from_errno (); //may it happen that no errno?

	g_set_error (&lsaver->priv->error,
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


static void
gedit_local_document_saver_save (GeditDocumentSaver *saver,
				 time_t              old_mtime)
{
	GeditLocalDocumentSaver *lsaver = GEDIT_LOCAL_DOCUMENT_SAVER (saver);

	lsaver->priv->doc_mtime = old_mtime;

	lsaver->priv->local_path = gnome_vfs_get_local_path_from_uri (saver->uri);
	if (lsaver->priv->local_path != NULL)
	{
		save_file (lsaver);
	}
	else
	{
		g_set_error (&lsaver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_NOT_SUPPORTED,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_NOT_SUPPORTED));
	}
}

static const gchar *
gedit_local_document_saver_get_mime_type (GeditDocumentSaver *saver)
{
	return GEDIT_LOCAL_DOCUMENT_SAVER (saver)->priv->mime_type;
}

static time_t
gedit_local_document_saver_get_mtime (GeditDocumentSaver *saver)
{
	return GEDIT_LOCAL_DOCUMENT_SAVER (saver)->priv->doc_mtime;
}

static GnomeVFSFileSize
gedit_local_document_saver_get_file_size (GeditDocumentSaver *saver)
{
	return GEDIT_LOCAL_DOCUMENT_SAVER (saver)->priv->size;
}

static GnomeVFSFileSize
gedit_local_document_saver_get_bytes_written (GeditDocumentSaver *saver)
{
	return GEDIT_LOCAL_DOCUMENT_SAVER (saver)->priv->bytes_written;
}
