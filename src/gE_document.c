/* gEdit
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

#include <stdio.h>
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#define PLUGIN_TEST 1
#if PLUGIN_TEST
#include "plugin.h"
#include "gE_plugin_api.h"
#endif
#include "menus.h"
#include "toolbar.h"


#ifndef WITHOUT_GNOME
GnomeUIInfo gedit_file_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New"),  NULL, file_new_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
          'N', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Open"),  NULL, file_open_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	  'O', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save"),  NULL, file_save_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	  'S', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save as"),  NULL, file_save_as_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS },
	{ GNOME_APP_UI_ITEM, N_("Print"),  NULL, file_print_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT },	
	{ GNOME_APP_UI_ITEM, N_("Close"),  NULL, file_close_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	  'W', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Quit"),  NULL, file_quit_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	  'Q', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_edit_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Cut"),  NULL, edit_cut_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
	  'X', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Copy"),  NULL, edit_copy_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
	  'C', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Paste"),  NULL, edit_paste_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
	  'V', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Select All"),  NULL, edit_selall_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK },
	GNOMEUIINFO_END
};	

GnomeUIInfo gedit_search_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Search"),  NULL, search_search_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_ITEM, N_("Search and Replace"),  NULL, search_replace_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Search Again"),  NULL, search_again_cmd_callback, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_tab_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Top"),     NULL, tab_top_cback, NULL, NULL },
	{ GNOME_APP_UI_ITEM, N_("Bottom"),  NULL, tab_bot_cback, NULL, NULL },
	{ GNOME_APP_UI_ITEM, N_("Left"),    NULL, tab_lef_cback, NULL, NULL },
	{ GNOME_APP_UI_ITEM, N_("Right"),   NULL, tab_rgt_cback, NULL, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle"),   NULL, tab_toggle_cback, NULL, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_toolbar_menu []= {
	{GNOME_APP_UI_ITEM, N_("Show Toolbar"), NULL, tb_on_cb, NULL, NULL },
	{GNOME_APP_UI_ITEM, N_("Hide Toolbar"), NULL, tb_off_cb, NULL, NULL },
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Pictures and Text"), NULL, tb_pic_text_cb, NULL, NULL },
	{GNOME_APP_UI_ITEM, N_("Pictures only"), NULL, tb_pic_only_cb, NULL, NULL },
	{GNOME_APP_UI_ITEM, N_("Text only"), NULL, tb_text_only_cb, NULL, NULL },
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Tooltips On"), NULL, tb_tooltips_on_cb, NULL, NULL },
	{GNOME_APP_UI_ITEM, N_("Tooltips Off"), NULL, tb_tooltips_off_cb, NULL, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_options_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Text Font..."),  NULL, prefs_callback, NULL, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle Autoindent"),  NULL, auto_indent_toggle_callback, NULL, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Statusbar"),  NULL, gE_window_toggle_statusbar, NULL, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Wordwrap"),  NULL, gE_document_toggle_wordwrap, NULL, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_SUBTREE, N_("Document Tabs"), NULL, &gedit_tab_menu },
	{ GNOME_APP_UI_SUBTREE, N_("Toolbar"), NULL, &gedit_toolbar_menu },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Save Settings"),  NULL, gE_save_settings },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_help_menu []= {
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}, 
	
	{GNOME_APP_UI_ITEM, N_("About..."), NULL, gE_about_box, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
	
	/*
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}, 
		
	{ GNOME_APP_UI_ITEM, N_("About"), NULL, gE_about_box, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT },
	{ GNOME_APP_UI_SEPARATOR },
	GNOMEUIINFO_HELP ("gedit"),
	GNOMEUIINFO_END */
};

