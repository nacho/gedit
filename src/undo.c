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

void gedit_undo_add (gchar *text, gint start_pos, gint end_pos, gint action, Document *doc, View *view);
void gedit_undo_do (GtkWidget *w, gpointer data);
void gedit_undo_redo (GtkWidget *w, gpointer data);

void
gedit_undo_add (gchar *text, gint start_pos, gint end_pos,
		gint action, Document *doc, View *view)
{
	gedit_undo *undo;

	gedit_debug ("\n", DEBUG_UNDO);

	undo = g_new (gedit_undo, 1);

	undo->text = g_strdup (text);
	undo->start_pos = start_pos;
	undo->end_pos = end_pos;
	undo->action = action;
	undo->status = doc->changed;

	undo->view = view;
	undo->window_position = GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->value;

	/* nuke the redo list, if its available */
	if (doc->redo)
	{
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

	gedit_debug ("\n", DEBUG_UNDO);

	if (doc==NULL)
		return;
	
	if(doc->undo==NULL)
		return;

	redo = g_new (gedit_undo, 1);
	
	/* The undo data we need is always at the top op the
	   stack. So, therefore, the first one =) */
	undo = g_list_nth_data (doc->undo, 0);
	doc->redo = g_list_prepend (doc->redo, undo);
	doc->undo = g_list_remove (doc->undo, undo);

        /*("Entering undo_do..  start:%i end:%i acction:%i\n", undo->start_pos, undo->end_pos, undo->action);*/ 

	gtk_adjustment_set_value (GTK_ADJUSTMENT(GTK_TEXT(VIEW(undo->view)->text)->vadj),
				  undo->window_position);
	
	if (undo->action == DELETE)
	{
		/* We're inserting something that was deleted */
		/*
		if ((doc->buf->len > 0) && (undo->end_pos < doc->buf->len) && (undo->end_pos))
			doc->buf = g_string_insert (doc->buf, undo->start_pos, undo->text);
		else if (undo->end_pos == 0)
			doc->buf = g_string_prepend (doc->buf, undo->text);
		else
			doc->buf = g_string_append (doc->buf, undo->text);
		*/
		views_insert (doc, undo->start_pos, undo->text, undo->end_pos - undo->start_pos, NULL);
	}
	else if (undo->action == INSERT)
	{

		/* We're deleteing somthing that had been inserted */
		/*
		if (undo->end_pos + (undo->end_pos - undo->start_pos) <= doc->buf->len)
			doc->buf = g_string_erase (doc->buf, undo->start_pos, (undo->end_pos - undo->start_pos));
		else
			doc->buf = g_string_truncate (doc->buf, undo->start_pos);
		*/
		views_delete (doc, undo->start_pos, undo->end_pos);
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

	gedit_debug ("\n", DEBUG_UNDO);

	if (doc==NULL)
		return;

	if (!doc->redo)
		return;
	
	redo = g_list_nth_data (doc->redo, 0);
	doc->undo = g_list_prepend (doc->undo, redo);
	doc->redo = g_list_remove (doc->redo, redo);

	gtk_adjustment_set_value (GTK_ADJUSTMENT(GTK_TEXT(VIEW(redo->view)->text)->vadj),
				  redo->window_position);
				  
	/* Now we can do the undo proper.. */
	if (redo->action == INSERT)
	{
		/* We're inserting something that was deleted */
		/*
		if ((doc->buf->len > 0) && (redo->end_pos < doc->buf->len) && (redo->end_pos))
			doc->buf = g_string_insert (doc->buf, redo->start_pos, redo->text);
		if (redo->end_pos == 0)
			doc->buf = g_string_prepend (doc->buf, redo->text);
		else
			doc->buf = g_string_append (doc->buf, redo->text);
		*/

		/* PRINT THE INFO BEFORE inserting */
		views_insert (doc, redo->start_pos, redo->text, redo->end_pos - redo->start_pos, NULL);
	}
	else if (redo->action == DELETE)
	{
		/* We're deleteing somthing that had been inserted */
		/*
		if (redo->end_pos + (redo->end_pos - redo->start_pos) <= doc->buf->len)
			doc->buf = g_string_erase (doc->buf, redo->start_pos, (redo->end_pos - redo->start_pos));
		else
			doc->buf = g_string_truncate (doc->buf, redo->start_pos);
		*/

		views_delete (doc, redo->start_pos, redo->end_pos);
		/*doc->changed = undo->status; this line by chema */
	}
	else 
		g_assert_not_reached ();
}

