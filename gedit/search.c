/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 2000 Chema Celorio
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gnome.h>
#include <ctype.h>       /* this is for file_info () */
#include "document.h"
#include "view.h"
#include "utils.h"
#include "search.h"
#include "string.h"
#include "dialogs/dialogs.h"

/*
   WHY do we need to make a copy of the buffer ?
   well, the problem lies in that if we manipulate the information
   using the current text widget, the search becomes VERY slow.

   This is very memory ineficient, but is either this or an unusuable
   search.

   NOTE :
   Here is how the search functions work :

   1. We have a function called search_start() that we call if
   seach_in_progress  = SEARCH_IN_PROGRESS_NO ;

   In this function we make a copy of the buffer into a gchar plain
   vanilla buffer. and we change
   search_in_progress =  SEARCH_IN_PROGRESS_YES.

   2. Whenever we finish the searching functions, we call search_end() to
   free the buffer.
   Search in progress = SEARCH_IN_PROGRESS_NO

*/

/* DO WE NEED TO SET SEARCH_IN_PROGESS UPON STARTUP ????
   or can we asume that it will contain FALSE ?? Chema */
typedef enum {
	SEARCH_IN_PROGRESS_NO,
	SEARCH_IN_PROGRESS_YES,
	SEARCH_IN_PROGRESS_RELOAD,
} gedit_search_states;

SearchInfo gedit_search_info;

gint
search_verify_document (void)
{
	Document *doc = gedit_document_current();

	/* If everything is "cool" return ..*/
	if (gedit_search_info.doc == doc)
		return TRUE;

	/* If there are no documents open, error */
	if (doc==NULL)
		return FALSE;

	/* Reload the doc */
	gedit_search_end();
	gedit_search_start();

	return TRUE;
	
}

void
gedit_search_start (void)
{
	gedit_debug (DEBUG_RECENT, "");

	gedit_search_info.view = gedit_view_active();
	gedit_search_info.doc = gedit_document_current();
	gedit_search_info.original_readonly_state = gedit_search_info.view->readonly;

	switch (gedit_search_info.state) {
	case SEARCH_IN_PROGRESS_NO:
		gedit_search_info.buffer_length = gedit_document_get_buffer_length (gedit_search_info.doc);
		gedit_search_info.buffer = gedit_document_get_buffer (gedit_search_info.doc);
		gedit_search_info.state = SEARCH_IN_PROGRESS_YES;
		gedit_search_info.replace_start = 0;
		gedit_search_info.replace_end = 0;
		break;
	case SEARCH_IN_PROGRESS_YES :
		g_warning("This should not happen, gedit called start_search and search in progress = YES \n");
		/* Do nothing */
		break;
	case SEARCH_IN_PROGRESS_RELOAD:
		g_warning("This should not happen, gedit called end_search and search in progress = NO \n");
		break;
	}
	
}

void
gedit_search_end (void)
{
	gedit_debug (DEBUG_RECENT, "");

	switch (gedit_search_info.state) {
	case SEARCH_IN_PROGRESS_NO:
		g_warning("This should not happen, gedit called end_search and search in progress = NO \n");
		g_free (gedit_search_info.buffer);
		gedit_search_info.state = SEARCH_IN_PROGRESS_NO;
		break;
	case SEARCH_IN_PROGRESS_YES :
		/* free the buffer */
		g_free (gedit_search_info.buffer);
		gedit_search_info.state = SEARCH_IN_PROGRESS_NO;
		break;
	case SEARCH_IN_PROGRESS_RELOAD:
		/* free the buffer */
		g_warning("This should not happen, gedit called end_search and search in progress = NO \n");
		gedit_search_info.state = SEARCH_IN_PROGRESS_NO;
		break;
	}
	
}

void
dump_search_state (void)
{
	g_print("SEARCH STATE IS : ");
	
	switch (gedit_search_info.state) {
	case SEARCH_IN_PROGRESS_NO:
		g_print("Search NOT in Progress \n");
		break;
	case SEARCH_IN_PROGRESS_YES :
		g_print("Search in Progress \n");
		break;
	case SEARCH_IN_PROGRESS_RELOAD:
		g_print("Search needs to reload buffer\n");
		break;
	}
		
}

