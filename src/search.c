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
#include <string.h>

#include "document.h"
#include "view.h"
#include "utils.h"
#include "search.h"
#include "dialogs/dialogs.h"

typedef enum {
	SEARCH_IN_PROGRESS_NO,
	SEARCH_IN_PROGRESS_YES,
} GeditSearchState;

typedef struct _GeditSearchInfo GeditSearchInfo;

struct _GeditSearchInfo {
	GeditView *view;

	guchar *buffer;
	guint   length;

	GeditSearchState state;
};

GeditSearchInfo search_info;

/* --------- Cleaned stuff -------------------- */
/**
 * gedit_find_cb:
 * @widget: 
 * @data: 
 * 
 * Find callback
 **/
void
gedit_find_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	if (!gedit_document_current())
		return;

	gedit_dialog_replace (FALSE);
}

/**
 * gedit_find_again_cb:
 * @widget: 
 * @data: 
 * 
 * Find again callback
 **/
void
gedit_find_again_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	if (!gedit_document_current())
		return;

	gedit_find_again();
}

/**
 * gedit_replace_cb:
 * @widget: 
 * @data: 
 * 
 * Replace callbacl
 **/
void
gedit_replace_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	if (!gedit_document_current())
		return;
	
	gedit_dialog_replace (TRUE);
}


/**
 * gedit_goto_line_cb:
 * @widget: 
 * @data: 
 * 
 * Goto line callback
 **/
void
gedit_goto_line_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	if (!gedit_document_current())
		return;

	gedit_dialog_goto_line ();
}

/**
 * gedit_search_start:
 * @void: 
 * 
 * Init the search process
 **/
void
gedit_search_start (void)
{
	gedit_debug (DEBUG_RECENT, "");

	search_info.view = gedit_view_active();

	if (search_info.state == SEARCH_IN_PROGRESS_YES) {
		g_warning("This should not happen, gedit called start_search"
			  " and search in progress = YES \n");
		return;
	}
	
	search_info.buffer = gedit_document_get_buffer (search_info.view->doc);
	search_info.length = strlen (search_info.buffer);
	search_info.state  = SEARCH_IN_PROGRESS_YES;
}

/**
 * gedit_search_end:
 * @void: 
 * 
 * Finalize the search process
 **/
void
gedit_search_end (void)
{
	gedit_debug (DEBUG_RECENT, "");

	if (search_info.state == SEARCH_IN_PROGRESS_NO) {
		g_warning("This should not happen, gedit called end_search and search in progress = NO \n");
		search_info.state = SEARCH_IN_PROGRESS_NO;
		return;
	}

	g_free (search_info.buffer);
	search_info.state = SEARCH_IN_PROGRESS_NO;
}







































































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


gint
gedit_search_verify (void)
{
	GeditDocument *doc = gedit_document_current();

	/* If everything is "cool" return ..*/
	if (search_info.view->doc == doc)
		return TRUE;

	/* If there are no documents open, error */
	if (doc==NULL)
		return FALSE;

	/* Reload the doc */
	gedit_search_end();
	gedit_search_start();

	return TRUE;
	
}


