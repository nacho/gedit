
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
 * Author :
 *    Chema Celorio <chema@celorio.com>
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

GtkWidget *replace_text_dialog;

static void replace_text_destroyed_cb (GtkWidget *widget, gint button);
static void replace_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog);
static void search_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog);
static void replace_text_clicked_cb (GtkWidget *widget, gint button);
static void views_delete (Document *doc, gulong start_pos, gulong end_pos);
static void views_insert (Document *doc, gulong start_pos, gchar * text_to_insert);

void dialog_replace (gint full);

static void
replace_text_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug("\n", DEBUG_SEARCH);
	search_end();
	replace_text_dialog = NULL;
}


static void
replace_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	/* behave as if the user clicked Find/Find next button */
	gedit_debug("\n", DEBUG_SEARCH);
	replace_text_clicked_cb (dialog, 0);
}

static void
search_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	/* behave as if the user clicked Find/Find next button */
	gedit_debug("\n", DEBUG_SEARCH);
	replace_text_clicked_cb (dialog, 0);
}

static void
replace_text_clicked_cb (GtkWidget *widget, gint button)
{
	GtkWidget *search_entry, *replace_entry, *case_sensitive, *radio_button_1;
	gint line_found, total_lines, eureka;
	gint start_search_from;
	gulong pos_found, start_pos = 0;
	gint search_text_length, replace_text_length;
	gchar * text_to_search_for, *text_to_replace_with;
	
	View *view;
	GtkText *text;

	gedit_debug("\n", DEBUG_SEARCH);

	if (!search_verify_document())
	{
		/* There are no documents open */
		gnome_dialog_close (GNOME_DIALOG (widget));
		return;
	}

	view = VIEW (mdi->active_view);
	text = GTK_TEXT (view->text);

	g_assert ((text!= NULL)&&(view!=NULL));

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

	g_print ("Button :%i\n", button);

	if (button == 3) /* Close */
		gnome_dialog_close (GNOME_DIALOG (widget));
	
	if (button == 2) /* Replace All */
		g_print("Replacing All ....\n");
	 
	if (button == 1) /* Replace */
	{
		/* At this point the text we need to replace is selected */
		/* we need to 1. call view_delete for the text selected */
		/* and 2. call view insert to insert the new text */
		/* FIXME : We can assume that the entries will not change for now,
		   we will need to add a callback to the text entries on their
		   chage signals */

		gulong start, end;

		if (!GTK_EDITABLE(text)->selection_end_pos)
			g_warning("This should not happen !!!!. There should be some text selected");

		start = GTK_EDITABLE(text)->selection_start_pos;
		end   = GTK_EDITABLE(text)->selection_end_pos;
		
		/* Diselect the text and set the point after this occurence*/

	        g_print("deleting text ....\n");

		doc_delete_text_cb (GTK_WIDGET(text), start, end, view);
		views_delete (view->document, start, end);

		doc_insert_text_cb (GTK_WIDGET(text), text_to_replace_with, replace_text_length, start, view);
		views_insert (view->document, start, text_to_replace_with);


		/*
		gtk_text_set_point (text, end);
		*/

		/* We need to reload the buffer since we changed it */
		search_end();
		search_start();

		/* After replacing this occurrence, act as is a Find Next was pressed */
		button = 0;
	}
	
	if (button == 0) /* Find Next */
	{
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

		g_print("Start_search_from %i start pos %lu\n", start_search_from, start_pos);
			
		eureka = search_text_execute ( start_pos,
					       GTK_TOGGLE_BUTTON (case_sensitive)->active,
					       text_to_search_for,
					       &pos_found,
					       &line_found,
					       &total_lines,
					       TRUE);


		if (!eureka)
		{
			search_text_not_found_notify (text);
			return;
		}

		gedit_flash_va (_("Text found at line :%i"),line_found);
		update_text (text, line_found, total_lines);
		gtk_text_set_point (text, pos_found+1);
		gtk_text_insert (text, NULL, NULL, NULL, " ", 1);
		gtk_text_backward_delete (text, 1);
		gtk_editable_select_region (GTK_EDITABLE(text), pos_found+1, pos_found+1+search_text_length);

		gtk_radio_button_select (GTK_RADIO_BUTTON(radio_button_1)->group, 1);
	}
}

/* FIXME: This rutine was copied from undo.c, we need not to duplicate
   it. But the parameters we need are diferent. We can do this when
   we rewrite view.c ( which is ugly ) Chema */
static void
views_delete (Document *doc, gulong start_pos, gulong end_pos)
{
	View *nth_view;
	gint n;
	gulong p1;

	gedit_debug (" start \n", DEBUG_SEARCH);

	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);

	        p1 = gtk_text_get_point (GTK_TEXT (nth_view->text));
		gtk_text_set_point (GTK_TEXT(nth_view->text), end_pos);
		gtk_text_backward_delete (GTK_TEXT (nth_view->text), (end_pos - start_pos));

