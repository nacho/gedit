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

#include <glib-object.h>
#include <gio/gio.h>
#include <gedit/gedit-encodings.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_DOCUMENT			(gedit_document_get_type ())
#define GEDIT_DOCUMENT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_DOCUMENT, GeditDocument))
#define GEDIT_IS_DOCUMENT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_DOCUMENT))
#define GEDIT_DOCUMENT_GET_INTERFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEDIT_TYPE_DOCUMENT, GeditDocumentIface))

typedef struct _GeditDocument		GeditDocument;
typedef struct _GeditDocumentIface	GeditDocumentIface;

typedef enum
{
	GEDIT_SEARCH_DONT_SET_FLAGS	= 1 << 0, 
	GEDIT_SEARCH_ENTIRE_WORD	= 1 << 1,
	GEDIT_SEARCH_CASE_SENSITIVE	= 1 << 2

} GeditSearchFlags;

/**
 * GeditDocumentSaveFlags:
 * @GEDIT_DOCUMENT_SAVE_IGNORE_MTIME: save file despite external modifications.
 * @GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP: write the file directly without attempting to backup.
 * @GEDIT_DOCUMENT_SAVE_PRESERVE_BACKUP: preserve previous backup file, needed to support autosaving.
 */
typedef enum
{
	GEDIT_DOCUMENT_SAVE_IGNORE_MTIME 	= 1 << 0,
	GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP	= 1 << 1,
	GEDIT_DOCUMENT_SAVE_PRESERVE_BACKUP	= 1 << 2
} GeditDocumentSaveFlags;

struct _GeditDocumentIface
{
	GTypeInterface parent;
	
	/* Signals */
	
	void (* modified_changed)	(GeditDocument       *document);

	/* Document load */
	void (* load)			(GeditDocument       *document,
					 const gchar         *uri,
					 const GeditEncoding *encoding,
					 gint                 line_pos,
					 gboolean             create);

	void (* loading)		(GeditDocument    *document,
					 goffset	   size,
					 goffset	   total_size);

	void (* loaded)			(GeditDocument    *document,
					 const GError     *error);

	/* Document save */
	void (* save)			(GeditDocument          *document,
					 const gchar            *uri,
					 const GeditEncoding    *encoding,
					 GeditDocumentSaveFlags  flags);

	void (* saving)			(GeditDocument    *document,
					 goffset	   size,
					 goffset	   total_size);

	void (* saved)  		(GeditDocument    *document,
					 const GError     *error);
	
	/* Virtual table */
	
	GFile * (* get_location)	(GeditDocument *doc);
	
	gchar * (* get_uri)		(GeditDocument *doc);
	
	void (* set_uri)		(GeditDocument *doc,
					 const gchar   *uri);
	
	gchar * (* get_uri_for_display)	(GeditDocument *doc);
	
	gchar * (* get_short_name_for_display)
					(GeditDocument *doc);
	
	gchar * (* get_content_type)	(GeditDocument *doc);
	
	gchar * (* get_mime_type)	(GeditDocument *doc);
	
	gboolean (* get_readonly)	(GeditDocument *doc);

	gboolean (* load_cancel)	(GeditDocument       *doc);

	gboolean (* is_untouched)	(GeditDocument       *doc);
	gboolean (* is_untitled)	(GeditDocument       *doc);
	
	gboolean (* is_local)		(GeditDocument       *doc);
	
	gboolean (* get_deleted)	(GeditDocument       *doc);
	
	gboolean (* goto_line)	 	(GeditDocument       *doc, 
					 gint                 line);

	gboolean (* goto_line_offset)	(GeditDocument *doc,
					 gint           line,
					 gint           line_offset);

	const GeditEncoding * (* get_encoding)
					(GeditDocument       *doc);
	
	glong (* get_seconds_since_last_save_or_load)
					(GeditDocument       *doc);
	
	gboolean (* check_externally_modified)
					(GeditDocument       *doc);
	
	/* GSV releated functions */
	
	void (* undo)			(GeditDocument       *doc);
	void (* redo)			(GeditDocument       *doc);
	
