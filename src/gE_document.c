/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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

#define PLUGIN_TEST 1
#include <stdio.h>
#include <string.h>
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "gE_document.h"
#define PLUGIN_TEST 1
#if PLUGIN_TEST
#include "plugin.h"
#include "gE_plugin_api.h"
#endif
#include "commands.h"
#include "menus.h"
#include "toolbar.h"

static void notebook_switch_page (GtkWidget *w, GtkNotebookPage *page,
	gint num, gE_window *window);
static void gE_msgbar_clear(gpointer data);
static void gE_msgbar_timeout_add(gE_window *window);
static void gE_destroy_window(GtkWidget *, GdkEvent *event, gE_data *data);

static char *lastmsg = NULL;
static gint msgbar_timeout_id;


extern GList *plugins;

gE_window *gE_window_new()
{
  gE_window *window;
  gE_data *data;
  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *line_button, *col_button;
 
  window = g_malloc(sizeof(gE_window));
  window->notebook = NULL;
  window->save_fileselector = NULL;
  window->open_fileselector = NULL;
  window->documents = NULL;
  window->search = g_malloc (sizeof(gE_search));
  window->search->window = NULL;
  window->auto_indent = TRUE;
  window->show_tabs = TRUE;
  window->tab_pos = GTK_POS_TOP;
  
  data = g_malloc0 (sizeof (gE_data));

#ifdef WITHOUT_GNOME
  window->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (window->window, "gedit window");
#else
  window->window = gnome_app_new ("gEdit", GEDIT_ID );
#endif
/*  gtk_signal_connect (GTK_OBJECT (window->window), "destroy",
  		      GTK_SIGNAL_FUNC(destroy_window),
		      data);
*/

  data->window = window;

  gtk_signal_connect (GTK_OBJECT (window->window), "delete_event",
     		      GTK_SIGNAL_FUNC(gE_destroy_window),
		      data);
     
  gtk_window_set_wmclass ( GTK_WINDOW ( window->window ), "gEdit", "gedit" );
  gtk_window_set_title (GTK_WINDOW (window->window), GEDIT_ID);
  gtk_widget_set_usize(GTK_WIDGET(window->window), 595, 390);
  gtk_window_set_policy(GTK_WINDOW(window->window), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (window->window), 0);
      
  box1 = gtk_vbox_new (FALSE, 0);

  
#ifdef WITHOUT_GNOME
  gtk_container_add (GTK_CONTAINER (window->window), box1);
#endif

  gE_menus_init (window, data);

#ifdef WITHOUT_GNOME
/*
#ifdef GTK_HAVE_ACCEL_GROUP
  gtk_window_add_accel_group(GTK_WINDOW(window->window), window->accel);
#else
  gtk_window_add_accelerator_table(GTK_WINDOW(window->window), window->accel);
#endif
*/
  gtk_box_pack_start(GTK_BOX(box1), window->menubar, FALSE, TRUE, 0);
  gtk_widget_show(window->menubar);
#else
  gnome_app_set_contents (GNOME_APP(window->window), box1);
#endif

  gE_create_toolbar(window, data);
  
#ifdef WITHOUT_GNOME
  gtk_box_pack_start(GTK_BOX(box1), window->toolbar, FALSE, TRUE, 0);
  gtk_widget_show(window->toolbar);
#endif

  gtk_widget_show (box1);

  gE_document_new(window);

  gtk_box_pack_start(GTK_BOX(box1), window->notebook, TRUE, TRUE, 0);

      box2 = gtk_hbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (box2), 0);
      gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);

      window->statusbar = gtk_label_new ("Welcome to gEdit");
      gE_msgbar_timeout_add(window);
      gtk_box_pack_start (GTK_BOX (box2), window->statusbar, TRUE, TRUE, 0);
      gtk_misc_set_alignment (GTK_MISC (window->statusbar), 0.0, 0.5);
      gtk_widget_show (window->statusbar);
      
      line_button = gtk_button_new_with_label ("Line");
      gtk_signal_connect (GTK_OBJECT (line_button), "clicked",
                                   GTK_SIGNAL_FUNC (search_goto_line_callback), window);
      GTK_WIDGET_UNSET_FLAGS (line_button, GTK_CAN_FOCUS);
      window->line_label = gtk_label_new ("1");
      col_button = gtk_label_new ("Column");
      window->col_label = gtk_label_new ("0");
      gtk_box_pack_start (GTK_BOX (box2), line_button, FALSE, FALSE, 1);
      gtk_box_pack_start (GTK_BOX (box2), window->line_label, FALSE, FALSE, 1);
      gtk_box_pack_start (GTK_BOX (box2), col_button, FALSE, FALSE, 1);
      gtk_box_pack_start (GTK_BOX (box2), window->col_label, FALSE, FALSE, 1);
      gtk_widget_show (line_button);
      gtk_widget_show (window->line_label);
      gtk_widget_set_usize (window->line_label, 40, 0);
      gtk_widget_show (col_button);
      gtk_widget_show (window->col_label);
      gtk_widget_set_usize (window->col_label, 40, 0);
      gtk_widget_show (box2);
      window->statusbox = box2;

