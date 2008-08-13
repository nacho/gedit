/*
 * gedit-mmap-document-loader.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "gedit-mmap-document-loader.h"
#include "gedit-debug.h"
#include "gedit-metadata-manager.h"
#include "gedit-utils.h"

#include "gedit-marshal.h"

#define READ_CHUNK_SIZE 8192

#define GEDIT_MMAP_DOCUMENT_LOADER_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
				 GEDIT_TYPE_MMAP_DOCUMENT_LOADER,   \
				 GeditMmapDocumentLoaderPrivate))

static void		 gedit_mmap_document_loader_load		(GeditDocumentLoader *loader);
static gboolean		 gedit_mmap_document_loader_cancel		(GeditDocumentLoader *loader);
static const gchar	*gedit_mmap_document_loader_get_content_type	(GeditDocumentLoader *loader);
static time_t		 gedit_mmap_document_loader_get_mtime		(GeditDocumentLoader *loader);
static goffset		 gedit_mmap_document_loader_get_file_size	(GeditDocumentLoader *loader);
static goffset		 gedit_mmap_document_loader_get_bytes_read	(GeditDocumentLoader *loader);
static gboolean		 gedit_mmap_document_loader_get_readonly	(GeditDocumentLoader *loader);

struct _GeditMmapDocumentLoaderPrivate
{
	struct stat statbuf;
	gchar      *content_type;
	guint       statbuf_filled : 1;

	goffset     bytes_read;

	gint        fd;
	gchar      *local_file_name;

	gchar      *buffer;

	GError     *error;
};

G_DEFINE_TYPE(GeditMmapDocumentLoader, gedit_mmap_document_loader, GEDIT_TYPE_DOCUMENT_LOADER)

static void
gedit_mmap_document_loader_finalize (GObject *object)
{
	GeditMmapDocumentLoaderPrivate *priv;

	priv = GEDIT_MMAP_DOCUMENT_LOADER (object)->priv;

	g_free (priv->content_type);
	g_free (priv->local_file_name);
	g_free (priv->buffer);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_mmap_document_loader_parent_class)->finalize (object);
}

static void
gedit_mmap_document_loader_class_init (GeditMmapDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditDocumentLoaderClass *loader_class = GEDIT_DOCUMENT_LOADER_CLASS (klass);

	object_class->finalize = gedit_mmap_document_loader_finalize;

	loader_class->load = gedit_mmap_document_loader_load;
	loader_class->cancel = gedit_mmap_document_loader_cancel;
	loader_class->get_content_type = gedit_mmap_document_loader_get_content_type;
	loader_class->get_mtime = gedit_mmap_document_loader_get_mtime;
	loader_class->get_file_size = gedit_mmap_document_loader_get_file_size;
	loader_class->get_bytes_read = gedit_mmap_document_loader_get_bytes_read;
	loader_class->get_readonly = gedit_mmap_document_loader_get_readonly;

	g_type_class_add_private (object_class, sizeof(GeditMmapDocumentLoaderPrivate));
}

static void
gedit_mmap_document_loader_init (GeditMmapDocumentLoader *mloader)
{
	mloader->priv = GEDIT_MMAP_DOCUMENT_LOADER_GET_PRIVATE (mloader);
	mloader->priv->statbuf_filled = FALSE;
	mloader->priv->content_type = NULL;
	mloader->priv->fd = -1;
	mloader->priv->error = NULL;
}

#define MAX_MIME_SNIFF_SIZE 4096

static sigjmp_buf mmap_env;
static struct sigaction old_sigbusact;

/* When we access the mmapped data we need to handle
 * SIGBUS signal, which is emitted if there was an
 * I/O error or if the file was truncated */
static void
mmap_sigbus_handler (int signo)
{
	siglongjmp (mmap_env, 0);
}

