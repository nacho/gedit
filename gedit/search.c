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

#include <gnome.h>
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
   Here is how, I think the search, replace and count lines should work :

   1. We have a function called search_start() that we call if
   seach_in_progress  = SEARCH_IN_PROGRESS_NO ;

   In this function we make a copy of the buffer into a gchar plain
   vanilla buffer. and we change
   search_in_progress =  SEARCH_IN_PROGRESS_YES.

   2. Whenever we finish the searching functions, we call search_end() to
   free the buffer.
   Search in progress = SEARCH_IN_PROGRESS_NO

   3. We need to track changes in the document. For example if a search dialog
   is open and the user modifies the document itself or moves the cursor. We
   can hook a function to view edit to change the state of search_in_progress
   to SEARCH_IN_PROGRESS_RELOAD_BUFFER

   4. We also need to have a search in progress state called
   SEARCH_IN_PROGRESS_REPLACE
*/

/* DO WE NEED TO SET SEARCH_IN_PROGESS UPON STARTUP ????
   or can we asume that it will contain FALSE ?? Chema */
typedef enum {
	SEARCH_IN_PROGRESS_NO,
	SEARCH_IN_PROGRESS_YES,
	SEARCH_IN_PROGRESS_RELOAD,
	SEARCH_IN_PROGRESS_REPLACE,
	SEARCH_IN_PROGRESS_COUNT_LINES,
} gedit_search_states;

typedef struct _SearchInfo {
	gint state;
	gint original_readonly_state;
	guchar * buffer;
	gulong buffer_length;
	View *view;
} SearchInfo;

SearchInfo gedit_search_info;

void
search_start (void)
{
	GtkText *text_buffer;

	gedit_debug("\n", DEBUG_SEARCH);
	
	text_buffer = GTK_TEXT ( VIEW (mdi->active_view)->text );

	gedit_search_info.view = VIEW (mdi->active_view);
	gedit_search_info.original_readonly_state = gedit_search_info.view->read_only;
	gedit_view_set_read_only (gedit_search_info.view, TRUE);

	switch (gedit_search_info.state) {
	case SEARCH_IN_PROGRESS_NO:
		gedit_search_info.buffer_length = gtk_text_get_length (text_buffer);
		gedit_search_info.buffer = gtk_editable_get_chars (
			GTK_EDITABLE ( text_buffer ),
			0,
			gedit_search_info.buffer_length
			);
		gedit_search_info.state = SEARCH_IN_PROGRESS_YES;
		break;
	case SEARCH_IN_PROGRESS_YES :
		g_warning("This should not happen, gedit called start_search and search in progress = YES \n");
		/* Do nothing */
		break;
	case SEARCH_IN_PROGRESS_RELOAD:
		/* free the buffer and reload it */
		/* FIXME */
		break;
	case SEARCH_IN_PROGRESS_REPLACE:
		/* Dunno what do I have to do ... */
		break;
	case SEARCH_IN_PROGRESS_COUNT_LINES:
		/* Dunno what do I have to do ... */
		break;
	}
	
}

void
search_end (void)
{
	gedit_debug("\n", DEBUG_SEARCH);

	gedit_view_set_read_only (gedit_search_info.view,
				  gedit_search_info.original_readonly_state);

	switch (gedit_search_info.state) {
	case SEARCH_IN_PROGRESS_NO:
		g_warning("This should not happen, gedit called end_search and search in progress = NO \n");
		/* free the buffer */
		break;
	case SEARCH_IN_PROGRESS_YES :
		/* free the buffer */
		g_free (gedit_search_info.buffer);
		gedit_search_info.state = SEARCH_IN_PROGRESS_NO;
		break;
	case SEARCH_IN_PROGRESS_RELOAD:
		/* free the buffer */
		gedit_search_info.state = SEARCH_IN_PROGRESS_NO;
		break;
	case SEARCH_IN_PROGRESS_REPLACE:
		/* Dunno what to do ... */
		break;
	case SEARCH_IN_PROGRESS_COUNT_LINES:
		/* Dunno what to do ... */
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
	case SEARCH_IN_PROGRESS_REPLACE:
		g_print("Replace in Progress \n");
		break;
	}
		
}

