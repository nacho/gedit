/*
 * gedit-document.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002-2005 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */
 
#ifndef __GEDIT_DOCUMENT_H__
#define __GEDIT_DOCUMENT_H__

#include <gtk/gtk.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gedit/gedit-encodings.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_DOCUMENT              (gedit_document_get_type())
#define GEDIT_DOCUMENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_DOCUMENT, GeditDocument))
#define GEDIT_DOCUMENT_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_DOCUMENT, GeditDocument const))
#define GEDIT_DOCUMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_DOCUMENT, GeditDocumentClass))
#define GEDIT_IS_DOCUMENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_DOCUMENT))
#define GEDIT_IS_DOCUMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_DOCUMENT))
#define GEDIT_DOCUMENT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_DOCUMENT, GeditDocumentClass))

typedef enum
{
	GEDIT_SEARCH_DONT_SET_FLAGS	= 1 << 0, 
	GEDIT_SEARCH_ENTIRE_WORD	= 1 << 1,
	GEDIT_SEARCH_CASE_SENSITIVE	= 1 << 2

} GeditSearchFlags;

/* Private structure type */
typedef struct _GeditDocumentPrivate    GeditDocumentPrivate;

/*
 * Main object structure
 */
typedef struct _GeditDocument           GeditDocument;
 
struct _GeditDocument
{
	GtkSourceBuffer buffer;
	
	/*< private > */
	GeditDocumentPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditDocumentClass 	GeditDocumentClass;

struct _GeditDocumentClass
{
	GtkSourceBufferClass parent_class;

	/* Signals */ // CHECK: ancora da rivedere

	void (* cursor_moved)		(GeditDocument    *document);

	/* Document load */
	void (* loading)		(GeditDocument    *document,
					 GnomeVFSFileSize  size,
					 GnomeVFSFileSize  total_size);

	void (* loaded)			(GeditDocument    *document,
					 const GError     *error);

	/* Document save */
	void (* saving)			(GeditDocument    *document,
					 GnomeVFSFileSize  size,
					 GnomeVFSFileSize  total_size);

