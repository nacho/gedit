/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose Celorio
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
 *
 * Author :
 *    Chema Celorio <chema@celorio.com>
 * 
 */

#include <config.h>
#include <gnome.h>

#include "view.h"
#include "document.h"
#include "search.h"
#include "utils.h"
#include "window.h"
#include "dialogs/dialogs.h"

#include <glade/glade.h>

static void replace_text_destroyed_cb (GtkWidget *widget, gint button);
static void replace_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog);
static void search_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog);
static void replace_text_clicked_cb (GtkWidget *widget, gint button);
       void dialog_replace (gint full);


static void
replace_text_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug("", DEBUG_SEARCH);
	gedit_search_end();
	widget = NULL;
}


static void
replace_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	/* behave as if the user clicked Find/Find next button */
	gedit_debug("", DEBUG_SEARCH);
	replace_text_clicked_cb (dialog, 0);
}

static void
search_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	gedit_debug("", DEBUG_SEARCH);
	replace_text_clicked_cb (dialog, 0);
}

static void
replace_text_clicked_cb (GtkWidget *widget, gint button)
{
	GtkWidget *search_entry, *replace_entry, *case_sensitive, *radio_button_1;
	gint line_found, total_lines, eureka;
	gint start_search_from;
	guint pos_found, start_pos = 0, end_pos = 0;
	gint search_text_length, replace_text_length;
	gchar * text_to_search_for, *text_to_replace_with;
	
	View *view;

	gedit_debug("", DEBUG_SEARCH);

	if (!search_verify_document())
	{
		/* There are no documents open */
		gnome_dialog_close (GNOME_DIALOG (widget));
		return;
	}

	view = gedit_view_current();

	g_return_if_fail (view!=NULL);

	search_entry   = gtk_object_get_data (GTK_OBJECT (widget), "search_for_text_entry");
	replace_entry  = gtk_object_get_data (GTK_OBJECT (widget), "replace_with_text_entry");
	case_sensitive = gtk_object_get_data (GTK_OBJECT (widget), "case_sensitive");
	radio_button_1 = gtk_object_get_data (GTK_OBJECT (widget), "radio_button_1");

	g_assert (search_entry   != NULL);
	g_assert (replace_entry  != NULL);
	g_assert (case_sensitive != NULL);
	g_assert (radio_button_1 != NULL);

	text_to_search_for   = gtk_entry_get_text (GTK_ENTRY(search_entry));
	text_to_replace_with = gtk_entry_get_text (GTK_ENTRY(replace_entry));
	search_text_length   = strlen (text_to_search_for);
	replace_text_length  = strlen (text_to_replace_with);

	if (gedit_search_info.last_text_searched != NULL)
	{
		g_free (gedit_search_info.last_text_searched);
	}
	gedit_search_info.last_text_searched = g_strdup (text_to_search_for);
	gedit_search_info.last_text_searched_case_sensitive = GTK_TOGGLE_BUTTON (case_sensitive)->active;

	start_search_from = gtk_radio_group_get_selected (GTK_RADIO_BUTTON(radio_button_1)->group);

	switch (start_search_from){
	case 0:
		start_pos = 0;
		break;
	case 1:
		if (gedit_view_get_selection (view, &start_pos, &end_pos))
			start_pos = end_pos;
		else
			start_pos = gedit_view_get_position (view);
		break;
	default:
		g_warning ("Invalid start search from value");
		button = 3; /* Act as if close was clicked */
		break;
	}
		
	if (button == 3) /* Close */
		gnome_dialog_close (GNOME_DIALOG (widget));
	
	if (button == 2) /* Replace All */
	{
		/* At this point the text we need to replace is selected */
		/* we need to 1. call view_delete for the text selected */
		/* and 2. call view insert to insert the new text */
		/* FIXME : We can assume that the entries will not change for now,
		   we will need to add a callback to the text entries on their
		   chage signals. Chema [ This might not be true anymore since
		   we are "freezing gedit before doing the search/replace */

		gint replacements = 0;
		guchar *msg;
		guchar *new_buffer = NULL;

		start_search_from = gtk_radio_group_get_selected (GTK_RADIO_BUTTON(radio_button_1)->group);
		replacements = gedit_search_replace_all_execute ( view,
								  start_pos,
								  text_to_search_for,
								  text_to_replace_with,
								  GTK_TOGGLE_BUTTON (case_sensitive)->active,
								  &new_buffer);

		if (replacements > 0)
		{
			gedit_document_delete_text (view->document, 0, gedit_document_get_buffer_length(view->document), FALSE);
			gedit_document_insert_text (view->document, new_buffer, 0, FALSE);
			gedit_search_end();
			gedit_search_start();
		}

		if (new_buffer==NULL)
			g_free (new_buffer);
		
		gnome_dialog_close (GNOME_DIALOG (widget));
			
		msg = g_strdup_printf (_("found and replaced %i occurrences."),
				       replacements);
		gnome_dialog_run_and_close ((GnomeDialog *)
					    gnome_message_box_new (msg,
								   GNOME_MESSAGE_BOX_INFO,
								   GNOME_STOCK_BUTTON_OK,
								   NULL));
		g_free (msg);
	}
	 
	if (button == 1) /* Replace */
	{
		/* At this point the text we need to replace is selected */
		/* we need to 1. call view_delete for the text selected */
		/* and 2. call view insert to insert the new text */
		/* FIXME : We can assume that the entries will not change for now,
		   we will need to add a callback to the text entries on their
		   chage signals */

		guint start, end;

		if (!gedit_view_get_selection (view, &start, &end))
			g_warning("This should not happen !!!!. There should be some text selected");
		/* FIXME !!!!!!!!!! this is a big problem testing for
		   a selection is not the way to do it !!!. Chema */

		/* Diselect the text and set the point after this occurence*/
		gedit_document_delete_text (view->document, start, search_text_length, TRUE);
		gedit_document_insert_text (view->document, text_to_replace_with, start, TRUE);

		/* We need to reload the buffer since we changed it */
		gedit_search_end();
		gedit_search_start();

		/* After replacing this occurrence, act as is a Find Next was pressed */
		button = 0;
	}
	
	if (button == 0) /* Find Next */
	{
		eureka = search_text_execute ( start_pos,
					       GTK_TOGGLE_BUTTON (case_sensitive)->active,
					       text_to_search_for,
					       &pos_found,
					       &line_found,
					       &total_lines,
					       TRUE);

		if (!eureka)
		{
			search_text_not_found_notify (view);
			gtk_object_destroy (GTK_OBJECT(widget));
			return;
		}

		gedit_flash_va (_("Text found at line :%i"),line_found);
		gedit_view_set_window_position_from_lines (view, line_found, total_lines);
		
		gtk_text_set_point (GTK_TEXT(view->text), pos_found+1);
		gtk_text_insert (GTK_TEXT(view->text), NULL, NULL, NULL, " ", 1);
		gtk_text_backward_delete (GTK_TEXT(view->text), 1);
		gtk_editable_select_region (GTK_EDITABLE(view->text), pos_found+1, pos_found+1+search_text_length);

		gtk_radio_button_select (GTK_RADIO_BUTTON(radio_button_1)->group, 1);
	}
}