void
count_lines_cb (GtkWidget *widget, gpointer data)
{
	gint total_lines, line_number;
	gchar *msg;
	Document *doc;

	gedit_debug ("\n", DEBUG_SEARCH);

	if (gedit_search_info.state != SEARCH_IN_PROGRESS_NO)
	{
		gedit_flash_va (_("Can't count lines if another search operation is active, please close the search dialog."));
		return;
	}
	    
	doc = gedit_document_current ();

	if (!doc)
		return;
	
	search_start();
	line_number = pos_to_line (gtk_editable_get_position (GTK_EDITABLE (VIEW (mdi->active_view)->text)),
				   &total_lines);

	search_end();

	msg = g_strdup_printf (_("Filename: %s\n\nTotal Lines: %i\nCurrent Line: %i"),
			       doc->filename, total_lines, line_number);
			
	gnome_dialog_run_and_close ((GnomeDialog *)
				    gnome_message_box_new (msg,
							   GNOME_MESSAGE_BOX_INFO,
							   GNOME_STOCK_BUTTON_OK,
							   NULL));
	g_free (msg);
	
}

void
find_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug ("\n", DEBUG_SEARCH);
	dialog_find ();
}

void
replace_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug ("\n", DEBUG_SEARCH);
	g_print("I am a callback. But chema has not implemented my functionality. I need some love.....\n");
}


void
goto_line_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug ("\n", DEBUG_SEARCH);

	if (!gedit_document_current())
		return;

	dialog_find_line ();
}



gint
search_text_execute ( gulong starting_position,
		      gint case_sensitive,
		      guchar *text_to_search_for,
		      gulong * pos_found,
		      gint * line_found,
		      gint * total_lines,
		      gint return_the_line_number)
{
	gint p1 = 0;
	gulong p2 = 0;
	gint text_length;

	gedit_debug ("\n", DEBUG_SEARCH);

	if (gedit_search_info.state !=  SEARCH_IN_PROGRESS_YES)
		g_warning ("Search not started, watch out dude !\n");

	/* TODO, we need to check if the buffer is current. ( RELOAD needed )*/

	/*
	g_print("\n\nExecuting search ....\ntext: \n%s\n case sensitive :%i starting at: %lu\n\n",
		text_to_search_for,
		case_sensitive,
		starting_position);
	*/
	
	text_length = strlen (text_to_search_for);
	if (!case_sensitive)
	{
		for ( p2=starting_position; p2 < gedit_search_info.buffer_length; p2 ++)
		{
			if ((gedit_search_info.buffer[p2]|32)==(text_to_search_for[p1]|32))
			{
				p1++;
				if (p1==text_length)
					break;
			}
			else
				p1 = 0;
		}
	}
	else
	{
		for ( p2=starting_position; p2 < gedit_search_info.buffer_length; p2 ++)
		{
			if (gedit_search_info.buffer[p2]==text_to_search_for[p1])
			{
				p1++;
				if (p1==text_length)
					break;
			}
			else
				p1 = 0;
		}
	}
		
		

	if (p2 == gedit_search_info.buffer_length)
		return FALSE;

	if (return_the_line_number)
		*line_found = pos_to_line ( p2, total_lines);

	*pos_found = p2 - text_length;

	if (VIEW (mdi->active_view) != gedit_search_info.view)
		g_warning("View is not the same !!!!!!!!!!!!");

	return TRUE;
}

