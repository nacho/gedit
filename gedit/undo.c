/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Cleaned 10-00 by Chema */
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

#include "window.h"
#include "undo.h"
#include "utils.h"
#include "document.h"
#include "view.h"
#include "prefs.h"

typedef struct _GeditUndoInfo  GeditUndoInfo;

struct _GeditUndoInfo
{
	GeditUndoAction action;
	gchar *text;
	gint start_pos;
	gint end_pos;
	gfloat window_position;
	gint mergeable;

	gint status;	/* the changed status of the doc. (disabled) */
};

/**
 * gedit_undo_check_size:
 * @doc: document to check
 * 
 * Checks that the size of doc->undo does not excede settings->undo_levels and
 * frees any undo level above sett->undo_level.
 *
 **/
static void
gedit_undo_check_size (GeditDocument *doc)
{
	gint n;
	GeditUndoInfo *nth_undo;
	
	gedit_debug (DEBUG_UNDO, "");

	if (settings->undo_levels < 1)
		return;
	
	/* No need to check for the redo list size since the undo
	   list gets freed on any call to gedit_undo_add */
	if (g_list_length (doc->undo) > settings->undo_levels && settings->undo_levels > 0)
	{
		gint start;
		gint end;

		start = g_list_length (doc->undo);
		end   = settings->undo_levels;
		for (n = start; n >= end;  n--)
		{
			nth_undo = g_list_nth_data (doc->undo, n - 1);
			/* We don't want to remove a G_U_ACTION_REPLACE_DELETE level,
			   since they are loaded in pairs with a G_U_A_R_INSERT */
			if (n == end) {
				if ((nth_undo->action == GEDIT_UNDO_ACTION_REPLACE_DELETE))
				continue;
			}
			doc->undo = g_list_remove (doc->undo, nth_undo);
			g_free (nth_undo->text);
			g_free (nth_undo);
		}
	}

}

/**
 * gedit_undo_merge:
 * @last_undo: 
 * @start_pos: 
 * @end_pos: 
 * @action: 
 * 
 * This function tries to merge the undo object at the top of
 * the stack with a new set of data. So when we undo for example
 * typing, we can undo the whole word and not each letter by itself
 * 
 * Return Value: TRUE is merge was sucessful, FALSE otherwise
 **/
static gint
gedit_undo_merge (GList *list, guint start_pos, guint end_pos, gint action, const guchar* text)
{
	guchar * temp_string;
	GeditUndoInfo * last_undo;
	
	gedit_debug (DEBUG_UNDO, "");
	/* This are the cases in which we will NOT merge :
	   1. if (last_undo->mergeable == FALSE)
	   [mergeable = FALSE when the size of the undo data was not 1.
	   or if the data was size = 1 but = '\n' or if the undo object
	   has been "undone" already ]
	   2. The size of text is not 1
	   3. If the new merging data is a '\n'
	   4. If the last char of the undo_last data is a space/tab
	   and the new char is not a space/tab ( so that we undo
	   words and not chars )
	   5. If the type (action) of undo is different from the last one
	Chema */

	if (list==NULL)
		return FALSE;
	
	last_undo = g_list_nth_data (list, 0);

	if (!last_undo->mergeable)
	{
		return FALSE;
	}

	if (end_pos-start_pos != 1)
	{
		last_undo->mergeable = FALSE;
		return FALSE;
	}

	if (text[0]=='\n')
		goto gedit_undo_do_not_merge;

	if (action != last_undo->action)
		goto gedit_undo_do_not_merge;

	if (action == GEDIT_UNDO_ACTION_REPLACE_INSERT || action == GEDIT_UNDO_ACTION_REPLACE_DELETE)
		goto gedit_undo_do_not_merge;

	if (action == GEDIT_UNDO_ACTION_DELETE)
	{
		if (last_undo->start_pos!=end_pos && last_undo->start_pos != start_pos)
			goto gedit_undo_do_not_merge;

		if (last_undo->start_pos == start_pos)
		{
			/* Deleted with the delete key */
			if ( text[0]!=' ' && text[0]!='\t' &&
			     (last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] ==' '
			      || last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] == '\t'))
				goto gedit_undo_do_not_merge;
			
			temp_string = g_strdup_printf ("%s%s", last_undo->text, text);
			g_free (last_undo->text);
			last_undo->end_pos += 1;
			last_undo->text = temp_string;
		}
		else
		{
			/* Deleted with the backspace key */
			if ( text[0]!=' ' && text[0]!='\t' &&
			     (last_undo->text [0] == ' '
			      || last_undo->text [0] == '\t'))
				goto gedit_undo_do_not_merge;

			temp_string = g_strdup_printf ("%s%s", text, last_undo->text);
			g_free (last_undo->text);
			last_undo->start_pos = start_pos;
			last_undo->text = temp_string;
		}
	}
	else if (action == GEDIT_UNDO_ACTION_INSERT)
	{
		if (last_undo->end_pos != start_pos)
			goto gedit_undo_do_not_merge;

		if ( text[0]!=' ' && text[0]!='\t' &&
		     (last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] ==' '
		      || last_undo->text [last_undo->end_pos-last_undo->start_pos - 1] == '\t'))
			goto gedit_undo_do_not_merge;

		temp_string = g_strdup_printf ("%s%s", last_undo->text, text);
		g_free (last_undo->text);
		last_undo->end_pos = end_pos;
		last_undo->text = temp_string;
	}
	else
		g_warning ("Unknown action [%i] inside undo merge encountered", action);

	return TRUE;