void
dialog_replace (gint full)
{
	static GtkWidget *replace_text_dialog = NULL;
	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *case_sensitive;
	GtkWidget *radio_button_1;
	GtkWidget *hbox_replace_with;
	GtkWidget *replace_button;
	GtkWidget *replace_all_button;
	/* kill warning 
	GtkWidget *ask_before_replacing;
	*/
	GladeXML  *gui;

	gedit_debug("", DEBUG_SEARCH);
	
	if (replace_text_dialog!=NULL)
	{
		/* If the dialog is open and active, just show it
		   should we "raise" it instead for showing it ? maybe
		   chema */
		/* FIXME !!!!!!!!
		g_print(" ........\n");
		gdk_window_show (replace_text_dialog->window);
		gdk_window_raise (replace_text_dialog->window);
		return;
		*/
	}
	
	gui = glade_xml_new (GEDIT_GLADEDIR
			     "/replace.glade",
			     NULL);
	if (!gui)
	{
		g_warning ("Could not find search.glade, reinstall gedit.");
		return;
	}

	replace_text_dialog   = glade_xml_get_widget (gui, "dialog");
	search_entry          = glade_xml_get_widget (gui, "search_for_text_entry");
	replace_entry         = glade_xml_get_widget (gui, "replace_with_text_entry");
	case_sensitive        = glade_xml_get_widget (gui, "case_sensitive");
	radio_button_1        = glade_xml_get_widget (gui, "radio_button_1");
	hbox_replace_with     = glade_xml_get_widget (gui, "hbox_replace_with");
	replace_button        = glade_xml_get_widget (gui, "replace_button");
	replace_all_button    = glade_xml_get_widget (gui, "replace_all_button");
	/* disabled because it was causing the dialog to pop up twice
	ask_before_replacing  = glade_xml_get_widget (gui, "ask_before_replacing");
	*/

	/* FIXME: We hide this feature always because is not
	   implemented yet. Chema 
	gtk_widget_hide (ask_before_replacing);
	*/

	if (!replace_text_dialog ||
	    !search_entry ||
	    !replace_entry ||
	    !case_sensitive ||
	    !radio_button_1 ||
	    !hbox_replace_with ||
	    !replace_button ||
	    !replace_all_button /*||
				  !ask_before_replacing*/)
	{
		g_warning ("Corrupted search.glade detected, reinstall gedit.");
		return;
	}

	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "search_for_text_entry", search_entry);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "replace_with_text_entry", replace_entry);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "case_sensitive", case_sensitive);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "radio_button_1", radio_button_1);

	gedit_search_start();
	
	gtk_signal_connect (GTK_OBJECT (replace_text_dialog), "clicked",
			    GTK_SIGNAL_FUNC (replace_text_clicked_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (replace_text_dialog), "destroy",
			    GTK_SIGNAL_FUNC (replace_text_destroyed_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (search_entry), "activate",
			    GTK_SIGNAL_FUNC (replace_entry_activate_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (replace_entry), "activate",
			    GTK_SIGNAL_FUNC (search_entry_activate_cb), replace_text_dialog);

	gnome_dialog_set_parent (GNOME_DIALOG (replace_text_dialog),
				 gedit_window_active());
	gtk_window_set_modal (GTK_WINDOW (replace_text_dialog), TRUE);


	/* NOT needed
	gtk_widget_show_all (replace_text_dialog); 
	*/

	if (!full)
	{
		gtk_widget_hide (hbox_replace_with);
		gtk_widget_hide (replace_button);
		gtk_widget_hide (replace_all_button);
	}

	gtk_object_unref (GTK_OBJECT (gui));

	gnome_dialog_run (GNOME_DIALOG(replace_text_dialog));
}