#ifdef ENABLE_SPLIT_SCREEN
		g_print(" 1..");		
		gtk_text_freeze (GTK_TEXT (nth_view->split_screen));
		g_print(" 2..");		
	        p1 = gtk_text_get_point (GTK_TEXT (nth_view->split_screen));
		g_print(" 3..");		
		gtk_text_set_point (GTK_TEXT(nth_view->split_screen), start_pos);
		g_print(" 4..");		
		gtk_text_thaw (GTK_TEXT (nth_view->split_screen));
		g_print(" 5..");		
		gtk_text_backward_delete (GTK_TEXT (nth_view->split_screen), (end_pos - start_pos));
		/* I have not tried it but WHY whould you use backward above and forward here ? Chema
		gtk_text_forward_delete (GTK_TEXT (nth_view->split_screen), (end_pos - start_pos));
		*/
		g_print(" 6..\n");
#endif /* ENABLE_SPLIT_SCREEN */		
	}

	gedit_debug (" end \n", DEBUG_SEARCH);
}

static void
views_insert (Document *doc, gulong start_pos, gchar * text_to_insert)
{
	gint n;
	gint p1;
	View *nth_view;

	gedit_debug ("\n", DEBUG_SEARCH);
	
	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);
		/*
		gtk_text_freeze (GTK_TEXT (view->text));
		*/
		p1 = gtk_text_get_point (GTK_TEXT (nth_view->text));
		gtk_text_set_point (GTK_TEXT(nth_view->text),start_pos);
		/*
		gtk_text_thaw (GTK_TEXT (view->text));
		*/
		gtk_text_insert (GTK_TEXT (nth_view->text), NULL,
				 NULL, NULL, text_to_insert,
				 strlen(text_to_insert));
#ifdef ENABLE_SPLIT_SCREEN		
		gtk_text_freeze (GTK_TEXT (view->split_screen));
		p1 = gtk_text_get_point (GTK_TEXT (view->split_screen));
		gtk_text_set_point (GTK_TEXT(view->split_screen), undo->start_pos);
		gtk_text_thaw (GTK_TEXT (view->split_screen));
		gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
				 NULL, NULL, undo->text,
				 strlen(undo->text));
#endif		
	}  
}



void
dialog_replace (gint full)
{
	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *case_sensitive;
	GtkWidget *radio_button_1;
	GtkWidget *hbox_replace_with;
	GtkWidget *replace_button;
	GtkWidget *replace_all_button;
	GtkWidget *ask_before_replacing;
	
	GladeXML  *gui;

	gedit_debug("\n", DEBUG_SEARCH);
	
	if (replace_text_dialog!=NULL)
	{
		/* If the dialog is open and active, just show it
		   should we "raise" it instead for showing it ? maybe
		   chema */
		gtk_widget_show (replace_text_dialog);
		return;
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
	ask_before_replacing  = glade_xml_get_widget (gui, "ask_before_replacing");

	if (!replace_text_dialog ||
	    !search_entry ||
	    !replace_entry ||
	    !case_sensitive ||
	    !radio_button_1 ||
	    !hbox_replace_with ||
	    !replace_button ||
	    !replace_all_button ||
	    !ask_before_replacing)
	{
		g_warning ("Corrupted search.glade detected, reinstall gedit.");
		return;
	}

	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "search_for_text_entry", search_entry);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "replace_with_text_entry", replace_entry);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "case_sensitive", case_sensitive);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "radio_button_1", radio_button_1);

	search_start();
	
	gtk_signal_connect (GTK_OBJECT (replace_text_dialog), "clicked",
			    GTK_SIGNAL_FUNC (replace_text_clicked_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (replace_text_dialog), "destroy",
			    GTK_SIGNAL_FUNC (replace_text_destroyed_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (search_entry), "activate",
			    GTK_SIGNAL_FUNC (replace_entry_activate_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (replace_entry), "activate",
			    GTK_SIGNAL_FUNC (search_entry_activate_cb), replace_text_dialog);

	
	gnome_dialog_set_parent (GNOME_DIALOG (replace_text_dialog),
				 GTK_WINDOW (mdi->active_window));
	gtk_window_set_modal (GTK_WINDOW (replace_text_dialog), TRUE);
	gtk_widget_show_all (replace_text_dialog);

	if (!full)
	{
		gtk_widget_hide (hbox_replace_with);
		gtk_widget_hide (replace_button);
		gtk_widget_hide (replace_all_button);
		gtk_widget_hide (ask_before_replacing);
	}

	gtk_object_destroy (GTK_OBJECT (gui));
}


