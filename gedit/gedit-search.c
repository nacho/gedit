/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit - Find/Replace/Find Line and stuff
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

#include "gedit.h"
#include "gedit-window.h"
#include "gE_view.h"
#include "gE_mdi.h"
#include "gedit-search.h"


/* pointers to the dialogs */
static GtkWidget *line_dialog;
static GtkWidget *search_dialog;
static GtkWidget *replace_dialog;

/* private functions */
static GtkWidget* create_line_dialog (void);
static GtkWidget* create_search_dialog (void);
static GtkWidget* create_replace_dialog (void);

static void add_search_options (GtkWidget *dialog);
static void line_dialog_button_cb    (GtkWidget   *widget,
                                      gint         button,
                                      gedit_document *doc);
static void search_dialog_button_cb  (GtkWidget   *widget,
                                      gint         button,
                                      gedit_document *doc);
static void replace_dialog_button_cb (GtkWidget   *widget,
                                      gint         button,
                                      gedit_document *doc);
static void get_search_options       (gedit_document *doc,
                                      GtkWidget   *widget,
                                      gchar      **txt,
                                      gulong      *options,
                                      gint        *pos);
static gboolean search               (GtkEditable *text,
                                      gchar       *str,
                                      gint         pos,
                                      gulong       options);
static void search_select            (gedit_document *doc,
                                      gchar       *str,
                                      gint         pos,
                                      gulong       options);
static gint ask_replace (void);
static gint num_widechars (const gchar *str);

/* 
 * find in files variable declarations  
 */
GtkWidget *find_in_files_dialog;

typedef struct _gedit_clist_data
{
	gchar *fname;
	gchar *contents;
	gint   line;
	gint		index;
  
} gedit_clist_data;

/* 
 * find in files function declerations
 */
static GtkWidget* create_find_in_files_dialog (void);
static void show_search_result_window (void);
void search_results_clist_insert (gchar *fname, 
				  gchar *contents,
				  gint 	 line, 
				  gint 	 index);
static void find_in_files_dialog_button_cb (GtkWidget *widget, 
					    gint       button, 
					    gpointer   data);
static void search_for_text_in_files (gchar *text);
static GtkWidget* create_find_in_files_dialog (void);

static gchar* get_line_as_text (gedit_view *view, gint pos);
static int find_in_file_search (gedit_view *view, gchar *str);
static void search_for_text_in_files (gchar *text);

static void find_in_files_dialog_button_cb (GtkWidget 	*widget, 
					    gint 		button, 
					    gpointer 	data);

static void show_search_result_window (void);
static void destroy_clist_data (gpointer data);



/*
 * Public interface. 
 */
gint
pos_to_line (gedit_document *doc, gint pos, gint *numlines)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	gulong lines = 0, i, current_line = 0;
	gchar *c;

	for (i = 0; i < gtk_text_get_length(GTK_TEXT(view->text)); i++) 
	{
		c = gtk_editable_get_chars(GTK_EDITABLE(view->text), i, i + 1);
		if (!strcmp(c, "\n")) 
			lines++;
		  
		if (i == pos) 
			current_line = lines;

		g_free (c);
	}
	   
	*numlines = lines;
	return current_line;
}

gint
line_to_pos (gedit_document *doc, gint line, gint *numlines)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	gulong lines = 1, i, current = 0;
	gchar *c;

	if (gtk_text_get_length(GTK_TEXT (view->text)) == 0) 
		return 0;
	
	for (i = 1; i < gtk_text_get_length(GTK_TEXT(view->text)) - 1; i++) 
	{
		c = gtk_editable_get_chars(GTK_EDITABLE(view->text), i - 1, i);
		if (!strcmp(c, "\n")) 
			lines++;
		g_free (c);
		
		if (lines == line) 
			current = i;
	}
	   
	*numlines = lines;
	return current;
}