gedit_undo_do_not_merge:
	last_undo->mergeable = FALSE;		
	return FALSE;
}

/**
 * gedit_undo_add:
 * @text: 
 * @start_pos: 
 * @end_pos: 
 * @action: either GEDIT_UNDO_ACTION_INSERT or GEDIT_UNDO_ACTION_DELETE
 * @doc: 
 * @view: The view so that we save the scroll bar position.
 * 
 * Adds text to the undo stack. It also performs test to limit the number
 * of undo levels and deltes the redo list
 **/
void
gedit_undo_add (const gchar *text, gint start_pos, gint end_pos,
		GeditUndoAction action, GeditDocument *doc, GeditView *view)
{
	GeditUndoInfo *undo;

	gedit_debug (DEBUG_UNDO, "(%i)*%s*", strlen (text), text);

	g_return_if_fail (text != NULL);
	g_return_if_fail (end_pos >= start_pos);
	

	gedit_undo_free_list (&doc->redo);

	/* Set the redo sensitivity */
	gedit_document_set_undo (doc, GEDIT_UNDO_STATE_UNCHANGED, GEDIT_UNDO_STATE_FALSE);

	if (gedit_undo_merge (doc->undo, start_pos, end_pos, action, text))
		return;

	if (view==NULL)
		view = gedit_view_active ();
	
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

	gedit_document_set_undo (doc, GEDIT_UNDO_STATE_TRUE, GEDIT_UNDO_STATE_UNCHANGED);
}


/**
 * gedit_undo_undo:
 * @w: not used
 * @data: not used
 * 
 * Executes an undo request on the current document
 **/
