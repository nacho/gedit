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
#include "prefs.h"


static void gedit_undo_check_size (Document *doc);
       void gedit_undo_add (gchar *text, gint start_pos, gint end_pos, gint action, Document *doc, View *view);
       void gedit_undo_do (GtkWidget *w, gpointer data);
       void gedit_undo_redo (GtkWidget *w, gpointer data);
       void gedit_undo_free_list (GList **list_pointer);
static gint gedit_undo_merge (GList *list, guint start_pos, guint end_pos, gint action, guchar * text);

/**
 * gedit_undo_check_size:
 * @doc: document to check
 * 
 * Checks that the size of doc->undo does not excede settings->undo_levels and
 * frees any undo level above sett->undo_level.
 *
 **/
static void
gedit_undo_check_size (Document *doc)
{
	gint n;
	GeditUndoInfo *nth_undo;
	
	if (settings->undo_levels < 1)
		return;
	
	/* No need to check for the redo list size, at least for now
	   since the undo list gets freed on any call to gedit_undo_add */
	if (g_list_length (doc->undo) > settings->undo_levels && settings->undo_levels > 0)
	{
		for (n=settings->undo_levels; n < g_list_length (doc->undo); n++)
		{
			nth_undo = g_list_nth_data (doc->undo, n);
			doc->undo = g_list_remove (doc->undo, nth_undo);
			g_free (nth_undo->text);
			g_free (nth_undo);
		}
	}

#if 0	
	g_print("The undo list size is : %i taking %i bytes. Levels are set at :%i \n\n",
		g_list_length (doc->undo),
		g_list_length (doc->undo) * sizeof (GeditUndoInfo),
		settings->undo_levels);
	for (n=0; n < g_list_length (doc->undo); n++)
	{
		nth_undo = g_list_nth_data (doc->undo, n);
		if (nth_undo==NULL)
			g_warning ("nth_undo==NULL");
		g_print ("Level:%i Type :%i Text:%s Size:%i\n",
			 n, nth_undo->action, nth_undo->text, nth_undo->end_pos - nth_undo->start_pos);
	}
#endif
}

/**
 * gedit_undo_add:
 * @text: 
 * @start_pos: 
 * @end_pos: 
 * @action: either GEDIT_UNDO_INSERT or GEDIT_UNDO_DELETE
 * @doc: 
 * @view: The view so that we save the scroll bar position.
 * 
 * Adds text to the undo stack. It also performs test to limit the number
 * of undo levels and deltes the redo list
 **/
void
gedit_undo_add (gchar *text, gint start_pos, gint end_pos,
		gint action, Document *doc, View *view)
{
	GeditUndoInfo *undo;

	gedit_debug ("", DEBUG_UNDO);

	gedit_undo_free_list (&doc->redo);

	if (gedit_undo_merge( doc->undo, start_pos, end_pos, action, text))
		return;

	if (view==NULL)
		view = gedit_view_current ();
	
	undo = g_new (GeditUndoInfo, 1);
	undo->text = g_strdup (text);
	undo->start_pos = start_pos;
	undo->end_pos = end_pos;
	undo->action = action;
	undo->status = doc->changed;
	undo->window_position = gedit_view_get_window_position (view);
	if (end_pos-start_pos!=1 || text[0]=='\n')
		undo->mergeable = FALSE;
	else
		undo->mergeable = TRUE;

	doc->undo = g_list_prepend (doc->undo, undo);

	gedit_undo_check_size (doc);
}


/**
 * gedit_undo_merge:
 * @last_undo: 
 * @start_pos: 
 * @end_pos: 
 * @action: 
 * 
 * This function tries to merge the undo object at the top of
 * the stack with a new set of data
 * 
 * Return Value: TRUE is merge was sucessful, FALSE otherwise
 **/