void
file_info_cb (GtkWidget *widget, gpointer data)
{
	gint total_chars = 0 ;
	gint total_words = 0 ;
	gint total_lines = 1 ;
	gint total_paragraphs = 0 ;
	gint line_number = 0 ;
	gint column_number = 0 ;
	gchar *msg;
	gchar *doc_name;
	Document *doc;

	gedit_debug (DEBUG_RECENT, "");

	if (gedit_search_info.state != SEARCH_IN_PROGRESS_NO)
	{
		gedit_flash_va (_("Can't count lines if another search operation is active, please close the search dialog."));
		return;
	}
	    
	doc = gedit_document_current ();

	if (!doc)
		return;

	gedit_search_start();

	file_info ( gedit_view_get_position (gedit_search_info.view),  &total_chars , &total_words ,
		   &total_lines ,  &total_paragraphs , &line_number , &column_number ) ;

	gedit_search_end();

	doc_name = gedit_document_get_tab_name (doc);
	msg = g_strdup_printf (_("Filename: %s\n\n"
				 "Total Characters: %i\n"
				 "Total Words: %i\n"
				 "Total Lines: %i\n"
				 "Total Paragraphs: %i\n"
				 "Total Bytes: %i\n\n"
				 "Current Line: %i\n"
				 "Current Column: %i"),
			       doc_name, total_chars , total_words ,
			       total_lines , total_paragraphs , gedit_search_info.buffer_length, line_number , column_number );
	g_free (doc_name);
			
	gnome_dialog_run_and_close ((GnomeDialog *)
				    gnome_message_box_new (msg,
							   GNOME_MESSAGE_BOX_INFO,
							   GNOME_STOCK_BUTTON_OK,
							   NULL));
	g_free (msg);
	
}

void
search_text_not_found_notify (View * view)
{
	GtkWidget *gnome_dialog;
	gchar * msg;
	
	gedit_flash_va (_("Text not found"));

	if (gedit_view_get_selection (view, NULL, NULL))
		gedit_view_set_selection (view, 0, 0);

	msg = g_strdup (_("Text not found."));
	gnome_dialog = gnome_message_box_new (msg,
					      GNOME_MESSAGE_BOX_INFO,
					      GNOME_STOCK_BUTTON_OK,
					      NULL);
	gnome_dialog_set_parent (GNOME_DIALOG (gnome_dialog),
				 GTK_WINDOW (mdi->active_window));
	gnome_dialog_run_and_close (GNOME_DIALOG(gnome_dialog));

	g_free (msg);
}



static void
find_again_execute (void)
{
	guint start_pos, pos_found, line_found, total_lines;
	gint eureka;
	guchar *text;
	gint search_text_length;
	View *view;
	gint case_sensitive;

	view = gedit_view_active();
	if (view == NULL)
		return;

	text = gedit_search_info.last_text_searched;
	if (text == NULL)
	{
		gedit_flash_va (_("Can't find again. There is not a text string to search for."));
		return;
	}
	case_sensitive = gedit_search_info.last_text_searched_case_sensitive;

	start_pos = gedit_view_get_position (view);
	search_text_length = strlen (text);
	gedit_search_start();
	eureka = search_text_execute ( start_pos,
				       case_sensitive,
				       text,
				       &pos_found,
				       &line_found,
				       &total_lines,
				       TRUE);
	gedit_search_end();
	if (!eureka)
	{
		search_text_not_found_notify (view);
		return;
	}

	gedit_flash_va (_("Text found at line :%i"),line_found);
	gedit_view_set_window_position_from_lines (view, line_found, total_lines);
		
	gtk_text_set_point (GTK_TEXT(view->text), pos_found+1);
	gtk_text_insert (GTK_TEXT(view->text), NULL, NULL, NULL, " ", 1);
	gtk_text_backward_delete (GTK_TEXT(view->text), 1);
	gtk_editable_select_region (GTK_EDITABLE(view->text), pos_found+1, pos_found+1+search_text_length);
}

