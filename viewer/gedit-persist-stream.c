/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-persist-stream.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 Jeroen Zwartepoorte
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <libbonobo.h>

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

#include "gedit-persist-stream.h"
#include <gedit/gedit-convert.h>

static int
impl_save (BonoboPersistStream       *ps,
	   const Bonobo_Stream        stream,
	   Bonobo_Persist_ContentType type,
	   void                      *closure,
	   CORBA_Environment         *ev)
{
	g_message (_("Save method not implemented for gedit viewer"));

	return 0;
}

static int
impl_load (BonoboPersistStream       *ps,
	   Bonobo_Stream              stream,
	   Bonobo_Persist_ContentType type,
	   void                      *closure,
	   CORBA_Environment         *ev)
{
	GtkSourceView *source_view = GTK_SOURCE_VIEW (closure);
	GtkTextView *view = GTK_TEXT_VIEW (source_view);
	GtkTextBuffer *buffer = view->buffer;
	GtkTextIter start;
	GtkTextIter end;
	GtkSourceLanguagesManager *manager;
	GtkSourceLanguage *language = NULL;
	GString *text_buf;
	guint read_size = 16 *1024;

	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (buffer));

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	gtk_text_buffer_delete (buffer, &start, &end);

	manager = g_object_get_data (G_OBJECT (source_view), "languages-manager");
	language = gtk_source_languages_manager_get_language_from_mime_type (manager,
									     type);
	if (language) {
		g_object_set (G_OBJECT (buffer), "highlight", TRUE, NULL);
		gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
	}

	text_buf = g_string_new ("");

	/* Read the complete stream first. */
	while (TRUE) {
		Bonobo_Stream_iobuf *buf;

		Bonobo_Stream_read (stream, read_size, &buf, ev);

		if (ev->_major != CORBA_NO_EXCEPTION)
			break;

		if (buf->_length)
			text_buf = g_string_append_len (text_buf, buf->_buffer, buf->_length);

		if (buf->_length < read_size)
			break;
	}

	if (text_buf->len > 0) {
		gchar *converted_text;
		gint len = 0;

		if (g_utf8_validate (text_buf->str, text_buf->len, NULL)) {
			converted_text = text_buf->str;
			len = text_buf->len;
		} else {
			converted_text = gedit_convert_to_utf8 (text_buf->str,
								text_buf->len,
								NULL,
								NULL);
			if (converted_text != NULL)
			{
				len = strlen (converted_text);
			}	
			
			g_free (text_buf->str);
			
		}

		if (converted_text == NULL) {
			g_warning (_("Invalid UTF-8 data"));
			g_string_free (text_buf, FALSE);
			return FALSE;
		}

		gtk_text_buffer_get_end_iter (buffer, &end);
		gtk_text_buffer_insert (buffer,
					&end,
					converted_text,
					len);
		g_free (converted_text);
	}

	g_string_free (text_buf, FALSE);

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_place_cursor (buffer, &start);
	gtk_text_view_place_cursor_onscreen (view);
	gtk_text_buffer_set_modified (buffer, FALSE);

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (buffer));

	return 0;
}

static Bonobo_Persist_ContentTypeList *
impl_get_content_types (BonoboPersistStream *ps,
			gpointer             data,
			CORBA_Environment   *ev)
{
	return bonobo_persist_generate_content_types (1, "text/*");
}

BonoboPersistStream *
gedit_persist_stream_new (GtkSourceView *view)
{
	BonoboPersistStream *stream;

	stream = bonobo_persist_stream_new ((BonoboPersistStreamIOFn) impl_load,
					    (BonoboPersistStreamIOFn) impl_save,
					    (BonoboPersistStreamTypesFn) impl_get_content_types,
					    "GeditPersistStream",
					    view);

	g_object_set_data (G_OBJECT (view), "PersistStream", stream);

	return stream;
}
