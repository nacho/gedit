/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-goto-line.c
 * This file is part of gedit
 *
 * Copyright (C) 2001 Paolo Maggi 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <string.h>

#include <glade/glade-xml.h>
#include <libgnome/libgnome.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-utils.h"
#include "gedit-dialogs.h"
#include "gedit-document.h"
#include "gedit-mdi-child.h"
#include "gedit-view.h"
#include "gedit-debug.h"

#define GEDIT_RESPONSE_FIND		101
#define GEDIT_RESPONSE_REPLACE		102
#define GEDIT_RESPONSE_REPLACE_ALL	103

typedef struct _GeditDialogReplace GeditDialogReplace;
typedef struct _GeditDialogFind GeditDialogFind;

struct _GeditDialogReplace {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *replace_hbox;
	GtkWidget *case_sensitive;
	GtkWidget *beginning;
	GtkWidget *cursor;
};

struct _GeditDialogFind {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *case_sensitive;
	GtkWidget *beginning;
	GtkWidget *cursor;
};

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);

static void dialog_find_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogFind *find_dialog);

static void find_dlg_find_button_pressed (GeditDialogFind * dialog);

static void replace_dlg_find_button_pressed (GeditDialogReplace * dialog);
static void replace_dlg_replace_button_pressed (GeditDialogReplace * dialog);
static void replace_dlg_replace_all_button_pressed (GeditDialogReplace * dialog);

static GeditDialogReplace *dialog_replace_get_dialog 	(void);
static GeditDialogFind    *dialog_find_get_dialog 	(void);

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_SEARCH, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_replace_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogReplace *replace_dialog)
{
	gedit_debug (DEBUG_SEARCH, "");

	switch (res_id) {
		case GEDIT_RESPONSE_FIND:
			replace_dlg_find_button_pressed (replace_dialog);
			break;
			
		case GEDIT_RESPONSE_REPLACE:
			replace_dlg_replace_button_pressed (replace_dialog);
			break;
			
		case GEDIT_RESPONSE_REPLACE_ALL:
			replace_dlg_replace_all_button_pressed (replace_dialog);
			break;
		default:
			gtk_widget_destroy (replace_dialog->dialog);
	}
}

static void
dialog_find_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogFind *find_dialog)
{
	gedit_debug (DEBUG_SEARCH, "");

	switch (res_id) {
		case GEDIT_RESPONSE_FIND:
			find_dlg_find_button_pressed (find_dialog);
			break;

		default:
			gtk_widget_destroy (find_dialog->dialog);
	}
}

static GeditDialogReplace *
dialog_replace_get_dialog (void)
{
	static GeditDialogReplace *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *button;
	
	gedit_debug (DEBUG_SEARCH, "");
	
	if (dialog != NULL)
	{
		gdk_window_show (dialog->dialog->window);
		gdk_window_raise (dialog->dialog->window);
		gtk_widget_grab_focus (dialog->dialog);

		return dialog;
	}

	gui = glade_xml_new ( GEDIT_GLADEDIR "replace.glade2",
			     "replace_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find replace.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogReplace, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Replace"),
						      window,						      
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	/* Add Replace All button */
	gtk_dialog_add_button (GTK_DIALOG (dialog->dialog), 
			       _("Replace _All"), GEDIT_RESPONSE_REPLACE_ALL);
	
	/* Add Replace button */
	button = gedit_button_new_with_stock_image (_("_Replace"), GTK_STOCK_FIND_AND_REPLACE);
	g_return_val_if_fail (button != NULL, NULL);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog->dialog), 
				      button, GEDIT_RESPONSE_REPLACE);

	/* Add Find button */
	gtk_dialog_add_button (GTK_DIALOG (dialog->dialog), 
			       GTK_STOCK_FIND, GEDIT_RESPONSE_FIND);

	content = glade_xml_get_widget (gui, "replace_dialog_content");
	dialog->search_entry       = glade_xml_get_widget (gui, "search_for_text_entry");
	dialog->replace_entry      = glade_xml_get_widget (gui, "replace_with_text_entry");
	dialog->replace_hbox       = glade_xml_get_widget (gui, "hbox_replace_with");
	dialog->case_sensitive     = glade_xml_get_widget (gui, "case_sensitive");
	dialog->beginning          = glade_xml_get_widget (gui, "beginning_radio_button");
	dialog->cursor		   = glade_xml_get_widget (gui, "cursor_radio_button");

	if (!content			||
	    !dialog->search_entry 	||
	    !dialog->replace_entry  	|| 
	    !dialog->replace_hbox 	||
	    !dialog->case_sensitive 	||
	    !dialog->cursor	 	||
	    !dialog->beginning 	) 
	{
		g_print
		    ("Could not find the required widgets inside replace.glade2.\n");
		return NULL;
	}

	gtk_widget_show (dialog->replace_hbox);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_FIND);

	g_signal_connect(G_OBJECT (dialog->dialog), "destroy",
			 G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect(G_OBJECT (dialog->dialog), "response",
			 G_CALLBACK (dialog_replace_response_handler), dialog);

	g_object_unref (G_OBJECT (gui));

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	
	return dialog;
}