static gboolean
load_file_real (GeditMmapDocumentLoader *mloader)
{
	gint ret;

	g_return_val_if_fail (mloader->priv->fd != -1, FALSE);

	if (fstat (mloader->priv->fd, &mloader->priv->statbuf) != 0) 
	{
		g_set_error (&mloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     g_io_error_from_errno (errno),
			     g_strerror (errno));

		goto done;
	}

	/* not a regular file */
	if (!S_ISREG (mloader->priv->statbuf.st_mode))
	{
		if (S_ISDIR (mloader->priv->statbuf.st_mode))
		{
			g_set_error (&mloader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     G_IO_ERROR_IS_DIRECTORY,
				     "Is a directory");
		}
		else
		{
			g_set_error (&mloader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     G_IO_ERROR_NOT_REGULAR_FILE,
				     "Not a regular file");
		}

		goto done;
	}

	mloader->priv->statbuf_filled = TRUE;

	if (mloader->priv->statbuf.st_size == 0)
	{
		gedit_document_loader_update_document_contents (
						GEDIT_DOCUMENT_LOADER (mloader),
						"",
						0,
						&mloader->priv->error);

		/* guessing the mime from the filename is up to the caller */
	}
	else
	{
		gchar *mapped_file;
		const gchar *content_type;
		struct sigaction sigbusact;

		/* CHECK: should we lock the file */		
		mapped_file = mmap (0, /* start */
				    mloader->priv->statbuf.st_size, 
				    PROT_READ,
				    MAP_PRIVATE, /* flags */
				    mloader->priv->fd,
				    0 /* offset */);

		if (mapped_file == MAP_FAILED)
		{
			gedit_debug_message (DEBUG_LOADER, "mmap failed");

			g_set_error (&mloader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     g_io_error_from_errno (errno),
				     g_strerror (errno));

			goto done;
		}

		/* prepare to handle the SIGBUS signal which
		 * may be fired when accessing the mmapped
		 * data in case of I/O error.
		 * See bug #354046.
		 */
		sigbusact.sa_handler = mmap_sigbus_handler;
		sigemptyset (&sigbusact.sa_mask);
		sigbusact.sa_flags = SA_RESETHAND;
		sigaction (SIGBUS, &sigbusact, &old_sigbusact );

		if (sigsetjmp (mmap_env, 1) != 0)
		{
			gedit_debug_message (DEBUG_LOADER, "SIGBUS during mmap");

			g_set_error (&mloader->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     G_IO_ERROR_FAILED,
				     "I/O error");

			ret = munmap (mapped_file, mloader->priv->statbuf.st_size);
			if (ret != 0)
				g_warning ("File '%s' has not been correctly unmapped: %s",
					   GEDIT_DOCUMENT_LOADER (mloader)->uri,
					   strerror (errno));

			goto done;
		}

		mloader->priv->bytes_read = mloader->priv->statbuf.st_size;

		if (!gedit_document_loader_update_document_contents (
						GEDIT_DOCUMENT_LOADER (mloader),
						mapped_file,
						mloader->priv->statbuf.st_size,
						&mloader->priv->error))
		{
			ret = munmap (mapped_file, mloader->priv->statbuf.st_size);
			if (ret != 0)
				g_warning ("File '%s' has not been correctly unmapped: %s",
					   GEDIT_DOCUMENT_LOADER (mloader)->uri,
					   strerror (errno));

			goto done;
		}

		/* restore the default sigbus handler */
		sigaction (SIGBUS, &old_sigbusact, 0);

		content_type = g_content_type_guess (mloader->priv->local_file_name,
						    (const guchar *)mapped_file,
						     MIN (mloader->priv->bytes_read,
						         MAX_MIME_SNIFF_SIZE),
						     NULL);

		if ((content_type != NULL) && !g_content_type_is_unknown (content_type))
		{
			mloader->priv->content_type = content_type;
		}

		ret = munmap (mapped_file, mloader->priv->statbuf.st_size);

		if (ret != 0)
			g_warning ("File '%s' has not been correctly unmapped: %s",
				   GEDIT_DOCUMENT_LOADER (mloader)->uri,
				   strerror (errno));
	}

 done:
	ret = close (mloader->priv->fd);

	if (ret != 0)
		g_warning ("File '%s' has not been correctly closed: %s",
			   GEDIT_DOCUMENT_LOADER (mloader)->uri,
			   strerror (errno));

	mloader->priv->fd = -1;

	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (mloader),
				       TRUE,
				       mloader->priv->error);

	return FALSE;
}

