/* 	- Undo/Redo interface
 *
 * gEdit
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

#include <gnome.h>
#include <config.h>
#include "main.h"
#include "gE_undo.h"
#include "gE_mdi.h"
#include "gE_view.h"


void views_insert (gE_document *doc, gE_undo *undo);
void views_delete (gE_document *doc, gE_undo *undo);

void gE_undo_add (gchar *text, gint start_pos, gint end_pos, gint action, gE_document *doc)
{

	gE_undo *undo;
	
	undo = g_new(gE_undo, 1);
	
	undo->text = text;
	undo->start_pos = start_pos;
	undo->end_pos = end_pos;
	undo->action = action;
	undo->status = doc->changed;
	
	/* nuke the redo list, if its available */
	if (doc->redo) {
	
		g_list_free (doc->redo);
		doc->redo = NULL;
	
	}

#ifdef DEBUG
	g_message ("gE_undo_add: Adding to Undo list..");
#endif	
	doc->undo = g_list_prepend (doc->undo, undo);

}

void gE_undo_do (GtkWidget *w, gpointer data)
{

	gE_document *doc = gE_document_current();
	gE_undo *undo;
	
	if (!doc->undo)
	  return;
	
	/* The undo data we need is always at the top op the
	   stack. So, therefore, the first one =) */
	undo = g_list_nth_data (doc->undo, 0);

	/* add this undo data to the top of the redo stack.. */
	doc->redo = g_list_prepend (doc->redo, undo);
	
	/* remove the data from the undo stack */
	doc->undo = g_list_remove (doc->undo, undo);
	
	/* Now we can do the undo proper.. */
	
	if (undo->action) {
		
		/* We're inserting something that was deleted */
		
#ifdef DEBUG
		g_message ("gE_undo_do: Inserting..");
#endif
		
		if ((doc->buf->len > 0) && (undo->end_pos < doc->buf->len) && (undo->end_pos)) {
#ifdef DEBUG	
			g_message ("g_string_insert");
#endif
			doc->buf = g_string_insert (doc->buf, undo->start_pos, undo->text);
	
		} else if (undo->end_pos == 0) {
#ifdef DEBUG
			g_message ("g_string_prepend");
#endif
			doc->buf = g_string_prepend (doc->buf, undo->text);
	  
		} else {
#ifdef DEBUG	  
			g_message ("g_string_append");
#endif
			doc->buf = g_string_append (doc->buf, undo->text);
	
		}
			
		views_insert (doc, undo);

	} else {
	
		/* We're deleteing somthing that had been inserted */
		
		if (undo->end_pos + (undo->end_pos - undo->start_pos) <= doc->buf->len) {
#ifdef DEBUG
			g_message ("g_string_erase");
#endif
			doc->buf = g_string_erase (doc->buf, undo->start_pos, (undo->end_pos - undo->start_pos));
	  
		} else {
#ifdef DEBUG
			g_message ("g_string_truncate");
#endif
			doc->buf = g_string_truncate (doc->buf, undo->start_pos);
	
		}
		
		views_delete (doc, undo);
		doc->changed = undo->status;		
		
	}

}