static void
file_info ( gint pos,  gint *total_chars, gint *total_words, gint *total_lines,
	    gint *total_paragraphs, gint *line_number, gint *column_number )
{
	gint i=0 ;
	gint newlines_number ; /* this variable is the number of '\n' that there are
				  between two words */
	gint lines = 1 ;
	gint column = 0 ;
	
	gedit_debug (DEBUG_RECENT, "");

	if (search_info.state !=  SEARCH_IN_PROGRESS_YES)
		g_warning ("Search not started, watch out dude !\n");
	
	for (i = 0; i <= search_info.length; i++) 
	{
		if ( isalnum (search_info.buffer[i]) ||
		     search_info.buffer[i] == '\'' )
		{
			while (( isalnum ( search_info.buffer[i] ) ||
				 search_info.buffer[i] == '\'' ) &&
				i <= search_info.length )
			{				
				*total_chars = *total_chars + 1 ;
				i++;
			}
			*total_words = *total_words + 1;

			if ( i > search_info.length )
			{
				*total_paragraphs = *total_paragraphs + 1 ;
			}
			
		}

		if (!isalnum (search_info.buffer[i]) &&
		    i <= search_info.length  )
		{
			
			newlines_number = 0 ;
				
			while (!isalnum ( search_info.buffer[i] ) &&
			       i <= search_info.length )
			{
				if (search_info.buffer[i] == '\n')
				{
					newlines_number = newlines_number + 1 ;
				}
				i++;
			}
			
			*total_lines = *total_lines +  newlines_number ;

			if ((newlines_number > 1 && *total_words >0) ||
			    (i > search_info.length && newlines_number <= 1))
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
		if ( search_info.buffer[i]=='\n')
		{
			lines++;
			column = 0;
		}
		
	}

	
	return ;
		
}

void
gedit_file_info_cb (GtkWidget *widget, gpointer data)
{
	gint total_chars = 0 ;
	gint total_words = 0 ;
	gint total_lines = 1 ;
	gint total_paragraphs = 0 ;
	gint line_number = 0 ;
	gint column_number = 0 ;
	gchar *msg;
	gchar *doc_name;
	GeditDocument *doc;

	gedit_debug (DEBUG_RECENT, "");

	if (search_info.state != SEARCH_IN_PROGRESS_NO)
	{
		gedit_flash_va (_("Can't count lines if another search operation is active, please close the search dialog."));
		return;
	}
	    
	doc = gedit_document_current ();

	if (!doc)
		return;

	gedit_search_start();

	file_info ( gedit_view_get_position (search_info.view),  &total_chars , &total_words ,
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
			       total_lines , total_paragraphs , search_info.length, line_number , column_number );
	g_free (doc_name);
			
	gnome_dialog_run_and_close ((GnomeDialog *)
				    gnome_message_box_new (msg,
							   GNOME_MESSAGE_BOX_INFO,
							   GNOME_STOCK_BUTTON_OK,
							   NULL));
	g_free (msg);
	
}


static gint
pos_to_line (guint pos, gint *numlines)
{
	gint lines = 1;
	gint current_line = 0;
	gint i;

	gedit_debug (DEBUG_RECENT, "");

	*numlines = 0;
		
	if (search_info.state !=  SEARCH_IN_PROGRESS_YES) {
		g_warning ("Search not started, watch out dude !\n");
		return 0;
	}

	for (i = 0; i < search_info.length; i++) 
	{
		if (i == pos) 
			current_line = lines;
		if ( search_info.buffer[i]=='\n') 
			lines++;
	}

	if (i == pos) 
		current_line = lines;
	   
	*numlines = lines;
	
	return current_line;
}

gint
gedit_search_execute ( guint starting_position,
		       gint case_sensitive,
		       const guchar *text_to_search_for,
		       guint *pos_found,
		       gint  *line_found,
		       gint  *total_lines,
		       gboolean return_the_line_number)
{
	gint p1 = 0;
	guint p2 = 0;
	gint text_length;
	gint case_sensitive_mask;

	gedit_debug (DEBUG_RECENT, "");

#if 0
	/* FIXME: why do we get starting_position as a gulong ??? chema. It should
	   be guint */
	g_print ("Search text execute : start@ %i search for:%s buffer size :%i\n",
		 (gint) starting_position,
		 text_to_search_for,
		 (gint) search_info.length);
#endif

	g_return_val_if_fail (search_info.state ==  SEARCH_IN_PROGRESS_YES, FALSE);

	if (starting_position >= search_info.length)
		return FALSE;
	
	case_sensitive_mask = case_sensitive?0:32;
	text_length = strlen (text_to_search_for);
	for ( p2=starting_position; p2 < search_info.length; p2 ++)
	{
		if ((search_info.buffer[p2]|case_sensitive_mask)==(text_to_search_for[p1]|case_sensitive_mask))
		{
			p1++;
			if (p1==text_length)
				break;
		}
		else
			p1 = 0;
	}

	if (p2 == search_info.length)
		return FALSE;

	if (return_the_line_number)
		*line_found = pos_to_line ( p2, total_lines);

	*pos_found = p2 - text_length + 1;

	if (gedit_view_active() != search_info.view)
		g_warning("View is not the same F:%s L:%i", __FILE__, __LINE__);

	return TRUE;
}


guint
gedit_search_line_to_pos (gint line, gint *lines)
{
	gint current_line = 0, i;
	guint pos;
	
	gedit_debug (DEBUG_RECENT, "");

	pos = search_info.length;

	if (search_info.state !=  SEARCH_IN_PROGRESS_YES)
		g_warning ("Search not started, watch out dude !\n");

	for (i = 0; i < search_info.length; i++) 
	{
		if ( search_info.buffer[i]=='\n') 
			current_line++;
		if ( line == current_line + 2 )
			pos = i + 2 ;
	}

	if (line < 2)
		pos = 0;
	
	*lines = current_line ;
	
	return pos;
}




#define GEDIT_EXTRA_REPLACEMENTS_ 4
gint
gedit_replace_all_execute (GeditView *view, gint start_pos, const guchar *search_text,
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

	g_return_val_if_fail (search_info.state == SEARCH_IN_PROGRESS_YES, 0);
	g_return_val_if_fail (start_pos > -1, 0);

	search_text_length = strlen (search_text);
	replace_text_length = strlen (replace_text);

	buffer_in = search_info.buffer;
	buffer_in_length = search_info.length;

	if (buffer_in_length == 0)
	{
		/* Allocate something so that g_free doesn't crash */
		buffer_out = g_malloc (1);
		buffer_out [0] = '\0';
		return -1;
	}

	delta = replace_text_length - search_text_length;
	if (delta > 0)
		buffer_out_length = buffer_in_length + 100 + (grow_factor) * delta ;
	else
		buffer_out_length = buffer_in_length + 100;

	buffer_out = g_malloc (buffer_out_length);

	if (buffer_out==NULL)
	{
		g_warning ("Could not allocate the memory needed for search & replace\n");
		/* Allocate something, we __hope__ that 1 byte can be allocated */
		buffer_out = g_malloc (1);
		buffer_out [0] = '\0';
		return -1;
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
		if (p2 > buffer_out_length - (delta*2)) /* Allocate for at least 2 more replacements */
		{
			if (delta < 1) {
				g_warning ("This should not happen.\n");
				g_print ("Delta %i, Buffer_out_length:%i, p2:%i\n", delta, buffer_out_length, p2);
			}
			buffer_out_length = buffer_in_length + (replacements +
								(grow_factor <<= 1)) * delta;
			buffer_out = g_realloc (buffer_out, buffer_out_length);
			if (buffer_out == NULL)
			{
				g_warning ("Unable to realocate buffer");
				return -1;
			}
			
		}
		
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
		
		buffer_out [p2++] = buffer_in [p1++];
	}

	buffer_out [p2] = '\0';

	if (p2 >= buffer_out_length )
	{
		g_warning ("Error. Replace all has written outside of the memory requested.\n"
			   "P2:%i -  buffer_out_length:%i", p2, buffer_out_length);
	}

	*buffer = buffer_out;

	gedit_debug (DEBUG_RECENT, "end");
	
	return replacements;
}