static gboolean
open_failed (GeditMmapDocumentLoader *mloader)
{
	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (mloader),
				       TRUE,
				       mloader->priv->error);

	/* stop the timeout */
	return FALSE;
}

static void
load_file (GeditMmapDocumentLoader *mloader,
	   const gchar             *fname)
{
	gedit_debug (DEBUG_LOADER);

	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (mloader),
				       FALSE,
				       NULL);

	mloader->priv->fd = open (fname, O_RDONLY);
	if (mloader->priv->fd == -1)
	{
		g_set_error (&mloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     g_io_error_from_errno (errno),
			     g_strerror (errno));

		g_timeout_add_full (G_PRIORITY_HIGH,
				    0,
				    (GSourceFunc) open_failed,
				    mloader,
				    NULL);

		return;
	}

	g_free (mloader->priv->local_file_name);
	mloader->priv->local_file_name = g_strdup (fname);

	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    (GSourceFunc) load_file_real,
			    mloader,
			    NULL);
}

static void
gedit_mmap_document_loader_load (GeditDocumentLoader *loader)
{
	GeditMmapDocumentLoader *mloader = GEDIT_MMAP_DOCUMENT_LOADER (loader);
	gchar *local_path;
	GFile *gfile;
	
	gfile = g_file_new_for_uri (loader->uri);
	local_path = g_file_get_path (gfile);
	g_object_unref (gfile);

	if (local_path != NULL)
	{
		load_file (mloader, local_path);
		g_free (local_path);
	}
	else
	{
		g_set_error (&mloader->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     "Not supported");
	}
}

static const gchar *
gedit_mmap_document_loader_get_content_type (GeditDocumentLoader *loader)
{
	return GEDIT_MMAP_DOCUMENT_LOADER (loader)->priv->content_type;
}

static time_t
gedit_mmap_document_loader_get_mtime (GeditDocumentLoader *loader)
{
	GeditMmapDocumentLoader *mloader = GEDIT_MMAP_DOCUMENT_LOADER (loader);

	if (!mloader->priv->statbuf_filled)
		return FALSE;
	return mloader->priv->statbuf.st_mtime;
}

static goffset
gedit_mmap_document_loader_get_file_size (GeditDocumentLoader *loader)
{
	GeditMmapDocumentLoader *mloader = GEDIT_MMAP_DOCUMENT_LOADER (loader);

	if (!mloader->priv->statbuf_filled)
		return 0;
	return mloader->priv->statbuf.st_size;
}

static goffset
gedit_mmap_document_loader_get_bytes_read (GeditDocumentLoader *loader)
{
	return GEDIT_MMAP_DOCUMENT_LOADER (loader)->priv->bytes_read;
}

static gboolean
gedit_mmap_document_loader_cancel (GeditDocumentLoader *loader)
{
	GeditMmapDocumentLoader *mloader = GEDIT_MMAP_DOCUMENT_LOADER (loader);

	g_set_error (&mloader->priv->error,
		     GEDIT_DOCUMENT_ERROR,
		     G_IO_ERROR_CANCELLED,
		     "Cancelled");

	gedit_document_loader_loading (GEDIT_DOCUMENT_LOADER (mloader),
				       TRUE,
				       mloader->priv->error);

	return TRUE;
}

static gboolean
gedit_mmap_document_loader_get_readonly (GeditDocumentLoader *loader)
{
	GeditMmapDocumentLoader *mloader = GEDIT_MMAP_DOCUMENT_LOADER (loader);

	if (!mloader->priv->statbuf_filled)
		return FALSE;
	return !(mloader->priv->statbuf.st_mode & S_IWUSR);
}
