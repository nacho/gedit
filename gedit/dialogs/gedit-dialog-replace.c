/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-replace.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glade/glade-xml.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-entry.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-utils.h"
#include "gedit-dialogs.h"
#include "gedit-document.h"
#include "gedit-mdi-child.h"
#include "gedit-view.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-menus.h"

#define GEDIT_RESPONSE_FIND		101
#define GEDIT_RESPONSE_REPLACE		102
#define GEDIT_RESPONSE_REPLACE_ALL	103

typedef struct _GeditDialogReplace GeditDialogReplace;
typedef struct _GeditDialogFind GeditDialogFind;

struct _GeditDialogReplace {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *search_entry_list;
	GtkWidget *replace_entry_list;
	GtkWidget *match_case_checkbutton;
	GtkWidget *entire_word_checkbutton;
	GtkWidget *wrap_around_checkbutton;
	GtkWidget *search_backwards_checkbutton;
};

struct _GeditDialogFind {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *search_entry_list;
	GtkWidget *case_sensitive;
	GtkWidget *match_case_checkbutton;
	GtkWidget *entire_word_checkbutton;
	GtkWidget *wrap_around_checkbutton;
	GtkWidget *search_backwards_checkbutton;
};

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);

static void dialog_find_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogFind *find_dialog);

static void find_dlg_find_button_pressed (GeditDialogFind * dialog);

static void replace_dlg_find_button_pressed (GeditDialogReplace * dialog);
static void replace_dlg_replace_button_pressed (GeditDialogReplace * dialog);
static void replace_dlg_replace_all_button_pressed (GeditDialogReplace * dialog);

static void insert_text_handler (GtkEditable *editable, const gchar *text, gint length, gint *position);

static GeditDialogReplace *dialog_replace_get_dialog 	(void);
static GeditDialogFind    *dialog_find_get_dialog 	(void);

static GQuark was_wrap_around_id = 0;
static GQuark was_entire_word_id = 0;
static GQuark was_case_sensitive_id = 0;
static GQuark was_search_backwards_id = 0;

GQuark 
gedit_was_wrap_around_quark (void)
{
	if (!was_wrap_around_id)
		was_wrap_around_id = g_quark_from_static_string ("GeditWasWrapAround");

	return was_wrap_around_id;
}

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
text_not_found_dialog (const gchar *text, GtkWindow *parent)
{
	GtkWidget *message_dlg;

	g_return_if_fail (text != NULL);
	
	message_dlg = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			_("The text \"%s\" was not found."), text);
		
	gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (message_dlg), FALSE);
	
	gtk_dialog_run (GTK_DIALOG (message_dlg));
  	gtk_widget_destroy (message_dlg);
}

static void
update_menu_items_sensitivity (void)
{
	BonoboWindow* active_window = NULL;
	GeditDocument* doc = NULL;
	BonoboUIComponent *ui_component;
	gchar *lst;
	
	gedit_debug (DEBUG_SEARCH, "");
	
	active_window = gedit_get_active_window ();
	g_return_if_fail (active_window != NULL);
	
	ui_component = bonobo_mdi_get_ui_component_from_window (active_window);
	g_return_if_fail (ui_component != NULL);

	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	lst = gedit_document_get_last_searched_text (doc);

	gedit_menus_set_verb_sensitive (ui_component, "/commands/SearchFindAgain", lst != NULL);	
	g_free (lst);
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

static void 
replace_search_entry_changed (GtkEditable *editable, GeditDialogReplace *dialog)
{
	const gchar *search_string;
	
	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));		
	g_return_if_fail (search_string != NULL);

	if (strlen (search_string) <= 0)
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_FIND, FALSE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE, FALSE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE_ALL, FALSE);
	}
	else
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_FIND, TRUE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE_ALL, TRUE);
	}
}