	gboolean (* can_undo)		(GeditDocument       *doc);
	gboolean (* can_redo)		(GeditDocument       *doc);
	
	void (* begin_not_undoable_action) (GeditDocument    *doc);
	void (* end_not_undoable_action) (GeditDocument      *doc);
	
	/* GtkTextBuffer releated functions */
	
	void (* set_text)		(GeditDocument       *doc,
					 const gchar         *text,
					 gint                 len);

	void (* set_modified)		(GeditDocument       *doc,
					 gboolean             setting);
	
	gboolean (* get_modified)	(GeditDocument       *doc);
	
	gboolean (* get_has_selection)	(GeditDocument       *doc);
};

#define GEDIT_DOCUMENT_ERROR gedit_document_error_quark ()

enum
{
	GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED,
	GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
	GEDIT_DOCUMENT_ERROR_TOO_BIG,
	GEDIT_DOCUMENT_NUM_ERRORS 
};

GQuark		 gedit_document_error_quark	(void);

GType		 gedit_document_get_type	(void) G_GNUC_CONST;

GFile		*gedit_document_get_location	(GeditDocument       *doc);

gchar		*gedit_document_get_uri 	(GeditDocument       *doc);
void		 gedit_document_set_uri		(GeditDocument       *doc,
						 const gchar 	     *uri);

gchar		*gedit_document_get_uri_for_display
						(GeditDocument       *doc);
gchar		*gedit_document_get_short_name_for_display
					 	(GeditDocument       *doc);

gchar		*gedit_document_get_content_type
					 	(GeditDocument       *doc);

gchar		*gedit_document_get_mime_type 	(GeditDocument       *doc);

gboolean	 gedit_document_get_readonly 	(GeditDocument       *doc);

void		 gedit_document_load		(GeditDocument       *doc,
						 const gchar         *uri,
						 const GeditEncoding *encoding,
						 gint                 line_pos,
						 gboolean             create); 

gboolean	 gedit_document_load_cancel	(GeditDocument       *doc);

void		 gedit_document_save		(GeditDocument       *doc,
						 GeditDocumentSaveFlags flags);

void		 gedit_document_save_as 	(GeditDocument       *doc,
						 const gchar         *uri,
						 const GeditEncoding *encoding,
						 GeditDocumentSaveFlags flags);

gboolean	 gedit_document_is_untouched 	(GeditDocument       *doc);
gboolean	 gedit_document_is_untitled 	(GeditDocument       *doc);

gboolean	 gedit_document_is_local	(GeditDocument       *doc);

gboolean	 gedit_document_get_deleted	(GeditDocument       *doc);

gboolean	 gedit_document_goto_line 	(GeditDocument       *doc, 
						 gint                 line);

gboolean	 gedit_document_goto_line_offset(GeditDocument       *doc,
						 gint                 line,
						 gint                 line_offset);

const GeditEncoding 
		*gedit_document_get_encoding	(GeditDocument       *doc);

glong		 gedit_document_get_seconds_since_last_save_or_load 
						(GeditDocument       *doc);

/* Note: this is a sync stat: use only on local files */
gboolean	 gedit_document_check_externally_modified
						(GeditDocument       *doc);


void		 gedit_document_undo		(GeditDocument       *doc);
void		 gedit_document_redo		(GeditDocument       *doc);

gboolean	 gedit_document_can_undo	(GeditDocument       *doc);
gboolean	 gedit_document_can_redo	(GeditDocument       *doc);

void		 gedit_document_begin_not_undoable_action
						(GeditDocument       *doc);
void		 gedit_document_end_not_undoable_action
						(GeditDocument       *doc);

void		 gedit_document_set_text	(GeditDocument       *doc,
						 const gchar         *text,
						 gint                 len);

void		 gedit_document_set_modified	(GeditDocument       *doc,
						 gboolean             setting);

gboolean	 gedit_document_get_modified	(GeditDocument       *doc);

gboolean	 gedit_document_get_has_selection(GeditDocument      *doc);

G_END_DECLS

#endif /* __GEDIT_DOCUMENT_H__ */