gint
get_line_count (gedit_document *doc)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	gulong lines = 1, i;
	gchar *c;

	if (gtk_text_get_length(GTK_TEXT(view->text)) == 0) 
		return 0;
	  
	for (i = 1; i < gtk_text_get_length(GTK_TEXT(view->text)) - 1; i++) 
	{
		c = gtk_editable_get_chars(GTK_EDITABLE(view->text), i - 1, i);
		if (!strcmp(c, "\n")) 
			lines++;
		g_free (c);
	}
	   
	return lines;
}

void
seek_to_line (gedit_document *doc, gint line, gint numlines)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	gfloat value, ln, tl;

	if (numlines < 0) 
		numlines = get_line_count (doc);

	if (numlines < 3)
		return;
		
	if (line > numlines)
		return;
		
	ln = line;
	tl = numlines;
	value = (ln * GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->upper) /
		tl - GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->page_increment;
	gtk_adjustment_set_value(GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj),
				 value);
}

gint
gedit_search_search (gedit_document *doc, gchar *str, gint pos, gulong options)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	gint i, textlen;

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
		for (i = pos; i <= (textlen - num_widechars (str)); i++) 
		{
			if (search (GTK_EDITABLE (view->text), str, i, options)) 
				return i;
		}
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
gedit_search_replace (gedit_document *doc, gint pos, gint len, gchar *replace)
{
	gedit_view *view = GE_VIEW (mdi->active_view);

	gtk_editable_delete_text (GTK_EDITABLE (view->text),
				  pos, pos + len);
			
	gtk_editable_insert_text (GTK_EDITABLE (view->text),
				  replace, strlen (replace), &pos);
}

void
search_cb (GtkWidget *widget, gpointer data)
{
	gedit_document *doc;

	doc = gedit_document_current ();

	if (!search_dialog)
		search_dialog = create_search_dialog ();

	gtk_signal_connect (GTK_OBJECT (search_dialog), "clicked",
			    GTK_SIGNAL_FUNC (search_dialog_button_cb), doc);
	gtk_widget_show (search_dialog);
}

void
replace_cb (GtkWidget *widget, gpointer data)
{
	gedit_document *doc;

	doc = gedit_document_current ();

	if (!replace_dialog)
		replace_dialog = create_replace_dialog ();

	gtk_signal_connect (GTK_OBJECT (replace_dialog), "clicked",
			    GTK_SIGNAL_FUNC (replace_dialog_button_cb), doc);
	gtk_widget_show (replace_dialog);
}

void
goto_line_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *spin;
	GtkAdjustment *adj;
	gedit_document *doc;
	gint linecount;
	gedit_view *view = GE_VIEW (mdi->active_view);

	doc = gedit_document_current ();

	if (!line_dialog) 
		line_dialog = create_line_dialog ();

	spin = gtk_object_get_data (GTK_OBJECT (line_dialog), "line");

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin));
	adj->value = (gfloat) pos_to_line (doc,
					   gtk_editable_get_position (GTK_EDITABLE (view->text)),
					   &linecount);
	adj->upper = (gfloat) linecount;
	gtk_signal_connect (GTK_OBJECT (line_dialog), "clicked",
			    GTK_SIGNAL_FUNC (line_dialog_button_cb), doc);

	gtk_widget_show (line_dialog);
}

/* 
 *  All the find in files functions
 *
 */

/* 
 *  Callback sent from the menubar 
 */