/*  gtk_widget_show(window->menubar); 
*/
  gtk_widget_show (window->notebook);
  gtk_widget_show (window->window);

  g_list_foreach (plugins, (GFunc)add_plugins_to_window, window);
  
  window_list = g_list_append (window_list, (gpointer) window);
 
  return window;
}

void gE_window_toggle_statusbar (GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	if (window->show_status == TRUE)
	{
		gtk_widget_hide (window->statusbox);
		window->show_status = FALSE;
	}
	else
	{
		gtk_widget_show (window->statusbox);
		window->show_status = TRUE;
	}
}

gE_document *gE_document_new(gE_window *window)
{
	gE_document *document;
	GtkWidget *table, *hscrollbar, *vscrollbar;
	GtkStyle *style;

	document = g_malloc0(sizeof(gE_document));

	document->window = window;

	if (window->notebook == NULL) {
		window->notebook = gtk_notebook_new ();
		gtk_notebook_set_scrollable (GTK_NOTEBOOK(window->notebook), TRUE);
		gtk_signal_connect_after (GTK_OBJECT (window->notebook), "switch_page",
		                                     GTK_SIGNAL_FUNC (notebook_switch_page), window);
		GTK_WIDGET_UNSET_FLAGS (window->notebook, GTK_CAN_FOCUS);
		/* gtk_notebook_popup_enable (GTK_NOTEBOOK(window->notebook)); */
	}

	document->tab_label = gtk_label_new (("Untitled"));
	GTK_WIDGET_UNSET_FLAGS (document->tab_label, GTK_CAN_FOCUS);
	document->filename = NULL;
	document->word_wrap = 1;
	gtk_widget_show (document->tab_label);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
	gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
	gtk_widget_show (table);

	document->text = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT (document->text), TRUE);
	gtk_text_set_word_wrap (GTK_TEXT (document->text), TRUE);

	gtk_signal_connect_after (GTK_OBJECT (document->text), "button_press_event",
	                    GTK_SIGNAL_FUNC (gE_event_button_press), window);

	gtk_signal_connect_after (GTK_OBJECT(document->text), "key_press_event",
	                                                  GTK_SIGNAL_FUNC(auto_indent_callback), window);


	gtk_table_attach_defaults (GTK_TABLE (table), document->text, 0, 1, 0, 1);
	style = gtk_style_new ();
	/*style->bg[GTK_STATE_NORMAL] = style->white;
        document->text->style->font = "-adobe-helvetica-medium-r-normal--12-*-*-*-*-*-*-*";*/
	gtk_widget_set_style (GTK_WIDGET(document->text), style);
        /*style = gtk_style_attach (style, document->window);*/
	gtk_widget_set_rc_style(GTK_WIDGET(document->text));
	gtk_widget_ensure_style(GTK_WIDGET(document->text));


	document->changed = FALSE;
	document->changed_id = gtk_signal_connect (GTK_OBJECT(document->text), "changed", 
	                                                                  GTK_SIGNAL_FUNC(document_changed_callback), document);
	gtk_widget_show (document->text);
	gtk_text_set_point (GTK_TEXT (document->text), 0);
     
	hscrollbar = gtk_hscrollbar_new (GTK_TEXT (document->text)->hadj);
	gtk_table_attach (GTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
 		  	GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	/* gtk_widget_show (hscrollbar); */
   
	vscrollbar = gtk_vscrollbar_new (GTK_TEXT (document->text)->vadj);
	gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
     			GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
     	
     	GTK_WIDGET_UNSET_FLAGS (vscrollbar, GTK_CAN_FOCUS);
	gtk_widget_show (vscrollbar);

	gtk_notebook_append_page (GTK_NOTEBOOK(window->notebook), table, document->tab_label);

	window->documents = g_list_append (window->documents, document);

	gtk_notebook_set_page (GTK_NOTEBOOK(window->notebook), 
	                       g_list_length(GTK_NOTEBOOK (window->notebook)->children) - 1);

	gtk_widget_grab_focus (document->text);
	return document;
}