#if PLUGIN_TEST
GnomeUIInfo gedit_plugins_menu []= {
  { GNOME_APP_UI_ITEM, N_("Diff"), NULL, start_diff, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
  { GNOME_APP_UI_ITEM, N_("CVS Diff"), NULL, start_cvsdiff, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
  { GNOME_APP_UI_ENDOFINFO}
};
#endif

GnomeUIInfo gedit_menu [] = {
	{ GNOME_APP_UI_SUBTREE, N_("File"), NULL, &gedit_file_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Edit"), NULL, &gedit_edit_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Search"), NULL, &gedit_search_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Options"), NULL, &gedit_options_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#if PLUGIN_TEST
	{ GNOME_APP_UI_SUBTREE, N_("Plugins"), NULL, &gedit_plugins_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#endif
	{ GNOME_APP_UI_SUBTREE, N_("Help"), NULL, &gedit_help_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	GNOMEUIINFO_END
};
#endif

gE_window *gE_window_new()
{
  gE_window *window;
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
  window->auto_indent = 1;
  window->show_tabs = 1;

#ifdef WITHOUT_GNOME
  window->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (window->window, "gedit window");
#else
  window->window = gnome_app_new ("gEdit", GEDIT_ID );
#endif
  gtk_signal_connect (GTK_OBJECT (window->window), "destroy",
  		      GTK_SIGNAL_FUNC(destroy_window),
		      &window->window);
  gtk_signal_connect (GTK_OBJECT (window->window), "delete_event",
     		      GTK_SIGNAL_FUNC(destroy_window),
		      &window->window);
     
  gtk_window_set_wmclass ( GTK_WINDOW ( window->window ), "gEdit", "gedit" );
#ifdef WITHOUT_GNOME
  gtk_window_set_title (GTK_WINDOW (window->window), GEDIT_ID);
#endif
  gtk_widget_set_usize(GTK_WIDGET(window->window), 595, 390);
  gtk_window_set_policy(GTK_WINDOW(window->window), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (window->window), 0);
      
  box1 = gtk_vbox_new (FALSE, 0);

#ifdef WITHOUT_GNOME
  gtk_container_add (GTK_CONTAINER (window->window), box1);
  get_main_menu(&window->menubar, &window->accel);
#ifdef GTK_HAVE_ACCEL_GROUP
  gtk_window_add_accel_group(GTK_WINDOW(window->window), window->accel);
#else
  gtk_window_add_accelerator_table(GTK_WINDOW(window->window), window->accel);
#endif
  gtk_box_pack_start(GTK_BOX(box1), window->menubar, FALSE, TRUE, 0);
  gtk_widget_show(window->menubar);
#else
  gnome_app_set_contents (GNOME_APP(window->window), box1);
  gnome_app_create_menus (GNOME_APP (window->window), gedit_menu);
#endif

  gE_create_toolbar(window);
  
#ifdef WITHOUT_GNOME
  gtk_box_pack_start(GTK_BOX(box1), window->toolbar, FALSE, TRUE, 0);
  gtk_widget_show(window->toolbar);
#endif

  gtk_widget_show (box1);

  gE_document_new(window);

  gtk_box_pack_start(GTK_BOX(box1), window->notebook, TRUE, TRUE, 0);

/* looks ugly.. */
/*
      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);
      gtk_widget_show (separator);
*/
      box2 = gtk_hbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (box2), 0);
      gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);

      window->statusbar = gtk_statusbar_new ();
      gtk_box_pack_start (GTK_BOX (box2), window->statusbar, TRUE, TRUE, 0);
      gtk_widget_show (window->statusbar);
      
      line_button = gtk_button_new_with_label ("Line");
      gtk_signal_connect (GTK_OBJECT (line_button), "clicked",
                                   GTK_SIGNAL_FUNC (search_goto_line_callback), NULL);
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


  return window;
}

void gE_window_toggle_statusbar (GtkWidget *w, gpointer data)
{
	/* if (GTK_WIDGET_VISIBLE(main_window->statusbox))*/
	if (main_window->show_status == 1)
	{
		gtk_widget_hide (main_window->statusbox);
		main_window->show_status = 0;
	}
	else
	{
		gtk_widget_show (main_window->statusbox);
		main_window->show_status = 1;
	}
}

void gE_window_new_with_file(gE_window *window, char *filename)
{

	window = gE_window_new();
	
	gE_file_open(gE_document_current(window), filename);
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
		/* gtk_notebook_popup_enable (GTK_NOTEBOOK(window->notebook)); */
	}

	document->tab_label = gtk_label_new (("Untitled"));
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
	                    GTK_SIGNAL_FUNC (gE_event_button_press), document->text);

	gtk_signal_connect_after (GTK_OBJECT(document->text), "key_press_event",
	                                                  GTK_SIGNAL_FUNC(auto_indent_callback), document->text);


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
	gtk_widget_show (vscrollbar);

	gtk_notebook_append_page (GTK_NOTEBOOK(window->notebook), table, document->tab_label);

	if (window->documents == NULL) {
		window->documents = g_list_alloc ();
	}

	g_list_append (window->documents, document);

	gtk_notebook_set_page (GTK_NOTEBOOK(window->notebook), 
	                       g_list_length(((GtkNotebook *)(window->notebook))->first_tab) -1);

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
	cur = gtk_notebook_current_page (GTK_NOTEBOOK(window->notebook)) +1;
	/*g_print("%d\n",cur);*/
	current_document = (g_list_nth(window->documents, cur))->data;
	if (current_document == NULL)
		g_print ("Current Document == NULL\n");
	return current_document;
}

void gE_document_toggle_wordwrap (GtkWidget *w, gpointer data)
{
	gE_document *doc;
	doc = gE_document_current (main_window);
	doc->word_wrap = !doc->word_wrap;
	gtk_text_set_word_wrap (GTK_TEXT (doc->text), doc->word_wrap);
}

		
void gE_show_version()
{
	g_print ("%s\n", GEDIT_ID);
}

#if PLUGIN_TEST
void start_diff( GtkWidget *widget, gpointer data )
{
  plugin_callback_struct callbacks;
  plugin *plug = plugin_new( "/usr/local/bin/diff-plugin" );

  callbacks.document.create = gE_plugin_create;
  callbacks.text.append = gE_plugin_append;
  callbacks.document.show = gE_plugin_show;
  callbacks.document.current = gE_plugin_current;
  callbacks.document.filename = gE_plugin_filename;
  
  plugin_register( plug, &callbacks, GPOINTER_TO_INT( main_window ) );
}

void start_cvsdiff( GtkWidget *widget, gpointer data )
{
  plugin_callback_struct callbacks;
  plugin *plug = plugin_new( "/usr/local/bin/cvsdiff-plugin" );

  callbacks.document.create = gE_plugin_create;
  callbacks.text.append = gE_plugin_append;
  callbacks.document.show = gE_plugin_show;
  callbacks.document.current = gE_plugin_current;
  callbacks.document.filename = gE_plugin_filename;

  plugin_register( plug, &callbacks, GPOINTER_TO_INT( main_window ) );
}
#endif