void
find_in_files_cb (GtkWidget *widget, gpointer data)
{
	if (!find_in_files_dialog)
		find_in_files_dialog = create_find_in_files_dialog();

	gtk_signal_connect (GTK_OBJECT (find_in_files_dialog), "clicked",
			    GTK_SIGNAL_FUNC (find_in_files_dialog_button_cb), 
			    NULL);

	gtk_widget_show(find_in_files_dialog);
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
get_start_index_of_line (gedit_view *view,
			 gint pos)
{
	gchar *buffer;

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
get_line_as_text (gedit_view *view, 
		  gint pos)
{
	gchar *buffer;
	gint   start = pos;
	gint   end = pos;
	
	
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
find_in_file_search (gedit_view *view,
		     gchar   *str)
{
	gint i;
	gint textlen;
	gint counter = 0;
	gint line = 0;
	gchar *buffer;
	
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
	gedit_document *doc;
	gedit_view *view = NULL;
	gint i;
	gint j = 0;

	if (strlen(text) == 0)
	{
		search_results_clist_insert ("Total Matches", "", 0, -1);
		return;
	}	

        for (i = 0; i < g_list_length (mdi->children); i++) 
        {
		doc = GE_DOCUMENT ( g_list_nth_data (mdi->children, i) );
		view = GE_VIEW ( g_list_nth_data(doc->views, 0) );
		j += find_in_file_search (view, text);
        }

	
	search_results_clist_insert ("Total Matches", "", j, -1);
}


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
	gedit_view *view; 

	switch(button)
	{
		/* Search button */
	case 0:
		entry = gtk_object_get_data (GTK_OBJECT (widget), 
					     "find_in_files_text");
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
		view = GE_VIEW (mdi->active_view);
		gtk_window_set_focus (GTK_WINDOW(mdi->active_window),
				      view->text);
		break;
	}	  
}

/*
 * Show the search_result_window.
 */
static void
show_search_result_window (void)
{
	if (!GTK_WIDGET_VISIBLE (search_result_window))
		gtk_widget_show (search_result_window);
}

/*
 * Hide the search_result_window
 */
void 
remove_search_result_cb (GtkWidget 	*widget, 
			 gpointer 	data)
{
	gtk_button_set_relief (GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_widget_hide (search_result_window);
} 

void
search_results_clist_insert  (gchar 	*fname, 
			      gchar 	*contents,
			      gint 	line, 
			      gint 	index)
{
	gchar 			*insert[3];
	gedit_clist_data 	*data;
	
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
void
search_result_clist_cb (GtkWidget *list, gpointer func_data)
{
	gedit_view		*view;
	gedit_document		*doc;
	gedit_clist_data	*data;
	GList			*sel;
	gchar			*title;
	gint row = -1, i;

	sel = GTK_CLIST(list)->selection;
	
        if (!sel)
		return;
          
        row = (gint)sel->data;
	
        data = gtk_clist_get_row_data( GTK_CLIST(search_result_clist), row);
        
	if (data->index == -1)
		return;

	for (i = 0; i < g_list_length (mdi->children); i++) 
        {
		doc = GE_DOCUMENT ( g_list_nth_data (mdi->children, i) );
		view = GE_VIEW ( g_list_nth_data(doc->views, 0) );
            
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

void 
destroy_clist_data (gpointer data)
{
	g_free (data);
}


/* end find in files functions */


/*
 * PUBLIC: count_lines_cb
 *
 * public callback routine to count the total number of lines in the current
 * document and the current line number, and display the info in a dialog.
 */
void
count_lines_cb (GtkWidget *widget, gpointer data)
{
	gint total_lines, line_number;
	gchar *msg;
	gedit_document *doc;

	if ((doc = gedit_document_current ()))
	{
		line_number = pos_to_line (doc,
					   gtk_editable_get_position (GTK_EDITABLE (GE_VIEW (mdi->active_view)->text)),
					   &total_lines);
	
		msg = g_malloc0 (200);
		sprintf (msg, _("Filename: %s\n\nTotal Lines: %i\nCurrent Line: %i"), doc->filename, total_lines, line_number);
	
		gnome_dialog_run_and_close ((GnomeDialog *)
					    gnome_message_box_new (msg,
								   GNOME_MESSAGE_BOX_INFO,
								   GNOME_STOCK_BUTTON_OK,
								   NULL));
	}
}

/* private functions */

/* Creating the dialogs
 *
 * goto line dialog
 */

static GtkWidget *
create_line_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *hbox, *label, *spin;
	GtkObject *adj;

	dialog = gnome_dialog_new (_("Go to line"),
				   GNOME_STOCK_BUTTON_OK,
				   GNOME_STOCK_BUTTON_CANCEL,
				   NULL);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), hbox,
			    FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	label = gtk_label_new (_("Line number"));
	gtk_box_pack_start (GTK_BOX (hbox), label,
			    FALSE, FALSE, 0);
	gtk_widget_show (label);

	adj = gtk_adjustment_new (1, 1, 1, 1, 20, 20);
	spin = gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1, 0);
	gtk_box_pack_start (GTK_BOX (hbox), spin,
			    FALSE, FALSE, 0);
	gtk_widget_show (spin);

	gtk_object_set_data (GTK_OBJECT (dialog), "line", spin);
	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);

	return dialog;
}

static void
add_search_options (GtkWidget *dialog)
{
	GtkWidget *radio, *check;
	GSList *radiolist;

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
}

static GtkWidget *
create_search_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *frame, *entry;

	dialog = gnome_dialog_new (_("Search"),
				   _("Search"),
				   GNOME_STOCK_BUTTON_CLOSE,
				   NULL);

	frame = gtk_frame_new (_("Search for:"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), frame,
			    FALSE, FALSE, 0);
	gtk_widget_show (frame);

	entry = gnome_entry_new ("searchdialogsearch");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_widget_show (entry);
	entry = gnome_entry_gtk_entry (GNOME_ENTRY(entry));
	gtk_object_set_data (GTK_OBJECT (dialog), "searchtext", entry);
	add_search_options (dialog);
	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);
	
	return dialog;
}

