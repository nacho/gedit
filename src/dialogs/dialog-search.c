/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
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
 */

#include <config.h>
#include <gnome.h>

#include "document.h"
#include "search.h"
#include "view.h"
#include "utils.h"
#include "dialogs/dialogs.h"

#include <glade/glade.h>

GtkWidget *search_text_dialog;

static void search_text_clicked_cb (GtkWidget *widget, gint button);

static void
search_text_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug("\n", DEBUG_SEARCH);
	search_end();
	search_text_dialog = NULL;
}


static void
search_text_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	/* behave as if the user clicked Find/Find next button */
	gedit_debug("\n", DEBUG_SEARCH);
	search_text_clicked_cb (dialog, 0);
}

static void
search_text_clicked_cb (GtkWidget *widget, gint button)
{
	GtkWidget *text_entry, *case_sensitive, *radio_button_1;
	gint line_found, total_lines, eureka;
	gint start_search_from;
	gulong pos_found, start_pos = 0;
	gint text_length;
	gchar * text_to_search_for;
	

	View *view;
	GtkText *text;

	
	gedit_debug("\n", DEBUG_SEARCH);

	if (button == 1)
		gnome_dialog_close (GNOME_DIALOG (widget));
	
	if (button == 0)
	{
		if (!search_verify_document())
		{
			/* There are no documents open */
			gnome_dialog_close (GNOME_DIALOG (widget));
			return;
		}

		view = VIEW (mdi->active_view);
		text = GTK_TEXT (view->text);

		g_assert ((text!= NULL)&&(view!=NULL));

		text_entry     = gtk_object_get_data (GTK_OBJECT (widget), "text_entry");
		case_sensitive = gtk_object_get_data (GTK_OBJECT (widget), "case_sensitive");
		radio_button_1 = gtk_object_get_data (GTK_OBJECT (widget), "radio_button_1");

		g_assert (text_entry     != NULL);
		g_assert (case_sensitive != NULL);
		g_assert (radio_button_1 != NULL);

		text_to_search_for = gtk_entry_get_text (GTK_ENTRY(text_entry));
		text_length = strlen (text_to_search_for);

		if (text_length <1)
			return;

		start_search_from = gtk_radio_group_get_selected (GTK_RADIO_BUTTON(radio_button_1)->group);
		
		switch (start_search_from){
		case 0:
			start_pos = 0;
			break;
		case 1:
			if (GTK_EDITABLE(text)->selection_end_pos)
				start_pos = GTK_EDITABLE(text)->selection_end_pos;
			else
				start_pos = gtk_text_get_point (text);
			break;
		default:
			g_print("other ...\n");
			break;
		}
			
		eureka = search_text_execute ( start_pos,
					       GTK_TOGGLE_BUTTON (case_sensitive)->active,
					       text_to_search_for,
					       &pos_found,
					       &line_found,
					       &total_lines,
					       TRUE);


		if (!eureka)
		{
			gedit_flash_va (_("Text not found"));
			return;
		}

		gedit_flash_va (_("Text found at line :%i"),line_found);

		update_text (text, line_found, total_lines);
		gtk_text_set_point (text, pos_found+1);

		gtk_text_insert (text, NULL, NULL, NULL, " ", 1);
		gtk_text_backward_delete (text, 1);
		
		gtk_editable_select_region (GTK_EDITABLE(text), pos_found+1, pos_found+1+text_length);

		gtk_radio_button_select (GTK_RADIO_BUTTON(radio_button_1)->group, 1);
	}
}


void
dialog_find (void)
{
/*	GtkWidget *dialog;*/ 
	GtkWidget *text_entry;
	GtkWidget *case_sensitive;
	GtkWidget *radio_button_1;
	/* GtkAdjustment *adj; */
	GladeXML *gui;
	/*
  gint total_lines, line_number;
*/

	gedit_debug("\n", DEBUG_SEARCH);

	if (search_text_dialog!=NULL)
	{
		/* If the dialog is open and active, just show it
		   should we "raise" it instead for showing it ? maybe
		   chema */
		gtk_widget_show (search_text_dialog);
		return;
	}
	
	gui = glade_xml_new (GEDIT_GLADEDIR
			     "/search.glade",
			     NULL);
	if (!gui)
	{
		g_warning ("Could not find search.glade, reinstall gedit.");
		return;
	}

	/* FIXME : change the "Find Next" label. */
	search_text_dialog = glade_xml_get_widget (gui, "dialog");
	text_entry     = glade_xml_get_widget (gui, "text_entry");
	case_sensitive = glade_xml_get_widget (gui, "case_sensitive");
	radio_button_1 = glade_xml_get_widget (gui, "start_search_from_radio_button_1");
	
	if (!search_text_dialog || !text_entry || !case_sensitive || !radio_button_1)
	{
		g_warning ("Corrupted search.glade detected, reinstall gedit.");
		return;
	}

	gtk_object_set_data (GTK_OBJECT (search_text_dialog), "text_entry", text_entry);
	gtk_object_set_data (GTK_OBJECT (search_text_dialog), "case_sensitive", case_sensitive);
	gtk_object_set_data (GTK_OBJECT (search_text_dialog), "radio_button_1", radio_button_1);

	search_start();
	
	gtk_signal_connect (GTK_OBJECT (search_text_dialog), "clicked",
			    GTK_SIGNAL_FUNC (search_text_clicked_cb), search_text_dialog);
	gtk_signal_connect (GTK_OBJECT (search_text_dialog), "destroy",
			    GTK_SIGNAL_FUNC (search_text_destroyed_cb), search_text_dialog);
	gtk_signal_connect (GTK_OBJECT (text_entry), "activate",
			    GTK_SIGNAL_FUNC (search_text_entry_activate_cb), search_text_dialog);


	gtk_window_set_modal (GTK_WINDOW (search_text_dialog), TRUE);
	gtk_widget_show_all (search_text_dialog);
	gtk_object_destroy (GTK_OBJECT (gui));
}