void
gedit_undo_undo (GtkWidget *w, gpointer data)
{
	GeditDocument *doc = gedit_document_current();
	GeditUndoInfo *undo;

	gedit_debug (DEBUG_UNDO, "");

	if (doc->undo == NULL)
		return;
	
	g_return_if_fail (doc!=NULL);

	/* The undo data we need is always at the top op the
	   stack. So, therefore, the first one */
	undo = g_list_nth_data (doc->undo, 0);
	g_return_if_fail (undo!=NULL);
	undo->mergeable = FALSE;
	doc->redo = g_list_prepend (doc->redo, undo);
	doc->undo = g_list_remove (doc->undo, undo);

	/* Check if there is a selection active */
	if (gedit_view_get_selection (gedit_view_active(), NULL, NULL))
		gedit_view_set_selection (gedit_view_active(), 0, 0);
	
	/* Move the view (scrollbars) to the correct position */
	gedit_view_set_window_position (gedit_view_active(), undo->window_position);

	switch (undo->action){
	case GEDIT_UNDO_ACTION_DELETE:
		gedit_document_insert_text (doc, undo->text, undo->start_pos, FALSE);
		break;
	case GEDIT_UNDO_ACTION_INSERT:
		gedit_document_delete_text (doc, undo->start_pos, undo->end_pos-undo->start_pos, FALSE);
		break;
	case GEDIT_UNDO_ACTION_REPLACE_INSERT:
		gedit_document_delete_text (doc, undo->start_pos, undo->end_pos-undo->start_pos, FALSE);
		/* "pull" another data structure from the list */
		undo = g_list_nth_data (doc->undo, 0);
		g_return_if_fail (undo!=NULL);
		doc->redo = g_list_prepend (doc->redo, undo);
		doc->undo = g_list_remove (doc->undo, undo);
		g_return_if_fail (undo->action==GEDIT_UNDO_ACTION_REPLACE_DELETE);
		gedit_document_insert_text (doc, undo->text, undo->start_pos, FALSE);
		break;
	case GEDIT_UNDO_ACTION_REPLACE_DELETE:
		g_warning ("This should not happen. UNDO_REPLACE_DELETE");
		break;
	default:
		g_assert_not_reached ();
	}

	/* FIXME: There are some problems with this. Chema */
#if 0
	if (doc->changed != undo->status)
	{
		doc->changed = undo->status;
		gedit_document_set_title (doc);
	}
#endif
	
	gedit_document_set_undo (doc, GEDIT_UNDO_STATE_UNCHANGED, GEDIT_UNDO_STATE_TRUE);
	if (g_list_length (doc->undo) == 0)
		gedit_document_set_undo (doc, GEDIT_UNDO_STATE_FALSE, GEDIT_UNDO_STATE_UNCHANGED);
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
	GeditDocument *doc = gedit_document_current();
	GeditUndoInfo *redo;

	gedit_debug (DEBUG_UNDO, "");

	if (doc->redo == NULL)
		return;
	
	if (doc==NULL)
		return;
	
	redo = g_list_nth_data (doc->redo, 0);
	g_return_if_fail (redo!=NULL);
	doc->undo = g_list_prepend (doc->undo, redo);
	doc->redo = g_list_remove (doc->redo, redo);

	/* Check if there is a selection active */
	if (gedit_view_get_selection (gedit_view_active(), NULL, NULL))
		gedit_view_set_selection (gedit_view_active(), 0, 0);

	/* Move the view to the right position. */
	gedit_view_set_window_position (gedit_view_active(), redo->window_position);
				  
	switch (redo->action){
	case GEDIT_UNDO_ACTION_INSERT:
		gedit_document_insert_text (doc, redo->text, redo->start_pos, FALSE);
		break;
	case GEDIT_UNDO_ACTION_DELETE:
		gedit_document_delete_text (doc, redo->start_pos, redo->end_pos-redo->start_pos, FALSE);
		break;
	case GEDIT_UNDO_ACTION_REPLACE_DELETE:
		gedit_document_delete_text (doc, redo->start_pos, redo->end_pos-redo->start_pos, FALSE);
		/* "pull" another data structure from the list */
		redo = g_list_nth_data (doc->redo, 0);
		g_return_if_fail (redo!=NULL);
		doc->undo = g_list_prepend (doc->undo, redo);
		doc->redo = g_list_remove (doc->redo, redo);
		g_return_if_fail (redo->action==GEDIT_UNDO_ACTION_REPLACE_INSERT);
		gedit_document_insert_text (doc, redo->text, redo->start_pos, FALSE);
		break;
	case GEDIT_UNDO_ACTION_REPLACE_INSERT:
		g_warning ("This should not happen. Redo: UNDO_REPLACE_INSERT");
		break;
	default:
		g_assert_not_reached ();
	}

	/* There are some problems with this. Chema */
#if 0	
	if (doc->changed != redo->status)
	{
		doc->changed = redo->status;
		gedit_document_set_title (doc);
	}
#endif	

	gedit_document_set_undo (doc, GEDIT_UNDO_STATE_TRUE, GEDIT_UNDO_STATE_UNCHANGED);
	if (g_list_length (doc->redo) == 0)
		gedit_document_set_undo (doc, GEDIT_UNDO_STATE_UNCHANGED, GEDIT_UNDO_STATE_FALSE);
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
	
	gedit_debug (DEBUG_UNDO, "");

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

