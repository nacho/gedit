/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 * Copyright (C) 1999 Alex Roberts and Evan Lawrence
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "undo.h"
#include "utils.h"
#include "document.h"
#include "view.h"

void views_insert (Document *doc, gedit_undo *undo);
void views_delete (Document *doc, gedit_undo *undo);
void gedit_undo_add (gchar *text, gint start_pos, gint end_pos, gint action, Document *doc);
void gedit_undo_do (GtkWidget *w, gpointer data);
void gedit_undo_redo (GtkWidget *w, gpointer data);

void
gedit_undo_add (gchar *text, gint start_pos, gint end_pos,
		gint action, Document *doc)
{
	gedit_undo *undo;

	gedit_debug_mess ("F:      undo-gedit_undo_add\n", DEBUG_UNDO);
	/*"start:%i end:%i action:%i\n", start_pos, end_pos, action);*/

	undo = g_new (gedit_undo, 1);

	undo->text = g_strdup (text);
	undo->start_pos = start_pos;
	undo->end_pos = end_pos;
	undo->action = action;
	undo->status = doc->changed;
	
	/* nuke the redo list, if its available */
	if (doc->redo)
	{
		gedit_debug_mess ("F:      undo-gedit_undo_add - Kill redo list\n", DEBUG_UNDO_DEEP);
		g_list_free (doc->redo);
		doc->redo = NULL;
	}

	doc->undo = g_list_prepend (doc->undo, undo);
}

void
gedit_undo_do (GtkWidget *w, gpointer data)
{
	Document *doc = gedit_document_current();
	gedit_undo *undo, *redo;

	gedit_debug_mess ("F:      undo-gedit_undo_do\n", DEBUG_UNDO);
	
	redo = g_new (gedit_undo, 1);
	
	if(doc->undo==NULL)
		return;
	
	/* The undo data we need is always at the top op the
	   stack. So, therefore, the first one =) */
	undo = g_list_nth_data (doc->undo, 0);
	doc->redo = g_list_prepend (doc->redo, undo);
	doc->undo = g_list_remove (doc->undo, undo);

        /*("Entering undo_do..  start:%i end:%i acction:%i\n", undo->start_pos, undo->end_pos, undo->action);*/ 
	
	if (undo->action == DELETE)
	{
		gedit_debug_mess ("F:      undo-gedit_undo_do - DELETE \n", DEBUG_UNDO_DEEP);
		/* We're inserting something that was deleted */
		if ((doc->buf->len > 0) && (undo->end_pos < doc->buf->len) && (undo->end_pos))
			doc->buf = g_string_insert (doc->buf, undo->start_pos, undo->text);
		else if (undo->end_pos == 0)
			doc->buf = g_string_prepend (doc->buf, undo->text);
		else
			doc->buf = g_string_append (doc->buf, undo->text);

		views_insert (doc, undo);
	}
	else if (undo->action == INSERT)
	{
		gedit_debug_mess ("F:      undo-gedit_undo_do - INSERT\n", DEBUG_UNDO_DEEP);
		
		/* We're deleteing somthing that had been inserted */
		if (undo->end_pos + (undo->end_pos - undo->start_pos) <= doc->buf->len)
			doc->buf = g_string_erase (doc->buf, undo->start_pos, (undo->end_pos - undo->start_pos));
		else
			doc->buf = g_string_truncate (doc->buf, undo->start_pos);

		views_delete (doc, undo);
		doc->changed = undo->status;
	}
	else
		g_assert_not_reached ();
}

