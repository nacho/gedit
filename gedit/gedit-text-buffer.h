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
 
#ifndef __GEDIT_TEXT_BUFFER_H__
#define __GEDIT_TEXT_BUFFER_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourcebuffer.h>


G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_TEXT_BUFFER              (gedit_text_buffer_get_type())
#define GEDIT_TEXT_BUFFER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_TEXT_BUFFER, GeditTextBuffer))
#define GEDIT_TEXT_BUFFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_TEXT_BUFFER, GeditTextBufferClass))
#define GEDIT_IS_TEXT_BUFFER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_TEXT_BUFFER))
#define GEDIT_IS_TEXT_BUFFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_TEXT_BUFFER))
#define GEDIT_TEXT_BUFFER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_TEXT_BUFFER, GeditTextBufferClass))


/* Private structure type */
typedef struct _GeditTextBufferPrivate    GeditTextBufferPrivate;

/*
 * Main object structure
 */
typedef struct _GeditTextBuffer           GeditTextBuffer;
 
struct _GeditTextBuffer
{
	GtkSourceBuffer buffer;
	
	/*< private > */
	GeditTextBufferPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditTextBufferClass 	GeditTextBufferClass;

struct _GeditTextBufferClass
{
	GtkSourceBufferClass parent_class;
	
	/* signals */
	void (* cursor_moved)		(GeditTextBuffer    *document);
	
	void (* search_highlight_updated)
					(GeditTextBuffer    *document,
					 GtkTextIter      *start,
					 GtkTextIter      *end);
};


GType		 gedit_text_buffer_get_type		(void) G_GNUC_CONST;

GeditDocument	*gedit_text_buffer_new			(void);

void		 gedit_text_buffer_set_search_text	(GeditTextBuffer     *doc,
							 const gchar         *text,
							 guint                flags);
						 
gchar		*gedit_text_buffer_get_search_text	(GeditTextBuffer     *doc,
							 guint               *flags);

gboolean	 gedit_text_buffer_get_can_search_again	(GeditTextBuffer     *doc);

gboolean	 gedit_text_buffer_search_forward	(GeditTextBuffer     *doc,
							 const GtkTextIter   *start,
							 const GtkTextIter   *end,
							 GtkTextIter         *match_start,
							 GtkTextIter         *match_end);
						 
gboolean	 gedit_text_buffer_search_backward	(GeditTextBuffer     *doc,
							 const GtkTextIter   *start,
							 const GtkTextIter   *end,
							 GtkTextIter         *match_start,
							 GtkTextIter         *match_end);

gint		 gedit_text_buffer_replace_all		(GeditTextBuffer     *doc,
							 const gchar         *find, 
							 const gchar         *replace, 
							 guint                flags);

void 		 gedit_text_buffer_set_language 	(GeditTextBuffer     *doc,
							 GtkSourceLanguage   *lang);
GtkSourceLanguage 
		*gedit_text_buffer_get_language 	(GeditTextBuffer     *doc);

void		 gedit_text_buffer_set_enable_search_highlighting 
							(GeditTextBuffer     *doc,
							 gboolean             enable);

gboolean	 gedit_text_buffer_get_enable_search_highlighting
							(GeditTextBuffer     *doc);

void		 gedit_text_buffer_get_cursor_position	(GeditTextBuffer *buffer,
							 gint  tab_size,
							 gint *row,
							 gint *col);

/* 
 * Non exported functions
 */

/* Note: this is a sync stat: use only on local files */
void		_gedit_text_buffer_search_region	(GeditTextBuffer     *doc,
							 const GtkTextIter   *start,
							 const GtkTextIter   *end);

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

/* FIXME
typedef GMountOperation *(*GeditMountOperationFactory)(GeditTextBuffer *doc,
						       gpointer         userdata);

void		 _gedit_text_buffer_set_mount_operation_factory
							(GeditTextBuffer	    *doc,
							 GeditMountOperationFactory  callback,
							 gpointer	             userdata);
GMountOperation
		*_gedit_text_buffer_create_mount_operation
							(GeditTextBuffer	    *doc);*/

G_END_DECLS

#endif /* __GEDIT_TEXT_BUFFER_H__ */
