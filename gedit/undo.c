/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 * Copyright (C) 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach & Jose M Celorio
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
static void gedit_undo_free_list (GList ** list_pointer);
static gint gedit_undo_merge (gedit_undo * last_undo, guint start_pos, guint end_pos, gint action, guchar * text);


void
gedit_undo_add (gchar *text, gint start_pos, gint end_pos,
		gint action, Document *doc, View *view)
{
	gedit_undo *undo, *last_undo;

	gedit_debug ("", DEBUG_UNDO);

	gedit_undo_free_list (&doc->redo);

	if (doc->undo)
	{
		last_undo = g_list_nth_data (doc->undo, 0);
		if (gedit_undo_merge( last_undo, start_pos, end_pos, action, text))
			return;
	}

	undo = g_new (gedit_undo, 1);

	undo->text = g_strdup (text);
	undo->start_pos = start_pos;
	undo->end_pos = end_pos;
	undo->action = action;
	undo->status = doc->changed;
	undo->window_position = GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->value;
	undo->mergeable = TRUE;
	if (end_pos-start_pos!=1 || text[0]=='\n')
		undo->mergeable = FALSE;			

	doc->undo = g_list_prepend (doc->undo, undo);


	/*
	g_print("The undo list size is : %i taking %i bytes \n\n",
		g_list_length (doc->undo),
		g_list_length (doc->undo) * sizeof (gedit_undo));
	*/
}


/**
 * gedit_undo_merge:
 * @last_undo: 
 * @start_pos: 
 * @end_pos: 
 * @action: 
 * 
 * This function tries to merge an undo object with a new set of data
 * 
 * Return Value: TRUE is merge was sucessful, FALSE otherwise
 **/
static gint
gedit_undo_merge (gedit_undo *last_undo, guint start_pos, guint end_pos, gint action, guchar* text)
{
	guchar *temp_string;
	
	gedit_debug ("", DEBUG_UNDO);
	/* This are the cases in which we will not merge :
	   1. if (last_undo->mergeable == FALSE)
	   [mergeable = FALSE when the size of the undo data was not 1.
	   or if the data was size = 1 but = '\n']
	   2. The size of text is not 1
	   3. If the new merging data is a '\n'
	   4. If the last char of the undo_last data is a space/tab
	   and the new char is not a space/tab ( so that we undo
	   words and not chars )
	   5. If the type (action) of undo is different
	Chema */

	if (!last_undo->mergeable)
	{
		gedit_debug ("Previous Cell is not mergeable...", DEBUG_UNDO);
		return FALSE;
	}

	if (end_pos-start_pos != 1)
	{
		gedit_debug ("The size of the new data is not 1...", DEBUG_UNDO);
		return FALSE;
	}

	if (text[0]=='\n')
	{
		gedit_debug ("The data is a new line...", DEBUG_UNDO);
		return FALSE;
	}


	if (action != last_undo->action)
	{
		gedit_debug ("The action is different...", DEBUG_UNDO);
		return FALSE;
	}

	if (action == GEDIT_UNDO_DELETE)
	{
		if (last_undo->start_pos != end_pos)
		{
			gedit_debug ("The text is not in the same position.", DEBUG_UNDO);
			return FALSE;
		}
		
		if ( text[0]!=' ' && text[0]!='\t' &&
		     (last_undo->text [0] ==' '
		      || last_undo->text [0] == '\t'))
		{
			gedit_debug ("The text is a space/tab but the previous char is not...", DEBUG_UNDO);
			return FALSE;
		}

		temp_string = g_strdup_printf ("%s%s", text, last_undo->text);
		g_free (last_undo->text);
		last_undo->start_pos = start_pos;
		last_undo->text = temp_string;
	}
	else if (action == GEDIT_UNDO_INSERT)
	{
		if (last_undo->end_pos != start_pos)
		{
			gedit_debug ("The text is not in the same position.", DEBUG_UNDO);
			return FALSE;
		}

		if ( text[0]!=' ' && text[0]!='\t' &&
		     (last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] ==' '
		      || last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] == '\t'))
		{
			gedit_debug ("The text is a space/tab but the previous char is not...", DEBUG_UNDO);
			return FALSE;
		}

		temp_string = g_strdup_printf ("%s%s", last_undo->text, text);
		g_free (last_undo->text);
		last_undo->end_pos = end_pos;
		last_undo->text = temp_string;
	}
	else
		g_assert_not_reached();


	return TRUE;
}


void
gedit_undo_do (GtkWidget *w, gpointer data)
{
	Document *doc = gedit_document_current();
	gedit_undo *undo;

	gedit_debug ("", DEBUG_UNDO);

	if (doc==NULL || doc->undo==NULL)
		return;

	/* The undo data we need is always at the top op the
	   stack. So, therefore, the first one =) */
	undo = g_list_nth_data (doc->undo, 0);
	doc->redo = g_list_prepend (doc->redo, undo);
	doc->undo = g_list_remove (doc->undo, undo);

	/* Move the view (scrollbars) to the correct position */
	gtk_adjustment_set_value (GTK_ADJUSTMENT(GTK_TEXT( gedit_view_current()->text)->vadj),
				  undo->window_position);
	
	if (undo->action == GEDIT_UNDO_DELETE)
		/* We're inserting something that was deleted */
		views_insert (doc, undo->start_pos, undo->text, undo->end_pos - undo->start_pos, NULL);
	else if (undo->action == GEDIT_UNDO_INSERT)
		/* We're deleteing somthing that had been inserted */
		views_delete (doc, undo->start_pos, undo->end_pos);
        	/*doc->changed = undo->status; Disabled by chema. See below..*/
	else
		g_assert_not_reached ();
}

void
gedit_undo_redo (GtkWidget *w, gpointer data)
{
	Document *doc = gedit_document_current();
	gedit_undo *redo;

	gedit_debug ("", DEBUG_UNDO);

	if (doc==NULL || doc->redo==NULL)
		return;
	
	redo = g_list_nth_data (doc->redo, 0);
	doc->undo = g_list_prepend (doc->undo, redo);
	doc->redo = g_list_remove (doc->redo, redo);

	/* Move the view to the right position. */
	gtk_adjustment_set_value (GTK_ADJUSTMENT(GTK_TEXT(gedit_view_current()->text)->vadj),
				  redo->window_position);
				  
	if (redo->action == GEDIT_UNDO_INSERT)
		/* We're inserting something that was deleted */
		views_insert (doc, redo->start_pos, redo->text, redo->end_pos - redo->start_pos, NULL);
	else if (redo->action == GEDIT_UNDO_DELETE)
		/* We're deleteing somthing that had been inserted */
		views_delete (doc, redo->start_pos, redo->end_pos);
		/*doc->changed = undo->status; disable by chema. We need to call the appropiate function
		  to actually change the title and stuff.*/
	else 
		g_assert_not_reached ();
}

static void
gedit_undo_free_list (GList ** list_pointer)
{
	gint n;
	gedit_undo *nth_redo;
	GList *list = * list_pointer;
	
	gedit_debug ("", DEBUG_UNDO);

	if (list==NULL)
	{
		gedit_debug ("Nothing to free", DEBUG_UNDO);
		return;
	}
	
	for (n=0; n < g_list_length (list); n++)
	{
		nth_redo = g_list_nth_data (list, n);
		if (nth_redo==NULL)
			g_warning ("nth_redo==NULL");
		g_free (nth_redo->text);
		g_free (nth_redo);
	}

	g_print("Removed %i objects\n", n);
	g_list_free (list);
	*list_pointer = NULL;
}