static GeditDialogReplace *
dialog_replace_get_dialog (void)
{
	static GeditDialogReplace *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *replace_with_label;
	
	gedit_debug (DEBUG_SEARCH, "");
	
	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      GTK_WINDOW (window));
		gtk_window_present (GTK_WINDOW (dialog->dialog));
		gtk_widget_grab_focus (dialog->dialog);

		return dialog;
	}

	gui = glade_xml_new ( GEDIT_GLADEDIR "replace.glade2",
			     "replace_dialog_content", NULL);
	if (!gui)
	{
		gedit_warning (window,
			       MISSING_FILE,
			       GEDIT_GLADEDIR "replace.glade2");
		return NULL;
	}

	dialog = g_new0 (GeditDialogReplace, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Replace"),
						      window,						      
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog->dialog), FALSE);

	gtk_dialog_add_button (GTK_DIALOG (dialog->dialog), 
			       _("Replace _All"), GEDIT_RESPONSE_REPLACE_ALL);

	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog),
				 _("_Replace"), GTK_STOCK_FIND_AND_REPLACE,
				 GEDIT_RESPONSE_REPLACE);

	gtk_dialog_add_button (GTK_DIALOG (dialog->dialog), 
			       GTK_STOCK_FIND, GEDIT_RESPONSE_FIND);

	content = glade_xml_get_widget (gui, "replace_dialog_content");
	dialog->search_entry       = glade_xml_get_widget (gui, "search_for_text_entry");
	dialog->replace_entry      = glade_xml_get_widget (gui, "replace_with_text_entry");
	dialog->search_entry_list  = glade_xml_get_widget (gui, "search_for_text_entry_list");
	dialog->replace_entry_list = glade_xml_get_widget (gui, "replace_with_text_entry_list");
	replace_with_label         = glade_xml_get_widget (gui, "replace_with_label");
	dialog->match_case_checkbutton = glade_xml_get_widget (gui, "match_case_checkbutton");
	dialog->wrap_around_checkbutton =  glade_xml_get_widget (gui, "wrap_around_checkbutton");
	dialog->entire_word_checkbutton = glade_xml_get_widget (gui, "entire_word_checkbutton");
	dialog->search_backwards_checkbutton = glade_xml_get_widget (gui, "search_backwards_checkbutton");

	if (!content				||
	    !dialog->search_entry 		||
	    !dialog->replace_entry  		||
	    !dialog->search_entry_list		||
	    !dialog->replace_entry_list		||  
	    !replace_with_label 		||
	    !dialog->match_case_checkbutton 	||
	    !dialog->entire_word_checkbutton 	||
	    !dialog->wrap_around_checkbutton 	||
	    !dialog->search_backwards_checkbutton)
	{
		gedit_warning (window,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "replace.glade2");
		return NULL;
	}

	gtk_widget_show (replace_with_label);
	gtk_widget_show (dialog->replace_entry);
	gtk_widget_show (dialog->replace_entry_list);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_FIND);

	g_signal_connect (G_OBJECT (dialog->search_entry_list), "changed",
			  G_CALLBACK (replace_search_entry_changed), dialog);
	
	g_signal_connect (G_OBJECT (dialog->search_entry), "insert_text",
			  G_CALLBACK (insert_text_handler), NULL);

	g_signal_connect (G_OBJECT (dialog->replace_entry), "insert_text",
			  G_CALLBACK (insert_text_handler), NULL);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			  G_CALLBACK (dialog_replace_response_handler), dialog);

	g_object_unref (G_OBJECT (gui));

	return dialog;
}

static void 
find_search_entry_changed (GtkEditable *editable, GeditDialogReplace *dialog)
{
	const gchar *search_string;
	
	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));		
	g_return_if_fail (search_string != NULL);

	if (strlen (search_string) <= 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_FIND, FALSE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_FIND, TRUE);
}