static GeditDialogFind *
dialog_find_get_dialog (void)
{
	static GeditDialogFind *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *replace_hbox;
	
	gedit_debug (DEBUG_SEARCH, "");

	if (dialog != NULL)
	{
		gdk_window_show (dialog->dialog->window);
		gdk_window_raise (dialog->dialog->window);
		gtk_widget_grab_focus (dialog->dialog);
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "replace.glade2",
			     "replace_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find replace.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogFind, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Find"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      GTK_STOCK_FIND,
						      GEDIT_RESPONSE_FIND,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content = glade_xml_get_widget (gui, "replace_dialog_content");

	dialog->search_entry       = glade_xml_get_widget (gui, "search_for_text_entry");
	
	replace_hbox       	   = glade_xml_get_widget (gui, "hbox_replace_with");
	
	dialog->case_sensitive     = glade_xml_get_widget (gui, "case_sensitive");
	dialog->beginning          = glade_xml_get_widget (gui, "beginning_radio_button");
	dialog->cursor		   = glade_xml_get_widget (gui, "cursor_radio_button");

	if (!content			||
	    !dialog->search_entry 	||
	    !replace_hbox 		||
	    !dialog->case_sensitive 	||
            !dialog->cursor	 	||
	    !dialog->beginning 	) 
	{
		g_print
		    ("Could not find the required widgets inside replace.glade2.\n");
		return NULL;
	}
	
	gtk_widget_hide (replace_hbox);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_FIND);

	g_signal_connect(G_OBJECT (dialog->dialog), "destroy",
			 G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect(G_OBJECT (dialog->dialog), "response",
			 G_CALLBACK (dialog_find_response_handler), dialog);

	g_object_unref (G_OBJECT (gui));

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	
	return dialog;
}

void
gedit_dialog_find (void)
{
	GeditDialogFind *dialog;
	GeditMDIChild *active_child;
	GeditDocument *doc;
	gchar* last_searched_text;

	gedit_debug (DEBUG_SEARCH, "");

	dialog = dialog_find_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Cannot create the Find dialog");
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	last_searched_text = gedit_document_get_last_searched_text (doc);
	if (last_searched_text != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (dialog->search_entry), last_searched_text);
		g_free (last_searched_text);	
	}

	gtk_widget_grab_focus (dialog->search_entry);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->beginning), TRUE);
	
	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show (dialog->dialog);
}

void
gedit_dialog_replace (void)
{
	GeditDialogReplace *dialog;
	GeditMDIChild *active_child;
	GeditDocument *doc;
	gchar* last_searched_text;
	gchar* last_replace_text;

	gedit_debug (DEBUG_SEARCH, "");

	dialog = dialog_replace_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Cannot create the Replace dialog");
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	last_searched_text = gedit_document_get_last_searched_text (doc);
	if (last_searched_text != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (dialog->search_entry), last_searched_text);
		g_free (last_searched_text);	
	}

	last_replace_text = gedit_document_get_last_replace_text (doc);
	if (last_replace_text != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (dialog->replace_entry), last_replace_text);
		g_free (last_replace_text);	
	}

	gtk_widget_grab_focus (dialog->search_entry);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE, FALSE);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->beginning), TRUE);
	
	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show (dialog->dialog);
}

static void
find_dlg_find_button_pressed (GeditDialogFind *dialog)
{
	GeditMDIChild *active_child;
	GeditView* active_view;
	GeditDocument *doc;
	const gchar* search_string = NULL;
	gboolean from_cursor;
	gboolean case_sensitive;
	
	gedit_debug (DEBUG_SEARCH, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
		return;
	
	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view != NULL);
	
	doc = active_child->document;
	g_return_if_fail (doc != NULL);
			
	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));		
	g_return_if_fail (search_string != NULL);

	if (strlen (search_string) <= 0)
		return;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->beginning)))
		from_cursor = FALSE;
	else
		from_cursor = TRUE;

	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->case_sensitive)))
		case_sensitive = TRUE;
	else
		case_sensitive = FALSE;
			
	if (!gedit_document_find (doc, search_string, from_cursor, case_sensitive))
	{	
		GtkWidget *message_dlg;

		message_dlg = gtk_message_dialog_new (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				_("The text \"%s\" was not found."), search_string);
		
		gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);
	
		gtk_dialog_run (GTK_DIALOG (message_dlg));
  		gtk_widget_destroy (message_dlg);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->beginning), TRUE);		
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->cursor), TRUE);		
		gedit_view_scroll_to_cursor (active_view);
	}
}