/* This is currently not used, but I'm leaving it here in case we find some bugs
   in how we're doing it now.... -Evan

void get_current_doc (gpointer doc, gpointer tab)
{
	gE_document *a;
	GtkWidget *b;
	a = doc;
	b = tab;
	if (a->tab_label == b)
		current_document = doc;
}
*/

gE_document *gE_document_current(gE_window *window)
{
	gint cur;
	gE_document *current_document;
	current_document = NULL;
	cur = gtk_notebook_current_page (GTK_NOTEBOOK(window->notebook));
	/*g_print("%d\n",cur);*/
	current_document = g_list_nth_data (window->documents, cur);
	if (current_document == NULL)
		g_print ("Current Document == NULL\n");
	return current_document;
}

void gE_document_toggle_wordwrap (GtkWidget *w, gpointer cbwindow)
{
	gE_document *doc;
	gE_window *window = (gE_window *)cbwindow;

	doc = gE_document_current (window);
	doc->word_wrap = !doc->word_wrap;
	gtk_text_set_word_wrap (GTK_TEXT (doc->text), doc->word_wrap);
}

static void
notebook_switch_page (GtkWidget *w, GtkNotebookPage *page,
	gint num, gE_window *window)
{
	gE_document *doc;
	gchar *title;

	if (window->documents == NULL)
		return;	
	if (GTK_WIDGET_REALIZED (window->window)) {
	doc = g_list_nth_data (window->documents, num);
	gtk_widget_grab_focus (doc->text);
	title = g_malloc0 (strlen (GEDIT_ID) + strlen (GTK_LABEL(doc->tab_label)->label) + 4);
	
	sprintf (title, "%s - %s", GEDIT_ID, GTK_LABEL (doc->tab_label)->label);
	gtk_window_set_title (GTK_WINDOW (window->window), title);
	g_free (title);
	}
}

static void
gE_destroy_window (GtkWidget *widget, GdkEvent *event, gE_data *data)
{
	file_close_window_cmd_callback (widget, data);
}


/*
 * PUBLIC: gE_messagebar_set
 *
 * sets the message/status bar.  remembers the last message set so that
 * a duplicate one won't be set on top of the current one.
 */
void
gE_msgbar_set(gE_window *window, char *msg)
{
	if (lastmsg == NULL || strcmp(lastmsg, msg)) {
		gtk_label_set(GTK_LABEL(window->statusbar), msg);
		if (lastmsg)
			g_free(lastmsg);
		lastmsg = g_strdup(msg);
		gtk_timeout_remove(msgbar_timeout_id);
		gE_msgbar_timeout_add(window);
	}
}


/*
 * PRIVATE: gE_msgbar_timeout_add
 *
 * automatically clears the text after 3 seconds
 */
static void
gE_msgbar_timeout_add(gE_window *window)
{
	msgbar_timeout_id =
		gtk_timeout_add(3000, (GtkFunction)gE_msgbar_clear, window);
}


/*
 * PRIVATE: gE_msgbar_clear
 *
 * clears the text by using a space (" ").  apparently, Gtk has no
 * provision for clearing a label widget.
 */
static void
gE_msgbar_clear(gpointer data)
{
	gE_window *window = (gE_window *)data;

	gE_msgbar_set(window, MSGBAR_CLEAR);
	gtk_timeout_remove(msgbar_timeout_id);
}

/* the end */