	void (* saved)  		(GeditDocument    *document,
					 const GError     *error);
};


typedef enum
{
	/* save file despite external modifications */
	GEDIT_DOCUMENT_SAVE_IGNORE_MTIME 	= 1 << 0,

	/* write the file directly without attempting to backup */
	GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP	= 1 << 1
} GeditDocumentSaveFlags;


#define GEDIT_DOCUMENT_ERROR gedit_document_error_quark ()

enum
{
	/* start at GNOME_VFS_NUM_ERRORS since we use GnomeVFSResult 
	 * for the error codes */ 
	GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED = GNOME_VFS_NUM_ERRORS,
	GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE,
	GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
	GEDIT_DOCUMENT_NUM_ERRORS 
};

GQuark		 gedit_document_error_quark	(void);

GType		 gedit_document_get_type      	(void) G_GNUC_CONST;

GeditDocument   *gedit_document_new 		(void);

gchar		*gedit_document_get_uri 	(GeditDocument       *doc);

gchar		*gedit_document_get_uri_for_display
						(GeditDocument       *doc);
gchar		*gedit_document_get_short_name_for_display
					 	(GeditDocument       *doc);

gchar		*gedit_document_get_mime_type 	(GeditDocument       *doc);

gboolean	 gedit_document_get_readonly 	(GeditDocument       *doc);

gboolean	 gedit_document_load 		(GeditDocument       *doc, 
						 const gchar         *uri, 
						 const GeditEncoding *encoding,
						 gint                 line_pos,
						 gboolean             create); 
						 
gboolean	 gedit_document_insert_file	(GeditDocument       *doc,
						 GtkTextIter         *iter, 
						 const gchar         *uri, 
						 const GeditEncoding *encoding);

gboolean	 gedit_document_load_cancel	(GeditDocument       *doc);

void		 gedit_document_save 		(GeditDocument       *doc,
						 GeditDocumentSaveFlags flags);

void		 gedit_document_save_as 	(GeditDocument       *doc,	
						 const gchar         *uri, 
						 const GeditEncoding *encoding,
						 GeditDocumentSaveFlags flags);

gboolean	 gedit_document_is_untouched 	(GeditDocument       *doc);
gboolean	 gedit_document_is_untitled 	(GeditDocument       *doc);

gboolean	 gedit_document_get_deleted	(GeditDocument       *doc);
/* Ancora da discutere
gboolean	 gedit_document_get_externally_modified
						(GeditDocument       *doc);
*/

gboolean	 gedit_document_goto_line 	(GeditDocument       *doc, 
						 gint                 line);

void		 gedit_document_set_search_text	(GeditDocument       *doc,
						 const gchar         *text,
						 guint                flags);
						 
gchar		*gedit_document_get_search_text	(GeditDocument       *doc,
						 guint               *flags);

gboolean	 gedit_document_get_can_search_again
						(GeditDocument       *doc);

gboolean	 gedit_document_search_forward	(GeditDocument       *doc,
						 const GtkTextIter   *start,
						 const GtkTextIter   *end,
						 GtkTextIter         *match_start,
						 GtkTextIter         *match_end);
						 
gboolean	 gedit_document_search_backward	(GeditDocument       *doc,
						 const GtkTextIter   *start,
						 const GtkTextIter   *end,
						 GtkTextIter         *match_start,
						 GtkTextIter         *match_end);

gint		 gedit_document_replace_all 	(GeditDocument       *doc,
				            	 const gchar         *find, 
						 const gchar         *replace, 
					    	 guint                flags);

void 		 gedit_document_set_language 	(GeditDocument       *doc,
						 GtkSourceLanguage   *lang);
GtkSourceLanguage 
		*gedit_document_get_language 	(GeditDocument       *doc);

const GeditEncoding 
		*gedit_document_get_encoding	(GeditDocument       *doc);

// CHECK: I think this can be private
void		 gedit_document_set_auto_save_enabled	
						(GeditDocument       *doc, 
						 gboolean             enable);
void		 gedit_document_set_auto_save_interval 
						(GeditDocument       *doc, 
						 gint                 interval);

/* 
 * Non exported functions
 */
gboolean	_gedit_document_is_saving_as	(GeditDocument       *doc);

// CHECK: va bene un gint?
glong		 _gedit_document_get_seconds_since_last_save_or_load 
						(GeditDocument       *doc);

/* private because the property will move to gtk */
gboolean	 _gedit_document_get_has_selection
						(GeditDocument       *doc);


/* Search macros */
#define GEDIT_SEARCH_IS_DONT_SET_FLAGS(sflags) ((sflags & GEDIT_SEARCH_DONT_SET_FLAGS) != 0)
#define GEDIT_SEARCH_SET_DONT_SET_FLAGS(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_DONT_SET_FLAGS) : (sflags &= ~GEDIT_SEARCH_DONT_SET_FLAGS))

#define GEDIT_SEARCH_IS_ENTIRE_WORD(sflags) ((sflags & GEDIT_SEARCH_ENTIRE_WORD) != 0)
#define GEDIT_SEARCH_SET_ENTIRE_WORD(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_ENTIRE_WORD) : (sflags &= ~GEDIT_SEARCH_ENTIRE_WORD))

#define GEDIT_SEARCH_IS_CASE_SENSITIVE(sflags) ((sflags &  GEDIT_SEARCH_CASE_SENSITIVE) != 0)
#define GEDIT_SEARCH_SET_CASE_SENSITIVE(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_CASE_SENSITIVE) : (sflags &= ~GEDIT_SEARCH_CASE_SENSITIVE))

G_END_DECLS

#endif /* __GEDIT_DOCUMENT_H__ */