static GtkWidget *
create_replace_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *frame, *entry, *check;

	dialog = gnome_dialog_new (_("Replace"),
				   _("Replace"),
				   _("Replace all"),
				   GNOME_STOCK_BUTTON_CLOSE,
				   NULL);

	frame = gtk_frame_new (_("Search for:"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), frame,
			    FALSE, FALSE, 0);
	gtk_widget_show (frame);
	entry = gnome_entry_new ("searchdialogsearch");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_widget_show (entry);
	entry = gnome_entry_gtk_entry (GNOME_ENTRY(entry));
	gtk_object_set_data (GTK_OBJECT (dialog), "searchtext", entry);

	frame = gtk_frame_new (_("Replace with:"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), frame,
			    FALSE, FALSE, 0);
	gtk_widget_show (frame);
	entry = gnome_entry_new ("searchdialogreplace");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_widget_show (entry);
	entry = gnome_entry_gtk_entry (GNOME_ENTRY(entry));
	gtk_object_set_data (GTK_OBJECT (dialog), "replacetext", entry);

	add_search_options (dialog);
	check = gtk_check_button_new_with_label (_("Ask before replacing"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), check,
			    FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "confirm", check);
	gtk_widget_show (check);

	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);

	return dialog;
}

/*
 * callback for the goto line dialog
 */
static void
line_dialog_button_cb (GtkWidget *widget, gint button, gedit_document *doc)
{
	GtkWidget *spin;
	gint line, linecount;
	gint pos;
	gedit_view *view = GE_VIEW (mdi->active_view);

	g_return_if_fail (doc != NULL);
	
	if (button == 0)
	{
		spin = gtk_object_get_data (GTK_OBJECT (widget), "line");
		line = gtk_spin_button_get_value_as_int
			(GTK_SPIN_BUTTON (spin));
		seek_to_line (doc, line, -1);
		pos = line_to_pos (doc, line, &linecount);
		gtk_editable_set_position (GTK_EDITABLE (view->text), pos);
	}
	gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				       GTK_SIGNAL_FUNC (line_dialog_button_cb),
				       doc);
	gnome_dialog_close (GNOME_DIALOG (widget));
}

/*
 * Search dialog
 */
static void
search_dialog_button_cb (GtkWidget *widget, gint button, gedit_document *doc)
{
	gint pos;
	gulong options = 0;
	gchar *str;

	if (button == 0)
	{
		get_search_options (doc, widget, &str, &options, &pos);
		pos = gedit_search_search (doc, str, pos, options);
		if (pos != -1)
			search_select (doc, str, pos, options);
	}
	else
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
					       GTK_SIGNAL_FUNC (search_dialog_button_cb),
					       doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
}