gint
pos_to_line (gint pos, gint *numlines)
{
	gulong lines = 1, i, current_line = 0;

	gedit_debug ("\n", DEBUG_SEARCH);

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

gulong
line_to_pos (Document *doc, gint line, gint *lines)
{
	gint current_line = 0, i;
	gulong pos;
	
	gedit_debug ("\n", DEBUG_SEARCH);

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
update_text (GtkText *text, gint line, gint lines)
{
	float adjustment;
	float upper;
	float page_increment;
	

	g_assert (lines >= line);

	upper = GTK_ADJUSTMENT(text->vadj)->upper;
	page_increment = GTK_ADJUSTMENT(text->vadj)->page_increment; 

	adjustment = ( line *  upper / lines - page_increment);

	gtk_adjustment_set_value (GTK_ADJUSTMENT(text->vadj), adjustment);
}



























































































































































































































#if 0

#include <config.h>
#include <glade/glade.h>

#include "view.h"
#include "document.h"
#include "utils.h"
#include "dialogs/dialogs.h"

/*
GtkWidget *search_result_window;
GtkWidget *search_result_clist;
*/
GtkWidget *search_dialog;

void edit_search_cb (GtkWidget *widget, gpointer data);

static GtkWidget* create_search_dialog (void);

/*
typedef struct _gedit_clist_data
{
	gchar *fname;
	gchar *contents;
	gint   line;
	gint   index;
} gedit_clist_data;
*/


/*
gint pos_to_line (Document *doc, gint pos, gint *numlines);
gint line_to_pos (Document *doc, gint line, gint *numlines);
gint get_line_count (Document *doc);
void seek_to_line (Document *doc, gint line, gint numlines);
gint gedit_search_search (Document *doc, gchar *str, gint pos, gulong options);
void gedit_search_replace (Document *doc, gint pos, gint len, gchar *replace);
void find_in_files_cb (GtkWidget *widget, gpointer data);
static GtkWidget* create_find_in_files_dialog (void);
static int get_start_index_of_line (View *view, gint pos);
static gchar* get_line_as_text (View *view, gint pos);
static int find_in_file_search (View *view, gchar *str);
static void search_for_text_in_files (gchar *text);
static void find_in_files_dialog_button_cb (GtkWidget *widget, gint button, gpointer data);
static void show_search_result_window (void);
void remove_search_result_cb (GtkWidget *widget, gpointer data);
void search_results_clist_insert (gchar *fname, gchar *contents, gint line, gint index);
void search_result_clist_cb (GtkWidget *list, gpointer func_data);
void destroy_clist_data (gpointer data);
void count_lines_cb (GtkWidget *widget, gpointer data);
void add_search_options (GtkWidget *dialog);
void get_search_options (Document *doc, GtkWidget *widget, gchar **txt, gulong *options, gint *pos);
static gboolean search (GtkEditable *text, gchar *str, gint pos, gulong options);
void search_select (Document *doc, gchar *str, gint pos, gulong options);
gint num_widechars (const gchar *str);
*/

void
search_cb (GtkWidget *widget, gpointer data)
{
	Document *doc;

	gedit_debug ("F:search_cb\n", DEBUG_SEARCH);

	doc = gedit_document_current ();

	if (!doc)
	     return;

	if (!search_dialog)
		search_dialog = create_search_dialog ();

/*
  gtk_signal_connect (GTK_OBJECT (search_dialog), "clicked",
			    GTK_SIGNAL_FUNC (search_dialog_button_cb), doc);
*/
	gtk_widget_show (search_dialog);
}


#if 0
static GtkWidget*
create_search_dialog (void)
{
	GladeXML *gui;
	GtkWidget *dialog;

/*
	gchar *username, *fullname, *hostname;
	gchar *from;
	gchar *filename_label;
*/

	gui = glade_xml_new (GEDIT_GLADEDIR "/search.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find search.glade");
		return;
	}

	dialog        = glade_xml_get_widget (gui, "dialog");
	to_entry      = glade_xml_get_widget (gui, "to_entry");
	from_entry    = glade_xml_get_widget (gui, "from_entry");
	subject_entry = glade_xml_get_widget (gui, "subject_entry");

	username = g_get_user_name ();
	fullname = g_get_real_name ();
	hostname = getenv ("HOSTNAME");

	if (gnome_config_get_string ("/Editor_Plugins/Email/From"))
	{
		gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gui, "from_entry")), 
				    gnome_config_get_string ("/Editor_Plugins/Email/From"));
	}
	else if (fullname && hostname)
	{
		from = g_strdup_printf ("%s <%s@%s>",
					fullname,
					username,
					hostname);

		gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gui, "from_entry")),
				    from);
		g_free (from);
	}

	/* Set the subject entry box */
	if (doc->filename)
		gtk_entry_set_text (GTK_ENTRY (subject_entry), doc->filename);
	else
		gtk_entry_set_text (GTK_ENTRY (subject_entry), _("Untitled"));

	/* Set the filename label */
	filename_label = g_strdup (GTK_LABEL (glade_xml_get_widget (gui, "filename_label"))->label);
	if (doc->filename)
		filename_label = g_strconcat (filename_label, doc->filename, NULL);
	else
		filename_label = g_strconcat (filename_label, _("Untitled"), NULL);
	gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (gui, "filename_label")),
			    filename_label);
	g_free (filename_label);
	
	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (email_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (email_finish), NULL);

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_destroy (GTK_OBJECT (gui));
}
#endif

/*
 * PUBLIC: count_lines_cb
 *
 * public callback routine to count the total number of lines in the current
 * document and the current line number, and display the info in a dialog.
 */