static void
replace_dlg_find_button_pressed (GeditDialogReplace *dialog)
{
	/* This is basically the same as find_dlg_find_button_pressed */

	GeditMDIChild *active_child;
	GeditView* active_view;
	GeditDocument *doc;
	const gchar* search_string = NULL;
	gboolean from_cursor;
	gboolean case_sensitive;
	
	gedit_debug (DEBUG_SEARCH, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
		return;
	
	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view != NULL);
	
	doc = active_child->document;
	g_return_if_fail (doc != NULL);
			
	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));		
	g_return_if_fail (search_string != NULL);

	if (strlen (search_string) <= 0)
		return;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->beginning)))
		from_cursor = FALSE;
	else
		from_cursor = TRUE;

	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->case_sensitive)))
		case_sensitive = TRUE;
	else
		case_sensitive = FALSE;
			
	if (!gedit_document_find (doc, search_string, from_cursor, case_sensitive))
	{	
		GtkWidget *message_dlg;

		message_dlg = gtk_message_dialog_new (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				_("The text \"%s\" was not found."), search_string);
		
		gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

		gtk_dialog_run (GTK_DIALOG (message_dlg));
  		gtk_widget_destroy (message_dlg);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->beginning), TRUE);	
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, FALSE);
	
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->cursor), TRUE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, TRUE);
		
		gedit_view_scroll_to_cursor (active_view);
	}
}

static void
replace_dlg_replace_button_pressed (GeditDialogReplace *dialog)
{
	GeditMDIChild *active_child;
	GeditView* active_view;
	GeditDocument *doc;
	const gchar* search_string = NULL;
	const gchar* replace_string = NULL;
	gchar* selected_text = NULL;
	gboolean case_sensitive;

	gedit_debug (DEBUG_SEARCH, "");

	g_return_if_fail (dialog != NULL);
	
	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, FALSE);
		return;
	}

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view != NULL);
	
	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));
	replace_string = gtk_entry_get_text (GTK_ENTRY (dialog->replace_entry));

	g_return_if_fail (search_string);
	g_return_if_fail (replace_string);

	if (strlen (search_string) <= 0)
		return;
	
	if (strlen (replace_string) <= 0)
		return;

	selected_text = gedit_document_get_selected_text (doc, NULL, NULL);

	gedit_debug (DEBUG_SEARCH, "Sel text: %s", selected_text);
	gedit_debug (DEBUG_SEARCH, "Search string: %s", search_string);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->case_sensitive)))
		case_sensitive = TRUE;
	else
		case_sensitive = FALSE;

	if ((selected_text == NULL) ||
	    (case_sensitive && (strcmp (selected_text, search_string) != 0)) ||
	    (!case_sensitive && !g_uft8_caselessnmatch (selected_text, 
						       search_string, 
						       strlen (selected_text), 
						       strlen (search_string)) != 0))
	{
		gedit_debug (DEBUG_SEARCH, "selected_text (%s) != search_string (%s)", 
				selected_text, search_string);

		if (selected_text != NULL)
			g_free (selected_text);

		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE, FALSE);
		
		replace_dlg_find_button_pressed (dialog);

		return;
	}
				
	g_free (selected_text);
		
	gedit_debug (DEBUG_SEARCH, "Replace string: %s", replace_string);

	gedit_document_replace_selected_text (doc, replace_string);

	gedit_debug (DEBUG_SEARCH, "Replaced");

	/* go ahead and find the next one */
	if (!gedit_document_find (doc, search_string, TRUE, case_sensitive))
	{	
	
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->beginning), TRUE);	
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, FALSE);	
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->cursor), TRUE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, TRUE);
		
		gedit_view_scroll_to_cursor (active_view);
	}

	gedit_debug (DEBUG_SEARCH, "END");
}


static void
replace_dlg_replace_all_button_pressed (GeditDialogReplace *dialog)
{
	GeditMDIChild *active_child;
	GeditView* active_view;
	GeditDocument *doc;
	const gchar* search_string = NULL;
	const gchar* replace_string = NULL;
	gboolean case_sensitive;
	gint replaced_items;
	GtkWidget *message_dlg;
	
 	gedit_debug (DEBUG_SEARCH, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
		return;

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view != NULL);
	
	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));		
	replace_string = gtk_entry_get_text (GTK_ENTRY (dialog->replace_entry));		
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->case_sensitive)))
		case_sensitive = TRUE;
	else
		case_sensitive = FALSE;
	
	replaced_items = gedit_document_replace_all (doc, search_string, replace_string, case_sensitive);
		
	if (replaced_items <= 0)
	{
		message_dlg = gtk_message_dialog_new (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			_("The text \"%s\" was not found."), search_string);
	}
	else
	{	
		message_dlg = gtk_message_dialog_new (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			_("Found and replaced %d occurences."), replaced_items);
	}
	
	gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

	gtk_dialog_run (GTK_DIALOG (message_dlg));
  	gtk_widget_destroy (message_dlg);
}