gint
search_text_execute ( gulong starting_position,
		      gint case_sensitive,
		      const guchar *text_to_search_for,
		      guint * pos_found,
		      gint * line_found,
		      gint * total_lines,
		      gint return_the_line_number)
{
	gint p1 = 0;
	gulong p2 = 0;
	gint text_length;
	gint case_sensitive_mask;

	gedit_debug (DEBUG_RECENT, "");

#if 0
	/* FIXME: why do we get starting_position as a gulong ??? chema. It should
	   be guint */
	g_print ("Search text execute : start@ %i search for:%s buffer size :%i\n",
		 (gint) starting_position,
		 text_to_search_for,
		 (gint) gedit_search_info.buffer_length);
#endif

	g_return_val_if_fail (gedit_search_info.state ==  SEARCH_IN_PROGRESS_YES, FALSE);

	if (starting_position >= gedit_search_info.buffer_length)
		return FALSE;
	
	case_sensitive_mask = case_sensitive?0:32;
	text_length = strlen (text_to_search_for);
	for ( p2=starting_position; p2 < gedit_search_info.buffer_length; p2 ++)
	{
		if ((gedit_search_info.buffer[p2]|case_sensitive_mask)==(text_to_search_for[p1]|case_sensitive_mask))
		{
			p1++;
			if (p1==text_length)
				break;
		}
		else
			p1 = 0;
	}

	if (p2 == gedit_search_info.buffer_length)
	{
#if 0
		g_print ("Search NOT FOUND\n");
#endif
		return FALSE;
	}

	if (return_the_line_number)
		*line_found = pos_to_line ( p2, total_lines);

	*pos_found = p2 - text_length;

	if (gedit_view_active() != gedit_search_info.view)
		g_warning("View is not the same !!!!!!!!!!!! search.c");

#if 0
	g_print ("Search found !\n");
#endif
	return TRUE;
}

gint
pos_to_line (gint pos, gint *numlines)
{
	gulong lines = 1, i, current_line = 0;

	gedit_debug (DEBUG_RECENT, "");

	if (gedit_search_info.state !=  SEARCH_IN_PROGRESS_YES)
		g_warning ("Search not started, watch out dude !\n");

	for (i = 0; i < gedit_search_info.buffer_length; i++) 
	{
		if (i == pos) 
			current_line = lines;
		if ( gedit_search_info.buffer[i]=='\n') 
			lines++;
	}

	if (i == pos) 
		current_line = lines;
	   
	*numlines = lines;
	return current_line;
}

guint
line_to_pos (gint line, gint *lines)
{
	gint current_line = 0, i;
	guint pos;
	
	gedit_debug (DEBUG_RECENT, "");

	pos = (gulong) gedit_search_info.buffer_length;

	if (gedit_search_info.state !=  SEARCH_IN_PROGRESS_YES)
		g_warning ("Search not started, watch out dude !\n");

	for (i = 0; i < gedit_search_info.buffer_length; i++) 
	{
		if ( gedit_search_info.buffer[i]=='\n') 
			current_line++;
		if ( line == current_line + 2 )
			pos = i + 2 ;
	}

	if (line < 2)
		pos = 0;
	
	*lines = current_line ;
	return pos;
}

void
file_info ( gint pos,  gint *total_chars, gint *total_words, gint *total_lines,
	    gint *total_paragraphs, gint *line_number, gint *column_number )
{
	gint i=0 ;
	gint newlines_number ; /* this variable is the number of '\n' that there are
				  between two words */
	gint lines = 1 ;
	gint column = 0 ;
	
	gedit_debug (DEBUG_RECENT, "");

	if (gedit_search_info.state !=  SEARCH_IN_PROGRESS_YES)
		g_warning ("Search not started, watch out dude !\n");
	
	for (i = 0; i <= gedit_search_info.buffer_length; i++) 
	{
		if ( isalnum (gedit_search_info.buffer[i]) ||
		     gedit_search_info.buffer[i] == '\'' )
		{
			while (( isalnum ( gedit_search_info.buffer[i] ) ||
				 gedit_search_info.buffer[i] == '\'' ) &&
				i <= gedit_search_info.buffer_length )
			{				
				*total_chars = *total_chars + 1 ;
				i++;
			}
			*total_words = *total_words + 1;

			if ( i > gedit_search_info.buffer_length )
			{
				*total_paragraphs = *total_paragraphs + 1 ;
			}
			
		}

		if (!isalnum (gedit_search_info.buffer[i]) &&
		    i <= gedit_search_info.buffer_length  )
		{
			
			newlines_number = 0 ;
				
			while (!isalnum ( gedit_search_info.buffer[i] ) &&
			       i <= gedit_search_info.buffer_length )
			{
				if (gedit_search_info.buffer[i] == '\n')
				{
					newlines_number = newlines_number + 1 ;
				}
				i++;
			}
			
			*total_lines = *total_lines +  newlines_number ;

			if ((newlines_number > 1 && *total_words >0) ||
			    (i > gedit_search_info.buffer_length && newlines_number <= 1))
			{
				*total_paragraphs = *total_paragraphs + 1 ;
			}

			

		}

	}

	if ( *total_words == 0 ) *total_paragraphs = 0 ;
	
	for (i = 0; i <= pos ; i++) 
	{
		column++;
		if (i == pos)
		{
			*line_number = lines;
			*column_number = column ;
		}
		if ( gedit_search_info.buffer[i]=='\n')
		{
			lines++;
			column = 0;
		}
		
	}

	
	return ;
		
}