void
count_lines_cb (GtkWidget *widget, gpointer data)
{
}


#if 0
void
add_search_options (GtkWidget *dialog)
{
	GtkWidget *radio, *check;
	GSList *radiolist;

	gedit_debug ("F:add_search_options\n", DEBUG_SEARCH);

	if (!gedit_document_current())
		return;
	/* 
	radio = gtk_radio_button_new_with_label (NULL,
						 _("Search from cursor"));
	radiolist = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), radio,
			    FALSE, FALSE, 0);
	gtk_widget_show (radio);
	
	radio = gtk_radio_button_new_with_label (radiolist,
						 _("Search from beginning of document"));
	radiolist = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), radio,
			    FALSE, FALSE, 0);
	gtk_widget_show (radio);
	
	radio = gtk_radio_button_new_with_label (radiolist,
						 _("Search from end of document"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), radio,
			    FALSE, FALSE, 0);
	gtk_widget_show (radio);
	radiolist = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	gtk_object_set_data (GTK_OBJECT (dialog), "searchfrom", radiolist);
	
	check = gtk_check_button_new_with_label (_("Case sensitive"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), check,
			    FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "case", check);
	gtk_widget_show (check);
	
	check = gtk_check_button_new_with_label (_("Reverse search"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), check,
			    FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "direction", check);
	gtk_widget_show (check);
	*/
}
#endif




















#if 0


















/*
 *  Find in files dialog button callbacks
 */
static void
find_in_files_dialog_button_cb (GtkWidget *widget, 
				gint button, 
				gpointer data)
{
	GtkWidget *entry;
	gchar *entry_text;
	View *view; 

	gedit_debug ("F:find_in_files_dialog_button_cb.\n", DEBUG_SEARCH);
	
	switch(button)
	{
		/* Search button */
	case 0:
		entry = gtk_object_get_data (GTK_OBJECT (widget), "find_in_files_text");
		entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
		gtk_clist_clear (GTK_CLIST (search_result_clist));
		search_for_text_in_files (entry_text);
		show_search_result_window ();
		break;
		/* Canel Button */
	case 1:
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
					       GTK_SIGNAL_FUNC (find_in_files_dialog_button_cb),
					       NULL);
		gnome_dialog_close (GNOME_DIALOG (find_in_files_dialog));
		view = VIEW (mdi->active_view);
		gtk_window_set_focus (GTK_WINDOW(mdi->active_window), view->text);
		break;
	}	  
}



gint
line_to_pos (Document *doc, gint line, gint *numlines)
{
	View *view = VIEW (mdi->active_view);
	gulong lines = 1, i, current = 0;
	gchar *c;

	gedit_debug ("F:line_to_pos.\n", DEBUG_SEARCH);

	if (gtk_text_get_length (GTK_TEXT (view->text)) == 0) 
		return 0;
	
	for (i = 1; i < gtk_text_get_length (GTK_TEXT(view->text)) - 1; i++) 
	{
		c = gtk_editable_get_chars (GTK_EDITABLE(view->text), i - 1, i);
		if (!strcmp (c, "\n")) 
			lines++;
		g_free (c);
		
		if (lines == line) 
			current = i;
	}
	   
	*numlines = lines;
	return current;
}

gint
get_line_count (Document *doc)
{
	View *view = VIEW (mdi->active_view);
	gulong lines = 1, i;
	gchar *c;

	gedit_debug ("F:get_line_count.\n", DEBUG_SEARCH);
	
	if (gtk_text_get_length (GTK_TEXT(view->text)) == 0) 
		return 0;
	  
	for (i = 1; i < gtk_text_get_length (GTK_TEXT(view->text)) - 1; i++) 
	{
		c = gtk_editable_get_chars (GTK_EDITABLE(view->text), i - 1, i);
		if (!strcmp (c, "\n")) 
			lines++;
		g_free (c);
	}
	   
	return lines;
}