static GeditDialogFind *
dialog_find_get_dialog (void)
{
	static GeditDialogFind *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *replace_with_label;
	GtkWidget *replace_entry;
	GtkWidget *table;
	
	gedit_debug (DEBUG_SEARCH, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      GTK_WINDOW (window));
		gtk_window_present (GTK_WINDOW (dialog->dialog));
		gtk_widget_grab_focus (dialog->dialog);

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "replace.glade2",
			     "replace_dialog_content", NULL);
	if (!gui)
	{
		gedit_warning (window,
			       MISSING_FILE,
			       GEDIT_GLADEDIR "replace.glade2");
		return NULL;
	}

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

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog->dialog), FALSE);

	content = glade_xml_get_widget (gui, "replace_dialog_content");

	dialog->search_entry       = glade_xml_get_widget (gui, "search_for_text_entry");
	dialog->search_entry_list  = glade_xml_get_widget (gui, "search_for_text_entry_list");	
	replace_entry      	   = glade_xml_get_widget (gui, "replace_with_text_entry_list");

	replace_with_label         = glade_xml_get_widget (gui, "replace_with_label");
	
	dialog->match_case_checkbutton = glade_xml_get_widget (gui, "match_case_checkbutton");
	dialog->wrap_around_checkbutton =  glade_xml_get_widget (gui, "wrap_around_checkbutton");
	dialog->entire_word_checkbutton = glade_xml_get_widget (gui, "entire_word_checkbutton");
	dialog->search_backwards_checkbutton = glade_xml_get_widget (gui, "search_backwards_checkbutton");

	table                      = glade_xml_get_widget (gui, "table");

	if (!content				||
	    !table				||
	    !dialog->search_entry 		||
	    !dialog->search_entry_list 		||
	    !replace_entry			||
	    !replace_with_label 		||
	    !dialog->match_case_checkbutton 	||
	    !dialog->entire_word_checkbutton 	||
	    !dialog->wrap_around_checkbutton	||
	    !dialog->search_backwards_checkbutton)
	{
		gedit_warning (window,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "replace.glade2");
		return NULL;
	}

	gtk_widget_hide (replace_with_label);
	gtk_widget_hide (replace_entry);

	gtk_table_set_row_spacings (GTK_TABLE (table), 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_FIND);

	g_signal_connect (G_OBJECT (dialog->search_entry_list), "changed",
			  G_CALLBACK (find_search_entry_changed), dialog);

	g_signal_connect (G_OBJECT (dialog->search_entry), "insert_text",
			  G_CALLBACK (insert_text_handler), NULL);

	g_signal_connect(G_OBJECT (dialog->dialog), "destroy",
			 G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect(G_OBJECT (dialog->dialog), "response",
			 G_CALLBACK (dialog_find_response_handler), dialog);

	g_object_unref (G_OBJECT (gui));

	return dialog;
}