void
gedit_undo_redo (GtkWidget *w, gpointer data)
{

	Document *doc = gedit_document_current();
	gedit_undo *redo;

	gedit_debug_mess ("F:      undo-gedit_undo_redo\n", DEBUG_UNDO);
	
	if (!doc->redo)
	  return;
	
	redo = g_list_nth_data (doc->redo, 0);
	doc->undo = g_list_prepend (doc->undo, redo);
	doc->redo = g_list_remove (doc->redo, redo);

	/* Now we can do the undo proper.. */
	if (redo->action == INSERT)
	{
		gedit_debug_mess ("F:      undo-gedit_undo_redo - INSERT \n", DEBUG_UNDO_DEEP);
		/* We're inserting something that was deleted */
		if ((doc->buf->len > 0) && (redo->end_pos < doc->buf->len) && (redo->end_pos))
			doc->buf = g_string_insert (doc->buf, redo->start_pos, redo->text);
		if (redo->end_pos == 0)
			doc->buf = g_string_prepend (doc->buf, redo->text);
		else
			doc->buf = g_string_append (doc->buf, redo->text);

		/* PRINT THE INFO BEFORE inserting */
		views_insert (doc, redo);
	}
	else if (redo->action == DELETE)
	{
		gedit_debug_mess ("F:      undo-gedit_undo_redo - DELETE \n", DEBUG_UNDO_DEEP);
		/* We're deleteing somthing that had been inserted */
		if (redo->end_pos + (redo->end_pos - redo->start_pos) <= doc->buf->len)
			doc->buf = g_string_erase (doc->buf, redo->start_pos, (redo->end_pos - redo->start_pos));
		else
			doc->buf = g_string_truncate (doc->buf, redo->start_pos);

		views_delete (doc, redo);
		/*doc->changed = undo->status; this line by chema */
	}
	else 
		g_assert_not_reached ();
}

void
views_insert (Document *doc, gedit_undo *undo)
{
	gint i;
	gint p1;
	View *view;

	gedit_debug_mess ("F:      undo-views_insert\n", DEBUG_UNDO);
	
	for (i = 0; i < g_list_length (doc->views); i++)
	{
		view = g_list_nth_data (doc->views, i);
		gtk_text_freeze (GTK_TEXT (view->text));
		p1 = gtk_text_get_point (GTK_TEXT (view->text));
		gtk_text_set_point (GTK_TEXT(view->text), undo->start_pos);
		gtk_text_thaw (GTK_TEXT (view->text));
		gtk_text_insert (GTK_TEXT (view->text), NULL,
				 NULL, NULL, undo->text,
				 strlen(undo->text));
		gtk_text_freeze (GTK_TEXT (view->split_screen));
		p1 = gtk_text_get_point (GTK_TEXT (view->split_screen));
		gtk_text_set_point (GTK_TEXT(view->split_screen), undo->start_pos);
		gtk_text_thaw (GTK_TEXT (view->split_screen));
		gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
				 NULL, NULL, undo->text,
				 strlen(undo->text));
	}  
}

void
views_delete (Document *doc, gedit_undo *undo)
{
	View *nth_view;
	gint n;
	gint p1;
	gint start_pos, end_pos;

	start_pos = undo->start_pos;
	end_pos = undo->end_pos;

	gedit_debug_mess ("F:      undo-views_insert\n", DEBUG_UNDO);
	
	/*("Entering views_del. start:%i end:%i acction:%i\n", undo->start_pos, undo->end_pos, undo->action);*/

	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);

		gtk_text_freeze (GTK_TEXT (nth_view->text));
	        p1 = gtk_text_get_point (GTK_TEXT (nth_view->text));
		gtk_text_set_point (GTK_TEXT(nth_view->text), end_pos);
		gtk_text_thaw (GTK_TEXT (nth_view->text)); /* thaw before deleting so that cursos repositions ok */
		gtk_text_backward_delete (GTK_TEXT (nth_view->text), (end_pos - start_pos));
			
		gtk_text_freeze (GTK_TEXT (nth_view->split_screen));
	        p1 = gtk_text_get_point (GTK_TEXT (nth_view->split_screen));
		gtk_text_set_point (GTK_TEXT(nth_view->split_screen), start_pos);
		gtk_text_thaw (GTK_TEXT (nth_view->split_screen));
		gtk_text_backward_delete (GTK_TEXT (nth_view->split_screen), (end_pos - start_pos));
		/* I have not tried it but WHY whould you use backward above and forward here ? Chema
		gtk_text_forward_delete (GTK_TEXT (nth_view->split_screen), (end_pos - start_pos));
		*/
	}
}