void
seek_to_line (Document *doc, gint line, gint numlines)
{
	View *view = VIEW (mdi->active_view);
/*	gfloat value, ln, tl; */

	int a, b;
	int len;
	char *buf;
	char *haystack;
	char *needle;
	int linenum = line;
	int numlines2 = 1;
	
	gedit_debug ("F:seek_to_line.\n", DEBUG_SEARCH);

	len = gtk_text_get_length (GTK_TEXT (view->text));
	buf = gtk_editable_get_chars (GTK_EDITABLE (view->text), 1, len);

	a = 1;
	b = len;
	haystack = buf;
	do {
		needle = strchr (haystack, '\n');
		if (needle) {
			haystack = needle + 1;
			if (linenum == numlines2)
				b = needle - buf + 1;
			numlines2++;
			if (linenum == numlines2)
				a = needle - buf + 1;
		}
	} while (needle != NULL);

	g_free (buf);
	gtk_text_set_point (GTK_TEXT (view->text), a + 1);
	gtk_editable_set_position (GTK_EDITABLE (view->text), a + 1);
	gtk_editable_select_region (GTK_EDITABLE (view->text), a, b);

	if (numlines < 0) 
		numlines = get_line_count (doc);

	if (numlines < 3)
		return;

	if (line > numlines)
		line = numlines;

/*
	ln = line;
	tl = numlines;
	value = (ln * GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->upper) /
		tl - GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->page_increment;
	gtk_adjustment_set_value (GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj),
				  value);
*/
}

gint
gedit_search_search (Document *doc, gchar *str, gint pos, gulong options)
{
	View *view = VIEW (mdi->active_view);
	gint i, textlen;

	gedit_debug ("F:gedit_search_search.\n", DEBUG_SEARCH);
	
	textlen = gtk_text_get_length(GTK_TEXT(view->text));
	if (textlen < num_widechars (str)) 
		return -1;

	if (options & SEARCH_BACKWARDS) 
	{
		pos -= num_widechars (str);
		for (i = pos; i >= 0; i--) 
		{
			if (search (GTK_EDITABLE (view->text), str, i, options))
				return i;
		}
	}
	else
	{
		gtk_text_freeze (GTK_TEXT (view->text));

		for (i = pos; i <= (textlen - num_widechars (str)); i++) 
		{
			if (search (GTK_EDITABLE (view->text), str, i, options)) 
				return i;
		}

		gtk_text_thaw (GTK_TEXT (view->text));
		
	}

	return -1;
}

/*
 * Replace the string searched earlier at position pos with
 * replace.
 *
 * This doesn NOT do any sanity checking
 */ 
void
gedit_search_replace (Document *doc, gint pos, gint len, gchar *replace)
{
	View *view = VIEW (mdi->active_view);

	gedit_debug ("F:gedit search_replace.\n", DEBUG_SEARCH);

	gtk_editable_delete_text (GTK_EDITABLE (view->text),
				  pos, pos + len);
			
	gtk_editable_insert_text (GTK_EDITABLE (view->text),
				  replace, strlen (replace), &pos);
}



/* 
 *  Create and return a dialog box for finding a text string in all open files.
 */
static GtkWidget*
create_find_in_files_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *frame;
	GtkWidget *entry;
	
	gedit_debug ("F:create_find_in_files_dialog.\n", DEBUG_SEARCH);

	dialog = gnome_dialog_new (_("Find In Files"),
				   _("Search"),
				   GNOME_STOCK_BUTTON_CLOSE,
				   NULL);

	frame = gtk_frame_new (_("Search for:"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), frame,
			    FALSE, FALSE, 0);
	gtk_widget_show (frame);
	
	entry = gnome_entry_new ("gedit_find_in_files");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_widget_show (entry);	
	
	entry = gnome_entry_gtk_entry (GNOME_ENTRY(entry));
	gtk_object_set_data (GTK_OBJECT (dialog), 
			     "find_in_files_text", 
			     entry);

	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);
	
	return dialog;
}

static int
get_start_index_of_line (View *view, gint pos)
{
	gchar *buffer;

	gedit_debug ("F:get_start_index_of_line.\n", DEBUG_SEARCH);
	
	g_return_val_if_fail (view != NULL, -1);
	
	while (pos > 0)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE(view->text), pos-1, pos);
		pos--;
		if (!strcmp (buffer, "\n"))
			return pos;
	}
	
	return pos;
}


/*
 * Return the line (as text) which view->text[pos] is located on.
 */
static gchar*
get_line_as_text (View *view, gint pos)
{
	gchar *buffer;
	gint   start = pos;
	gint   end = pos;
	
	gedit_debug ("F:get_lines_as_text.\n", DEBUG_SEARCH);
	
	while ( start > 0 )
	{
		buffer = gtk_editable_get_chars(GTK_EDITABLE(view->text), start-1, start);
		start--;
		if ( !strcmp(buffer, "\n") )
			break;
	}

	while (end < gtk_text_get_length(GTK_TEXT(view->text)))
	{
		buffer = gtk_editable_get_chars(GTK_EDITABLE(view->text), end , end+1);
		end++;
		if ( !strcmp(buffer, "\n") )
			break;
	}

	buffer = gtk_editable_get_chars(GTK_EDITABLE(view->text), start, end);
	
	return buffer;
	  
}