static gchar* 
escape_search_text (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

	gedit_debug (DEBUG_SEARCH, "Text: %s", text);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '\n':
          			g_string_append (str, "\\n");
          			break;
			case '\r':
          			g_string_append (str, "\\r");
          			break;
			case '\t':
          			g_string_append (str, "\\t");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

static void
insert_text_handler (GtkEditable *editable, const gchar *text, gint length, gint *position)
{
	static gboolean insert_text = FALSE;
	gchar *escaped_text;
	gint new_len;

	gedit_debug (DEBUG_SEARCH, "Text: %s", text);

	/* To avoid recursive behavior */
	if (insert_text)
		return;

	escaped_text = escape_search_text (text);

	gedit_debug (DEBUG_SEARCH, "Escaped Text: %s", escaped_text);

	new_len = strlen (escaped_text);

	if (new_len == length)
	{
		g_free (escaped_text);
		return;
	}

	insert_text = TRUE;

	g_signal_stop_emission_by_name (editable, "insert_text");
	
	gtk_editable_insert_text (editable, escaped_text, new_len, position);

	insert_text = FALSE;

	g_free (escaped_text);
}

void
gedit_dialog_find (void)
{
	GeditDialogFind *dialog;
	GeditMDIChild *active_child;
	GeditDocument *doc;
	gchar* last_searched_text;
	gint selection_start, selection_end;
	gboolean selection_exists;
	gboolean was_wrap_around;
	gboolean was_entire_word;
	gboolean was_case_sensitive;
	gboolean was_search_backwards;
	gpointer data;
	
	gedit_debug (DEBUG_SEARCH, "");

	dialog = dialog_find_get_dialog ();
	if (!dialog)
		return;

	if (GTK_WIDGET_VISIBLE (dialog->dialog))
		return;

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
					   GEDIT_RESPONSE_FIND, FALSE);

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	selection_exists = gedit_document_get_selection (doc, &selection_start, &selection_end);
	if (selection_exists && (selection_end - selection_start < 80)) 
	{
		gchar *selection_text;
		
		selection_text = gedit_document_get_chars (doc, selection_start, selection_end);
		
		gtk_entry_set_text (GTK_ENTRY (dialog->search_entry), selection_text);
		
		g_free (selection_text);
	} 
	else 
	{
		last_searched_text = gedit_document_get_last_searched_text (doc);
		if (last_searched_text != NULL)
		{
			gtk_entry_set_text (GTK_ENTRY (dialog->search_entry), last_searched_text);
			g_free (last_searched_text);	
		}
	}

	if (!was_search_backwards_id)
		was_search_backwards_id = g_quark_from_static_string ("GeditWasSearchBackwards");

	if (!was_wrap_around_id)
		was_wrap_around_id = gedit_was_wrap_around_quark ();

	if (!was_entire_word_id)
		was_entire_word_id = g_quark_from_static_string ("GeditWasEntireWord");

	if (!was_case_sensitive_id)
		was_case_sensitive_id = g_quark_from_static_string ("GeditWasCaseSensitive");

	data = g_object_get_qdata (G_OBJECT (doc), was_wrap_around_id);
	if (data == NULL)
		was_wrap_around = TRUE;
	else
		was_wrap_around = GPOINTER_TO_BOOLEAN (data);

	data = g_object_get_qdata (G_OBJECT (doc), was_search_backwards_id);
	if (data == NULL)
		was_search_backwards = FALSE;
	else
		was_search_backwards = GPOINTER_TO_BOOLEAN (data);

	data = g_object_get_qdata (G_OBJECT (doc), was_entire_word_id);
	if (data == NULL)
		was_entire_word = FALSE;
	else
		was_entire_word = GPOINTER_TO_BOOLEAN (data);

	data = g_object_get_qdata (G_OBJECT (doc), was_case_sensitive_id);
	if (data == NULL)
		was_case_sensitive = FALSE;
	else
		was_case_sensitive = GPOINTER_TO_BOOLEAN (data);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton), 
				      was_case_sensitive);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton),
				      was_entire_word);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton),
				      was_wrap_around);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->search_backwards_checkbutton),
				      was_search_backwards);

	gtk_widget_grab_focus (dialog->search_entry);
	
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
	gint selection_start, selection_end;
	gboolean selection_exists;
	gboolean was_wrap_around;
	gboolean was_entire_word;
	gboolean was_case_sensitive;
	gboolean was_search_backwards;
	gpointer data;

	gedit_debug (DEBUG_SEARCH, "");

	dialog = dialog_replace_get_dialog ();
	if (!dialog)
		return;

	if (GTK_WIDGET_VISIBLE (dialog->dialog))
		return;

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_FIND, FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE, FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE_ALL, FALSE);

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	selection_exists = gedit_document_get_selection (doc, &selection_start, &selection_end);
	if (selection_exists && (selection_end - selection_start < 80)) 
	{
		gchar *selection_text;
		
		selection_text = gedit_document_get_chars (doc, selection_start, selection_end);
		
		gtk_entry_set_text (GTK_ENTRY (dialog->search_entry), selection_text);
		
		g_free (selection_text);
	} 
	else 
	{
		last_searched_text = gedit_document_get_last_searched_text (doc);
		if (last_searched_text != NULL)
		{
			gtk_entry_set_text (GTK_ENTRY (dialog->search_entry), last_searched_text);
			g_free (last_searched_text);	
		}
	}
	
	last_replace_text = gedit_document_get_last_replace_text (doc);
	if (last_replace_text != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY (dialog->replace_entry), last_replace_text);
		g_free (last_replace_text);	
	}

	if (!was_search_backwards_id)
		was_search_backwards_id = g_quark_from_static_string ("GeditWasSearchBackwards");

	if (!was_wrap_around_id)
		was_wrap_around_id = gedit_was_wrap_around_quark ();

	if (!was_entire_word_id)
		was_entire_word_id = g_quark_from_static_string ("GeditWasEntireWord");

	if (!was_case_sensitive_id)
		was_case_sensitive_id = g_quark_from_static_string ("GeditWasCaseSensitive");

	data = g_object_get_qdata (G_OBJECT (doc), was_search_backwards_id);
	if (data == NULL)
		was_search_backwards = FALSE;
	else
		was_search_backwards = GPOINTER_TO_BOOLEAN (data);

	data = g_object_get_qdata (G_OBJECT (doc), was_wrap_around_id);
	if (data == NULL)
		was_wrap_around = TRUE;
	else
		was_wrap_around = GPOINTER_TO_BOOLEAN (data);

	data = g_object_get_qdata (G_OBJECT (doc), was_entire_word_id);
	if (data == NULL)
		was_entire_word = FALSE;
	else
		was_entire_word = GPOINTER_TO_BOOLEAN (data);

	data = g_object_get_qdata (G_OBJECT (doc), was_case_sensitive_id);
	if (data == NULL)
		was_case_sensitive = FALSE;
	else
		was_case_sensitive = GPOINTER_TO_BOOLEAN (data);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton), 
				      was_case_sensitive);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton),
				      was_entire_word);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton),
				      was_wrap_around);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->search_backwards_checkbutton),
				      was_search_backwards);

	gtk_widget_grab_focus (dialog->search_entry);

	gtk_widget_show (dialog->dialog);
}