/*
 * Replace dialog
 */
static void
replace_dialog_button_cb (GtkWidget *widget, gint button, gedit_document *doc)
{
	GtkWidget *datalink;
	gint pos, dowhat = 0;
	gulong options;
	gchar *str, *replace;
	gboolean confirm = FALSE;

	options = 0;
	if (button < 2)
	{
		get_search_options (doc, widget, &str, &options, &pos);
		datalink = gtk_object_get_data (GTK_OBJECT (widget),
						"replacetext");
		replace = gtk_entry_get_text (GTK_ENTRY (datalink));
		datalink = gtk_object_get_data (GTK_OBJECT (widget),
						"confirm");
		if (GTK_TOGGLE_BUTTON (datalink)->active)
			confirm = TRUE;
		do {
			pos = gedit_search_search (doc, str, pos, options);
			if (pos == -1) break;
			if (confirm) {
				search_select (doc, str, pos, options);
				dowhat = ask_replace();
				if (dowhat == 2)  break;
				if (dowhat == 1) {
					if ( !(options | SEARCH_BACKWARDS)) {
						pos += num_widechars (str);
					}
					continue;
				}
			}

			gedit_search_replace (doc, pos, num_widechars (str),
					      replace);
			if ( !(options | SEARCH_BACKWARDS)) {
				pos += num_widechars (replace);
			}
		} while (button);
	}
	else
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
					       GTK_SIGNAL_FUNC (replace_dialog_button_cb),
					       doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
}
/* Utility functions
 *
 * the actual search
 */

static void
get_search_options (gedit_document *doc, GtkWidget *widget,
		    gchar **txt, gulong *options, gint *pos)
{
	GtkWidget *datalink;
	GtkRadioButton *pos_but;
	gint pos_type = 0;
	GSList *from;
	gedit_view *view = GE_VIEW (mdi->active_view);
	
	datalink = gtk_object_get_data (GTK_OBJECT (widget), "case");
	if (GTK_TOGGLE_BUTTON (datalink)->active)
		*options |= SEARCH_NOCASE;

	datalink = gtk_object_get_data (GTK_OBJECT (widget), "direction");
	if (GTK_TOGGLE_BUTTON (datalink)->active)
		*options |= SEARCH_BACKWARDS;

	datalink = gtk_object_get_data (GTK_OBJECT (widget), "searchtext");
	*txt = gtk_entry_get_text (GTK_ENTRY (datalink));

	from = gtk_object_get_data (GTK_OBJECT (widget), "searchfrom");
	while (from) {
		pos_but = from->data;
		if (GTK_TOGGLE_BUTTON(pos_but)->active) {
			break;
		}
		from = from->next;
		pos_type++;
	}
	switch (pos_type) {
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

	buffer = gtk_editable_get_chars (text, pos, pos + num_widechars (str));
	if (options & SEARCH_NOCASE)
		retval = (!g_strcasecmp (buffer, str));
	else
		retval = (!strcmp (buffer, str));

	g_free (buffer);

	return retval;
}

static void
search_select (gedit_document *doc, gchar *str, gint pos, gulong options)
{
	gint numwcs;
	gedit_view *view = GE_VIEW (mdi->active_view);
	gint line, numlines;
	
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

static gint
ask_replace (void)
{
	return gnome_dialog_run_and_close (
		GNOME_DIALOG (gnome_message_box_new (
			_("Replace?"), GNOME_MESSAGE_BOX_INFO,
			GNOME_STOCK_BUTTON_YES,
			GNOME_STOCK_BUTTON_NO,
			GNOME_STOCK_BUTTON_CANCEL,
			NULL)));
}

/* returns the number of wide characters contained in str */
static gint
num_widechars (const gchar *str)
{
	gint numwcs;
	GdkWChar *wcs;

	wcs = g_new (GdkWChar, strlen(str));
	numwcs = gdk_mbstowcs (wcs, str, strlen(str));
	g_free (wcs);

	return numwcs >= 0 ? numwcs : 0;
}