/*
 * Search for a text string in view->text. For each match append to the clist 
 * filename, linenumber, line text. Return total number of matches 
 */
static int
find_in_file_search (View *view, gchar *str)
{
	gint i;
	gint textlen;
	gint counter = 0;
	gint line = 0;
	gchar *buffer;
	
	gedit_debug ("F:find_in_file_search.\n", DEBUG_SEARCH);

	textlen = gtk_text_get_length(GTK_TEXT(view->text));

	if ( gtk_text_get_length(GTK_TEXT(view->text) ) == 0) 
		return counter;
	  
	if (textlen < num_widechars (str)) 
		return counter;

	for (i = 0; i <= ( textlen - num_widechars(str) ); i++) 
	{	
		/* keep a count of the line number */
		buffer = gtk_editable_get_chars(GTK_EDITABLE(view->text), i , i+1);
		if ( buffer != NULL && !strcmp(buffer, "\n") )
			line++;


		if ( search (GTK_EDITABLE(view->text), str, i, FALSE) ) 
		{

			search_results_clist_insert 
				(
					g_strdup_printf ("%s", GNOME_MDI_CHILD(view->document)->name), 
					get_line_as_text(view, i),
					line, 
					get_start_index_of_line (view, i)
					);
			counter++;
	    
		}	 
		g_free (buffer);
	  
	}
	return counter;	
}

/*
 * Search all files in mdi->children list for specifice text string. Append to end 
 * of the clist the total number matched found in all files.
 */
static void 
search_for_text_in_files (gchar *text)
{
	Document *doc;
	View *view = NULL;
	gint i;
	gint j = 0;

	gedit_debug ("F:search_for_text_in_files.\n", DEBUG_SEARCH);

	if (strlen(text) == 0)
	{
		search_results_clist_insert ("Total Matches", "", 0, -1);
		return;
	}	

        for (i = 0; i < g_list_length (mdi->children); i++) 
        {
		doc = DOCUMENT (g_list_nth_data (mdi->children, i));
		view = VIEW (g_list_nth_data(doc->views, 0));
		j += find_in_file_search (view, text);
        }

	
	search_results_clist_insert ("Total Matches", "", j, -1);
}



/*
 * Show the search_result_window.
 */
static void
show_search_result_window (void)
{
	gedit_debug ("F:show_search_result_window\n", DEBUG_SEARCH);
	
	if (!GTK_WIDGET_VISIBLE (search_result_window))
		gtk_widget_show (search_result_window);
}

/*
 * Hide the search_result_window
 */

/* DIS BY CHEMA
void 
remove_search_result_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug ("F:remove_search_result_cb.\n", DEBUG_SEARCH);

	gtk_button_set_relief (GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_widget_hide (search_result_window);
} 
*/

void
search_results_clist_insert (gchar *fname, 
			     gchar *contents,
			     gint   line, 
			     gint   index)
{
	gchar 			*insert[3];
	gedit_clist_data 	*data;

	gedit_debug ("F:search_results_clist_insert\n", DEBUG_SEARCH);

	data = g_malloc0 (sizeof (gedit_clist_data));
	
	data->line = line;
	data->index = index;
	data->fname = insert[0] = fname;
	insert[1] = g_strdup_printf ("%d", line); 
	data->contents = insert[2] = contents;
	
	gtk_clist_append( (GtkCList *)search_result_clist, insert);	   	

	line = GTK_CLIST (search_result_clist)->rows - 1;

	gtk_clist_set_row_data_full ( GTK_CLIST(search_result_clist), line, data, 
				      destroy_clist_data ); 
}	

/*
 * Callback when a user clicks on a item in the search_result_clist 
 */