static void
find_dlg_find_button_pressed (GeditDialogFind *dialog)
{
	GeditMDIChild *active_child;
	GeditView* active_view;
	GeditDocument *doc;
	const gchar* search_string = NULL;
	gboolean found;

	gboolean case_sensitive;
	gboolean entire_word;
	gboolean wrap_around;
	gboolean search_backwards;
	gint flags = 0;

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

	gnome_entry_prepend_history (GNOME_ENTRY (dialog->search_entry_list), TRUE, search_string);
		
	/* retrieve search settings from the dialog */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton));
	entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton));
	wrap_around = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton));
	search_backwards = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->search_backwards_checkbutton));

	/* setup quarks for next invocation */
	g_object_set_qdata (G_OBJECT (doc), was_search_backwards_id, GBOOLEAN_TO_POINTER (search_backwards));
	g_object_set_qdata (G_OBJECT (doc), was_wrap_around_id, GBOOLEAN_TO_POINTER (wrap_around));
	g_object_set_qdata (G_OBJECT (doc), was_entire_word_id, GBOOLEAN_TO_POINTER (entire_word));
	g_object_set_qdata (G_OBJECT (doc), was_case_sensitive_id, GBOOLEAN_TO_POINTER (case_sensitive));

	/* setup search parameter bitfield */
	GEDIT_SEARCH_SET_FROM_CURSOR (flags, TRUE);
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_BACKWARDS (flags, search_backwards);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);

	/* run search */
	found = gedit_document_find (doc, search_string, flags);

	/* if we're able to wrap, don't use the cursor position */
	if (!found && wrap_around)
	{
		GEDIT_SEARCH_SET_FROM_CURSOR (flags, FALSE);
		found = gedit_document_find (doc, search_string, flags);
	}

	if (found)
		gedit_view_scroll_to_cursor (active_view);
	else
		text_not_found_dialog (search_string, (GTK_WINDOW (dialog->dialog)));

	update_menu_items_sensitivity ();
}