void gE_undo_redo (GtkWidget *w, gpointer data)
{

	gE_document *doc = gE_document_current();
	gE_undo *redo;
	
	if (!doc->redo)
	  return;
	
	/* The redo data we need is always at the top op the
	   stack. So, therefore, the first one =) */
	redo = g_list_nth_data (doc->redo, 0);

	/* add this redo data to the top of the undo stack.. */
	doc->undo = g_list_prepend (doc->undo, redo);
	
	/* remove the data from the redo stack */
	doc->redo = g_list_remove (doc->redo, redo);
	
	/* Now we can do the undo proper.. */
	
	if (!redo->action) {
		
		/* We're inserting something that was deleted */
#ifdef DEBUG		
		g_message ("gE_undo_redo: Deleting..");
#endif		
		if ((doc->buf->len > 0) && (redo->end_pos < doc->buf->len) && (redo->end_pos)) {
#ifdef DEBUG
			g_message ("g_string_insert");
#endif
			doc->buf = g_string_insert (doc->buf, redo->start_pos, redo->text);
	
		} else if (redo->end_pos == 0) {
#ifdef DEBUG
			g_message ("g_string_prepend");
#endif
			doc->buf = g_string_prepend (doc->buf, redo->text);
	  
		} else {
#ifdef DEBUG
			g_message ("g_string_append");
#endif
			doc->buf = g_string_append (doc->buf, redo->text);
	
		}
			
		views_insert (doc, redo);

	} else {
	
		/* We're deleteing somthing that had been inserted */
		
		if (redo->end_pos + (redo->end_pos - redo->start_pos) <= doc->buf->len) {
#ifdef DEBUG
			g_message ("g_string_erase");
#endif
			doc->buf = g_string_erase (doc->buf, redo->start_pos, (redo->end_pos - redo->start_pos));
	  
		} else {
#ifdef DEBUG
			g_message ("g_string_truncate");
#endif
			doc->buf = g_string_truncate (doc->buf, redo->start_pos);
	
		}
		
		views_delete (doc, redo);
		
	}


}

void views_insert (gE_document *doc, gE_undo *undo)
{

	gint i;
	gint p1, p2;
	gE_view *view;
	
	for (i = 0; i < g_list_length (doc->views); i++) {

	  view = g_list_nth_data (doc->views, i);
	  
	  gtk_text_freeze (GTK_TEXT (view->text));
		
	  p1 = gtk_text_get_point (GTK_TEXT (view->text));
		
	  gtk_text_set_point (GTK_TEXT(view->text), undo->start_pos);

	  gtk_text_insert (GTK_TEXT (view->text), NULL,
					 &view->text->style->black,
					 NULL, undo->text, strlen(undo->text));
	  gtk_text_set_point (GTK_TEXT (view->text), p1);
		
	  gtk_text_thaw (GTK_TEXT (view->text));
		
		
	  gtk_text_freeze (GTK_TEXT (view->split_screen));
	  p1 = gtk_text_get_point (GTK_TEXT (view->split_screen));
	  gtk_text_set_point (GTK_TEXT(view->split_screen), undo->start_pos);

	  gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
					 &view->split_screen->style->black,
					 NULL, undo->text, strlen(undo->text));
	  gtk_text_set_point (GTK_TEXT (view->text), p1);
	  gtk_text_thaw (GTK_TEXT (view->split_screen));
	
	}  
}

void views_delete (gE_document *doc, gE_undo *undo)
{

	gE_view *nth_view;
	gint n;
	gint p1;
	gint start_pos, end_pos;
	
	start_pos = undo->start_pos;
	end_pos = undo->end_pos;
#ifdef DEBUG
	g_message ("views_delete: start_pos %d, end_pos %d, text %s", start_pos, end_pos, undo->text);
#endif
	for (n = 0; n < g_list_length (doc->views); n++) {

	  nth_view = g_list_nth_data (doc->views, n);
		
	  gtk_text_freeze (GTK_TEXT (nth_view->text));

	  p1 = gtk_text_get_point (GTK_TEXT (nth_view->text));

	  gtk_text_set_point (GTK_TEXT(nth_view->text), end_pos);
	  gtk_text_backward_delete (GTK_TEXT (nth_view->text), (end_pos - start_pos));
	  gtk_text_set_point (GTK_TEXT (nth_view->text), p1);

	  gtk_text_thaw (GTK_TEXT (nth_view->text));
		
	  gtk_text_freeze (GTK_TEXT (nth_view->split_screen));

	  p1 = gtk_text_get_point (GTK_TEXT (nth_view->split_screen));
	  gtk_text_set_point (GTK_TEXT(nth_view->split_screen), start_pos);
	  gtk_text_forward_delete (GTK_TEXT (nth_view->split_screen), (end_pos - start_pos));
	  gtk_text_set_point (GTK_TEXT (nth_view->split_screen), p1);

	  gtk_text_thaw (GTK_TEXT (nth_view->split_screen));

	}


}