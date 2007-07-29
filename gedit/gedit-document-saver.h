/*
 * gedit-document-saver.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyrhing (C) 2007 - Paolo Maggi, Steve Frécinaux
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

#ifndef __GEDIT_DOCUMENT_SAVER_H__
#define __GEDIT_DOCUMENT_SAVER_H__

#include <gedit/gedit-document.h>
#include <libgnomevfs/gnome-vfs-file-size.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_DOCUMENT_SAVER              (gedit_document_saver_get_type())
#define GEDIT_DOCUMENT_SAVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_DOCUMENT_SAVER, GeditDocumentSaver))
#define GEDIT_DOCUMENT_SAVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_DOCUMENT_SAVER, GeditDocumentSaverClass))
#define GEDIT_IS_DOCUMENT_SAVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_DOCUMENT_SAVER))
#define GEDIT_IS_DOCUMENT_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_DOCUMENT_SAVER))
#define GEDIT_DOCUMENT_SAVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_DOCUMENT_SAVER, GeditDocumentSaverClass))

/*
 * Main object structure
 */
typedef struct _GeditDocumentSaver GeditDocumentSaver;

struct _GeditDocumentSaver 
{
	GObject object;

	/*< private >*/
	GeditDocument		 *document;
	gboolean		  used;

	gchar			 *uri;
	const GeditEncoding      *encoding;

	GeditDocumentSaveFlags    flags;

	gboolean		  keep_backup;
	gchar			 *backup_ext;
	gboolean                  backups_in_curr_dir;
};

/*
 * Class definition
 */
typedef struct _GeditDocumentSaverClass GeditDocumentSaverClass;

struct _GeditDocumentSaverClass 
{
	GObjectClass parent_class;

	/* Signals */
	void (* saving) (GeditDocumentSaver *saver,
			 gboolean             completed,
			 const GError        *error);

	/* VTable */
	void			(* save)		(GeditDocumentSaver *saver,
							 time_t              old_mtime);
	const gchar *		(* get_mime_type)	(GeditDocumentSaver *saver);
	time_t			(* get_mtime)		(GeditDocumentSaver *saver);
	GnomeVFSFileSize	(* get_file_size)	(GeditDocumentSaver *saver);
	GnomeVFSFileSize	(* get_bytes_written)	(GeditDocumentSaver *saver);
};

/*
 * Public methods
 */
GType 		 	 gedit_document_saver_get_type		(void) G_GNUC_CONST;

/* If enconding == NULL, the encoding will be autodetected */
GeditDocumentSaver 	*gedit_document_saver_new 		(GeditDocument        *doc,
								 const gchar          *uri,
								 const GeditEncoding  *encoding,
								 GeditDocumentSaveFlags flags);

gboolean		 gedit_document_saver_write_document_contents (
								 GeditDocumentSaver  *saver,
								 gint                 fd,
								 GError             **error);

void			 gedit_document_saver_saving		(GeditDocumentSaver *saver,
								 gboolean            completed,
								 GError             *error);
void			 gedit_document_saver_save		(GeditDocumentSaver  *saver,
								 time_t               old_mtime);

#if 0
void			 gedit_document_saver_cancel		(GeditDocumentSaver  *saver);
#endif

const gchar		*gedit_document_saver_get_uri		(GeditDocumentSaver  *saver);

/* If backup_uri is NULL no backup will be made */
const gchar		*gedit_document_saver_get_backup_uri	(GeditDocumentSaver  *saver);
void			*gedit_document_saver_set_backup_uri	(GeditDocumentSaver  *saver,
							 	 const gchar         *backup_uri);

const gchar		*gedit_document_saver_get_mime_type	(GeditDocumentSaver  *saver);

time_t			 gedit_document_saver_get_mtime		(GeditDocumentSaver  *saver);

/* Returns 0 if file size is unknown */
GnomeVFSFileSize	 gedit_document_saver_get_file_size	(GeditDocumentSaver  *saver);

GnomeVFSFileSize	 gedit_document_saver_get_bytes_written	(GeditDocumentSaver  *saver);


G_END_DECLS

#endif  /* __GEDIT_DOCUMENT_SAVER_H__  */