#define GEDIT_EXTRA_REPLACEMENTS_ 4
#define GEDIT_REPLACE_ALL_SIZE_LIMIT 1000000
gint
gedit_search_replace_all_execute ( View *view, guint start_pos, const guchar *search_text,
				   const guchar *replace_text, gint case_sensitive,
				   guchar **buffer)
{
	guchar * buffer_in;
	guchar * buffer_out;
	guint buffer_in_length;
	guint buffer_out_length;
	gint search_text_length;
	gint replace_text_length;
	gint delta;
	gint replacements = 0;
	guint p1, p2, p3, p4;
	/* p1 = points to the index on buffer_in
	   p2 = pointer to the index on buffer_out
	   p3 = pointer to the index in search_text
	   p4 = pointer to the index in replace_text */
	gint case_sensitive_mask;
	gint grow_factor = GEDIT_EXTRA_REPLACEMENTS_;

	gedit_debug (DEBUG_RECENT, "");
	
	g_return_val_if_fail (gedit_search_info.state == SEARCH_IN_PROGRESS_YES, 0);

	search_text_length = strlen (search_text);
	replace_text_length = strlen (replace_text);

	buffer_in = gedit_search_info.buffer;
	buffer_in_length = gedit_search_info.buffer_length;

	if (buffer_in_length == 0)
	{
		/* Allocate something so that g_free doesn't crash */
		buffer_out = g_malloc (1);
		buffer_out [0] = '\0';
		return 0;
	}

	delta = replace_text_length - search_text_length;
	if (delta > 0)
		buffer_out_length = buffer_in_length + (grow_factor) * delta;
	else
		buffer_out_length = buffer_in_length;

	buffer_out = g_malloc (buffer_out_length);

	if (buffer_out==NULL)
	{
		g_warning ("Could not allocate the memory needed for search & replace\n");
		/* Allocate something, we __hope__ that 1 byte can be allocated */
		buffer_out = g_malloc (1);
		buffer_out [0] = '\0';
		return 0;
	}

	p1 = 0;
	p2 = 0;
	p3 = 0;
	p4 = 0;

	case_sensitive_mask = case_sensitive?0:32;

	/* Copy the start of buffer if start_pos = cursor position */
	while (p1 < start_pos)
	{
		buffer_out [p2++] = buffer_in [p1++];
	}

	/* Do the actual replace all */
	while (p1 < buffer_in_length)
	{
		if ((buffer_in [p1]|case_sensitive_mask) == (search_text [p3]|case_sensitive_mask))
		{
			p3++;
			if (p3 == search_text_length)
			{
				p2 = p2 - search_text_length + 1;
				for (p4 = 0; p4 < replace_text_length; p4++)
					buffer_out [p2++] = replace_text [p4];
				replacements++;
				p3=0;
				p1++;
				continue;
			}
		}
		else
		{
			p3=0;
		}
		
		if (p2 > buffer_out_length - (delta*2))
		{
			if (delta < 1) {
				g_warning ("This should not happen.\n");
				g_print ("Delta %i, Buffer_out_length:%i, p2:%i\n", delta, buffer_out_length, p2);
			}
			buffer_out_length = buffer_in_length + (replacements +
								(grow_factor <<= 1)) * delta;
			/* g_realloc dies when g_reallocating large buffers, limit the buffer
			   to x MB's, which is a pretty large buffer. This is ugly but a crash
			   is even uglier. Chema */
			if (buffer_out_length > GEDIT_REPLACE_ALL_SIZE_LIMIT) {
				return -1;
			}
			buffer_out = g_realloc (buffer_out, buffer_out_length);
			if (buffer_out == NULL)
			{
				g_warning ("Unable to realocate buffer");
				return 0;
			}
			
		}
		buffer_out [p2++] = buffer_in [p1++];
	}

	buffer_out [p2] = '\0';

	if (p2 > buffer_out_length )
	{
		g_warning ("Error ...\n");
		g_assert_not_reached();
	}

	*buffer = buffer_out;

	g_warning ("Return ...\n");
	
	return replacements;
}



/* ----- Callbacks -------- */
void
find_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");
	gedit_dialog_replace (FALSE);
}

void
find_again_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");
	gedit_find_again();
}

void
replace_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");
	gedit_dialog_replace (TRUE);
}


void
goto_line_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	if (!gedit_document_current())
		return;

	gedit_dialog_goto_line ();
}