static gint
gedit_undo_merge (GList *list, guint start_pos, guint end_pos, gint action, guchar* text)
{
	guchar * temp_string;
	GeditUndoInfo * last_undo;
	
	gedit_debug ("", DEBUG_UNDO);
	/* This are the cases in which we will not merge :
	   1. if (last_undo->mergeable == FALSE)
	   [mergeable = FALSE when the size of the undo data was not 1.
	   or if the data was size = 1 but = '\n' or if the undo object
	   has been "undone" ]
	   2. The size of text is not 1
	   3. If the new merging data is a '\n'
	   4. If the last char of the undo_last data is a space/tab
	   and the new char is not a space/tab ( so that we undo
	   words and not chars )
	   5. If the type (action) of undo is different
	Chema */

	if (list==NULL)
		return FALSE;
	
	last_undo = g_list_nth_data (list, 0);

	if (!last_undo->mergeable)
	{
		gedit_debug ("Previous Cell is not mergeable...", DEBUG_UNDO);
		return FALSE;
	}

	if (end_pos-start_pos != 1)
	{
		gedit_debug ("The size of the new data is not 1...", DEBUG_UNDO);
		last_undo->mergeable = FALSE;
		return FALSE;
	}

	if (text[0]=='\n')
	{
		gedit_debug ("The data is a new line...", DEBUG_UNDO);
		last_undo->mergeable = FALSE;		
		return FALSE;
	}

	if (action != last_undo->action)
	{
		gedit_debug ("The action is different...", DEBUG_UNDO);
		last_undo->mergeable = FALSE;
		return FALSE;
	}

	if (action == GEDIT_UNDO_DELETE)
	{
		if (last_undo->start_pos!=end_pos && last_undo->start_pos != start_pos)
		{
			gedit_debug ("The text is not in the same position.", DEBUG_UNDO);
			last_undo->mergeable = FALSE;			
			return FALSE;
		}

		if (last_undo->start_pos == start_pos)
		{
			/* Deleted with the delete key */
			if ( text[0]!=' ' && text[0]!='\t' &&
			     (last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] ==' '
			      || last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] == '\t'))
			{
				gedit_debug ("The text is a space/tab but the previous char is not...", DEBUG_UNDO);
				last_undo->mergeable = FALSE;			
				return FALSE;
			}
			
			temp_string = g_strdup_printf ("%s%s", last_undo->text, text);
			g_free (last_undo->text);
			last_undo->end_pos += 1;
			last_undo->text = temp_string;
		}
		else
		{
			/* Deleted with the backspace key */
			if ( text[0]!=' ' && text[0]!='\t' &&
			     (last_undo->text [0] ==' '
			      || last_undo->text [0] == '\t'))
			{
				gedit_debug ("The text is a space/tab but the previous char is not...", DEBUG_UNDO);
				last_undo->mergeable = FALSE;			
				return FALSE;
			}

			temp_string = g_strdup_printf ("%s%s", text, last_undo->text);
			g_free (last_undo->text);
			last_undo->start_pos = start_pos;
			last_undo->text = temp_string;
		}
	}
	else if (action == GEDIT_UNDO_INSERT)
	{
		if (last_undo->end_pos != start_pos)
		{
			gedit_debug ("The text is not in the same position.", DEBUG_UNDO);
			last_undo->mergeable = FALSE;			
			return FALSE;
		}

		if ( text[0]!=' ' && text[0]!='\t' &&
		     (last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] ==' '
		      || last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] == '\t'))
		{
			gedit_debug ("The text is a space/tab but the previous char is not...", DEBUG_UNDO);
			last_undo->mergeable = FALSE;			
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


/**
 * gedit_undo_do:
 * @w: not used
 * @data: not used
 * 
 * Executes an undo request on the current document
 **/
void
gedit_undo_do (GtkWidget *w, gpointer data)
{
	Document *doc = gedit_document_current();
	GeditUndoInfo *undo;

	gedit_debug ("", DEBUG_UNDO);

	if (doc->undo == NULL)
	{
		gedit_debug ("There aren't any undo blocks. returning", DEBUG_UNDO);
		return;
	}
	
	g_return_if_fail (doc!=NULL);

	/* The undo data we need is always at the top op the
	   stack. So, therefore, the first one =) */
	undo = g_list_nth_data (doc->undo, 0);
	undo->mergeable = FALSE;

	doc->redo = g_list_prepend (doc->redo, undo);
	doc->undo = g_list_remove (doc->undo, undo);

	/* Check if there is a selection active */
	if (gedit_view_get_selection (gedit_view_current(), NULL, NULL))
		gedit_view_set_selection (gedit_view_current(), 0, 0);
	
	/* Move the view (scrollbars) to the correct position */
	gedit_view_set_window_position (gedit_view_current(), undo->window_position);
	
	if (undo->action == GEDIT_UNDO_DELETE)
		gedit_document_insert_text (doc, undo->text, undo->start_pos, FALSE);
	else if (undo->action == GEDIT_UNDO_INSERT)
		gedit_document_delete_text (doc, undo->start_pos, undo->end_pos-undo->start_pos, FALSE);
	else
		g_assert_not_reached ();

	/*doc->changed = undo->status; Disabled by chema. See below..*/
}

/**
 * gedit_undo_redo:
 * @w: not used
 * @data: not used
 * 
 * executes a redo request on the current document
 **/
void
gedit_undo_redo (GtkWidget *w, gpointer data)
{
	Document *doc = gedit_document_current();
	GeditUndoInfo *redo;

	gedit_debug ("", DEBUG_UNDO);

	if (doc->redo == NULL)
		return;
	
	if (doc==NULL)
		return;
	
	redo = g_list_nth_data (doc->redo, 0);
	doc->undo = g_list_prepend (doc->undo, redo);
	doc->redo = g_list_remove (doc->redo, redo);

	/* Check if there is a selection active */
	if (gedit_view_get_selection (gedit_view_current(), NULL, NULL))
		gedit_view_set_selection (gedit_view_current(), 0, 0);

	/* Move the view to the right position. */
	gedit_view_set_window_position (gedit_view_current(), redo->window_position);
				  
	if (redo->action == GEDIT_UNDO_INSERT)
		gedit_document_insert_text (doc, redo->text, redo->start_pos, FALSE);
	else if (redo->action == GEDIT_UNDO_DELETE)
		gedit_document_delete_text (doc, redo->start_pos, redo->end_pos-redo->start_pos, FALSE);
	else 
		g_assert_not_reached ();

	/*doc->changed = undo->status; disable by chema. We need to call the appropiate function
	  to actually change the title and stuff.*/
}


/**
 * gedit_undo_free_list:
 * @list_pointer: list to be freed
 *
 * frees and undo structure list
 **/
void
gedit_undo_free_list (GList ** list_pointer)
{
	gint n;
	GeditUndoInfo *nth_redo;
	GList *list = * list_pointer;
	
	gedit_debug ("", DEBUG_UNDO);

	if (list==NULL)
	{
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

	g_list_free (list);
	*list_pointer = NULL;
}