/*
void
search_result_clist_cb (GtkWidget *list, gpointer func_data)
{
	View		*view;
	Document		*doc;
	gedit_clist_data	*data;
	GList			*sel;
	gchar			*title;
	gint row = -1, i;

	gedit_debug ("F:search_results_clist_cb\n", DEBUG_SEARCH);
	
	sel = GTK_CLIST(list)->selection;
	
        if (!sel)
		return;
          
        row = (gint)sel->data;
	
        data = gtk_clist_get_row_data( GTK_CLIST(search_result_clist), row);
        
	if (data->index == -1)
		return;

	for (i = 0; i < g_list_length (mdi->children); i++) 
        {
		doc = DOCUMENT (g_list_nth_data (mdi->children, i));
		view = VIEW (g_list_nth_data(doc->views, 0));
            
		title = g_strdup_printf ("%s", GNOME_MDI_CHILD(doc)->name);
	  
		if ( !strcmp(title, data->fname) ||
		     !strcmp(title, g_strdup_printf("*%s",data->fname) ) )
		{
			gnome_mdi_set_active_view (mdi, (GtkWidget *)view);	    
			search_select (doc, data->contents, data->index, FALSE);
			break;
		}
	}
}
*/

void 
destroy_clist_data (gpointer data)
{
	gedit_debug ("F:destroy_clist_data\n", DEBUG_SEARCH);

	g_free (data);
}



void
get_search_options (Document *doc, GtkWidget *widget,
		    gchar **txt, gulong *options, gint *pos)
{
	GtkWidget *datalink;
	GtkRadioButton *pos_but;
	gint pos_type = 0;
	GSList *from;
	View *view = VIEW (mdi->active_view);
	
	gedit_debug ("F:get_search_options\n", DEBUG_SEARCH); 

	datalink = gtk_object_get_data (GTK_OBJECT (widget), "case");
	if (GTK_TOGGLE_BUTTON (datalink)->active)
		*options |= SEARCH_NOCASE;

	datalink = gtk_object_get_data (GTK_OBJECT (widget), "direction");
	if (GTK_TOGGLE_BUTTON (datalink)->active)
		*options |= SEARCH_BACKWARDS;

	datalink = gtk_object_get_data (GTK_OBJECT (widget), "searchtext");
	*txt = gtk_entry_get_text (GTK_ENTRY (datalink));

	from = gtk_object_get_data (GTK_OBJECT (widget), "searchfrom");
	while (from)
	{
		pos_but = from->data;
		if (GTK_TOGGLE_BUTTON (pos_but)->active)
			break;
		from = from->next;
		pos_type++;
	}

	switch (pos_type)
	{
	case 0:
		*pos = gtk_text_get_length (GTK_TEXT (view->text));
		break;
	case 1:
		*pos = 0;
		break;
	case 2:
		*pos = gtk_editable_get_position (GTK_EDITABLE (view->text));
		break;
	}
}

static gboolean
search (GtkEditable *text, gchar *str, gint pos, gulong options)
{
	gchar *buffer;
	gint retval;

	gedit_debug ("search\n", DEBUG_SEARCH); 
	
	buffer = gtk_editable_get_chars (text, pos, pos + num_widechars (str));
	if (options & SEARCH_NOCASE)
		retval = (!g_strcasecmp (buffer, str));
	else
		retval = (!strcmp (buffer, str));

	g_free (buffer);

	return retval;
}

void
search_select (Document *doc, gchar *str, gint pos, gulong options)
{
	gint numwcs;
	View *view = VIEW (mdi->active_view);
	gint line, numlines;

	gedit_debug ("F:search_select\n", DEBUG_SEARCH); 
	
	numwcs = num_widechars (str);
	line = pos_to_line (doc, pos, &numlines);
	seek_to_line (doc, line, numlines);
	if (options | SEARCH_BACKWARDS)
	{
		gtk_editable_set_position (GTK_EDITABLE (view->text),
					   pos + numwcs);
		gtk_editable_select_region (GTK_EDITABLE (view->text),
					    pos , pos + numwcs);
	}
	else
	{
		gtk_editable_set_position (GTK_EDITABLE (view->text),
					   pos);
		gtk_editable_select_region (GTK_EDITABLE (view->text),
					    pos + numwcs, pos);
	}
}

/* returns the number of wide characters contained in str */
gint
num_widechars (const gchar *str)
{
	gint numwcs;
	GdkWChar *wcs;

	gedit_debug ("F:num_widechars\n", DEBUG_SEARCH); 
	
	wcs = g_new (GdkWChar, strlen(str));
	numwcs = gdk_mbstowcs (wcs, str, strlen(str));
	g_free (wcs);

	return numwcs >= 0 ? numwcs : 0;
}


#endif
#endif