static void
replace_dlg_find_button_pressed (GeditDialogReplace *dialog)
{
	/* This is basically the same as find_dlg_find_button_pressed */

	GeditMDIChild *active_child;
	GeditView* active_view;
	GeditDocument *doc;
	const gchar* search_string = NULL;
	gboolean found;

	gboolean case_sensitive;
	gboolean entire_word;
	gboolean wrap_around;
	gboolean search_backwards;
	gint flags = 0;

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
	
	gnome_entry_prepend_history (GNOME_ENTRY (dialog->search_entry_list), TRUE, search_string);
		
	/* retrieve search settings from the dialog */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton));
	entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton));
	wrap_around = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton));
	search_backwards = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->search_backwards_checkbutton));

	/* setup quarks for next invocation */
	g_object_set_qdata (G_OBJECT (doc), was_search_backwards_id, GBOOLEAN_TO_POINTER (search_backwards));
	g_object_set_qdata (G_OBJECT (doc), was_wrap_around_id, GBOOLEAN_TO_POINTER (wrap_around));
	g_object_set_qdata (G_OBJECT (doc), was_entire_word_id, GBOOLEAN_TO_POINTER (entire_word));
	g_object_set_qdata (G_OBJECT (doc), was_case_sensitive_id, GBOOLEAN_TO_POINTER (case_sensitive));

	/* setup search parameter bitfield */
	GEDIT_SEARCH_SET_FROM_CURSOR (flags, TRUE);
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_BACKWARDS (flags, search_backwards);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);

	/* run search */
	found = gedit_document_find (doc, search_string, flags);

	/* if we're able to wrap, don't use the cursor position */
	if (!found && wrap_around)
	{
		GEDIT_SEARCH_SET_FROM_CURSOR (flags, FALSE);
		found = gedit_document_find (doc, search_string, flags);
	}

	if (found)
		gedit_view_scroll_to_cursor (active_view);
	else
		text_not_found_dialog (search_string, (GTK_WINDOW (dialog->dialog)));

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, found);

	update_menu_items_sensitivity ();
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
	gchar *converted_search_string = NULL;
	gint start, end;
	gboolean found;

	gboolean case_sensitive;
	gboolean entire_word;
	gboolean wrap_around;
	gboolean search_backwards;
	gint flags = 0;

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
	
	gnome_entry_prepend_history (GNOME_ENTRY (dialog->search_entry_list), TRUE, search_string);

	if (strlen (replace_string) > 0)
		gnome_entry_prepend_history (GNOME_ENTRY (dialog->replace_entry_list), 
					     TRUE, 
					     replace_string);

	if (gedit_document_get_selection (doc, &start, &end))
		selected_text = gedit_document_get_chars (doc, start, end);
	
	gedit_debug (DEBUG_SEARCH, "Sel text: %s", selected_text ? selected_text : "NULL");
	gedit_debug (DEBUG_SEARCH, "Search string: %s", search_string ? search_string : "NULL");

	/* retrieve search settings from the dialog */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton));
	entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton));
	wrap_around = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton));
	search_backwards = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->search_backwards_checkbutton));

	converted_search_string = gedit_utils_convert_search_text (search_string);

	if ((selected_text == NULL) ||
	    (case_sensitive && (strcmp (selected_text, converted_search_string)
	     !=0)) || (!case_sensitive && !g_utf8_caselessnmatch (selected_text, search_string, 
						       strlen (selected_text), 
						       strlen (search_string)) != 0))
	{
		gedit_debug (DEBUG_SEARCH, "selected_text (%s) != search_string (%s)", 
			     selected_text ? selected_text : "NULL",
			     search_string ? search_string : "NULL");

		if (selected_text != NULL)
			g_free (selected_text);

		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE, FALSE);
		
		replace_dlg_find_button_pressed (dialog);

		return;
	}
				
	g_free (selected_text);
	g_free (converted_search_string);
		
	gedit_debug (DEBUG_SEARCH, "Replace string: %s", replace_string ? replace_string : "NULL");

	gedit_document_replace_selected_text (doc, replace_string);

	gedit_debug (DEBUG_SEARCH, "Replaced");

	/* setup search parameter bitfield */
	GEDIT_SEARCH_SET_FROM_CURSOR (flags, TRUE);
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_BACKWARDS (flags, search_backwards);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);

	/* run search */
	found = gedit_document_find (doc, search_string, flags);

	/* if we're able to wrap, don't use the cursor position */
	if (!found && wrap_around)
	{
		GEDIT_SEARCH_SET_FROM_CURSOR (flags, FALSE);
		found = gedit_document_find (doc, search_string, flags);
	}

	if (found)
		gedit_view_scroll_to_cursor (active_view);
	
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
							GEDIT_RESPONSE_REPLACE, found);	

	update_menu_items_sensitivity ();

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
	gint replaced_items;
	GtkWidget *message_dlg;
	
	gboolean case_sensitive;
	gboolean entire_word;
	gint flags = 0;
	
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

	g_return_if_fail (search_string);
	g_return_if_fail (replace_string);

	if (strlen (search_string) <= 0)
		return;
	
	gnome_entry_prepend_history (GNOME_ENTRY (dialog->search_entry_list), TRUE, search_string);

	if (strlen (replace_string) > 0)
		gnome_entry_prepend_history (GNOME_ENTRY (dialog->replace_entry_list), 
					     TRUE, 
					     replace_string);

	/* retrieve search settings from the dialog */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton));
	entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton));

	/* setup search parameter bitfield */
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);

	replaced_items = gedit_document_replace_all (doc, search_string, replace_string, flags);

	update_menu_items_sensitivity ();

	if (replaced_items <= 0)
	{
		message_dlg = gtk_message_dialog_new (
			GTK_WINDOW (dialog->dialog),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			_("The text \"%s\" was not found."), search_string);
	}
	else
	{	
		message_dlg = gtk_message_dialog_new (
			GTK_WINDOW (dialog->dialog),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			ngettext("Found and replaced %d occurrence.",
				 "Found and replaced %d occurrences.",
				 replaced_items), replaced_items);
	}
	
	gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

	gtk_dialog_run (GTK_DIALOG (message_dlg));
  	gtk_widget_destroy (message_dlg);
}
