/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Cleaned 10-00 by Chema */
/*
 * gedit 
 *
 * Copyright (C) 2000 Jose M Celorio
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
#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkeditable.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-stock.h>
#include <glade/glade-xml.h>

#include "window.h"
#include "search.h"
#include "view.h"
#include "utils.h"
#include "dialogs/dialogs.h"

typedef struct
{
	View *view;
	GladeXML *gui;
	GnomeDialog *dialog;
	gboolean not_found;
	gboolean replacements;
	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *find_button;
	GtkWidget *replace_button;
	GtkWidget *replace_all_button;
	GtkWidget *replace_hbox;
	GtkWidget *case_sensitive;
	GtkWidget *position;
}GeditReplaceDialog;

static GeditReplaceDialog *
get_dialog (gboolean full)
{
	GeditReplaceDialog *dialog;

	gedit_debug (DEBUG_SEARCH, "");

	dialog = g_new0 (GeditReplaceDialog, 1);
	dialog->gui = glade_xml_new (GEDIT_GLADEDIR "/replace.glade", NULL);
	if (!dialog->gui) {
		g_warning ("Could not find replace.glade.");
		return NULL;
	}

	dialog->dialog             = GNOME_DIALOG (glade_xml_get_widget (dialog->gui, "dialog"));
	dialog->search_entry       = glade_xml_get_widget (dialog->gui, "search_for_text_entry");
	dialog->replace_entry      = glade_xml_get_widget (dialog->gui, "replace_with_text_entry");
	dialog->find_button        = glade_xml_get_widget (dialog->gui, "find_next_button");
	dialog->replace_button     = glade_xml_get_widget (dialog->gui, "replace_button");
	dialog->replace_all_button = glade_xml_get_widget (dialog->gui, "replace_all_button");
	dialog->replace_hbox       = glade_xml_get_widget (dialog->gui, "hbox_replace_with");
	dialog->case_sensitive     = glade_xml_get_widget (dialog->gui, "case_sensitive");
	dialog->position           = glade_xml_get_widget (dialog->gui, "radio_button_1");

	g_return_val_if_fail (dialog->dialog, NULL);
	g_return_val_if_fail (dialog->search_entry, NULL);
	g_return_val_if_fail (dialog->replace_entry, NULL);
	g_return_val_if_fail (dialog->find_button, NULL);
	g_return_val_if_fail (dialog->replace_button, NULL);
	g_return_val_if_fail (dialog->replace_all_button, NULL);
	g_return_val_if_fail (dialog->replace_hbox, NULL);
	g_return_val_if_fail (dialog->case_sensitive, NULL);
	g_return_val_if_fail (dialog->position, NULL);

	/* Now, set the properties of the gnome_dialog */
	gnome_dialog_editable_enters (dialog->dialog, GTK_EDITABLE (dialog->search_entry));
	gnome_dialog_editable_enters (dialog->dialog, GTK_EDITABLE (dialog->replace_entry));
	gnome_dialog_close_hides     (dialog->dialog, TRUE);

	gtk_object_unref (GTK_OBJECT (dialog->gui));

	return dialog;
}

static void
dialog_set (GeditReplaceDialog *dialog, gboolean replace, gboolean first)
{
	GtkLabel *label;
	
	gedit_debug (DEBUG_SEARCH, "");

	label = GTK_LABEL(GTK_BIN ((GTK_BUTTON (dialog->find_button)))->child);
	g_return_if_fail (GTK_IS_LABEL (label));

	if (replace) {
		gtk_widget_show (dialog->replace_hbox);
		gtk_widget_show (dialog->replace_button);
		gtk_widget_show (dialog->replace_all_button);
		gtk_widget_set_sensitive (dialog->replace_button, !first);
		gtk_window_set_title ( GTK_WINDOW (dialog->dialog), _("Replace"));
	} else {
		gtk_widget_hide (dialog->replace_hbox);
		gtk_widget_hide (dialog->replace_button);
		gtk_widget_hide (dialog->replace_all_button);
		gtk_window_set_title ( GTK_WINDOW (dialog->dialog), _("Find"));
	}

	if (first) {
		dialog->not_found = FALSE;
		dialog->replacements = -1;
		dialog->view = gedit_view_active ();
		
		gtk_label_set_text (label, "Find");
		
		gtk_widget_grab_focus (dialog->search_entry);

		gtk_entry_set_position (GTK_ENTRY (dialog->replace_entry), 0);
		gtk_entry_select_region (GTK_ENTRY (dialog->replace_entry), 0,
					 GTK_ENTRY (dialog->replace_entry)->text_length);
		
		gtk_entry_set_position (GTK_ENTRY (dialog->search_entry), 0);
		gtk_entry_select_region (GTK_ENTRY (dialog->search_entry), 0,
					 GTK_ENTRY (dialog->search_entry)->text_length);
		gtk_radio_button_select (GTK_RADIO_BUTTON(dialog->position)->group, 0);
		gnome_dialog_set_parent  (dialog->dialog,
					  gedit_window_active ());
	} else {
		gtk_entry_select_region (GTK_ENTRY (dialog->replace_entry), 0, 0);
		gtk_entry_select_region (GTK_ENTRY (dialog->search_entry), 0, 0);
		gtk_label_set_text (label, "Find Next");
		gtk_radio_button_select (GTK_RADIO_BUTTON(dialog->position)->group, 1);
	}

	gnome_dialog_set_default (dialog->dialog, replace ? (first ? 0 : 1) : 0);
	
}


static void
action_find (GeditReplaceDialog *dialog,
	     guint start_pos,
	     const gchar *search_text,
	     gboolean case_sensitive)
{
	gboolean found = FALSE;
	guint pos_found;
	gint line_found, total_lines;

	found = search_text_execute (start_pos, case_sensitive, search_text,
				     &pos_found, &line_found, &total_lines, TRUE);

	if (!found) {
		dialog->not_found = TRUE;
		return;
	}

	gedit_flash_va (_("Text found at line :%i"),line_found);
	gedit_view_set_window_position_from_lines (dialog->view, line_found, total_lines);
	gedit_view_set_position	(dialog->view, pos_found);
	gedit_view_set_selection (dialog->view,
				  pos_found + 1,	
				  pos_found + 1 + strlen (search_text));
	
	gtk_radio_button_select (GTK_RADIO_BUTTON(dialog->position)->group, 1);
}

static void
action_replace (GeditReplaceDialog *dialog,
		guint start_pos,
		const gchar *search_text,
		const gchar *replace_text,
		gboolean case_sensitive)
{

	gedit_document_replace_text (dialog->view->doc,
				     replace_text,
				     strlen (search_text),
				     start_pos - 1,
				     TRUE);

	/* We need to reload the buffer since we changed it */
	gedit_search_end();
	gedit_search_start();

	start_pos += strlen (replace_text) - strlen (search_text);
	
	action_find (dialog, start_pos, search_text, case_sensitive);

}

static void
action_replace_all (GeditReplaceDialog *dialog,
		    guint start_pos,
		    const gchar *search_text,
		    const gchar *replace_text,
		    gboolean case_sensitive)
{
	guchar *new_buffer = NULL;

	dialog->replacements = gedit_search_replace_all_execute (dialog->view,
								 start_pos,
								 search_text,
								 replace_text,
								 case_sensitive,
								 &new_buffer);
	
	if (dialog->replacements > 0)
	{
		gedit_document_delete_text (dialog->view->doc, 0,
					    gedit_document_get_buffer_length(dialog->view->doc),
					    TRUE);
		gedit_document_insert_text (dialog->view->doc, new_buffer, 0, TRUE);
		/* FIXME : use replace, for undo . !*/ 
		/*
		  gedit_document_replace_text (view->doc, new_buffer, 0, gedit_document_get_buffer_length(view->doc), TRUE);
		*/
	}

	if (new_buffer!=NULL)
		g_free (new_buffer);
		
}

static void
dialog_action (GeditReplaceDialog *dialog, gint button, gboolean replace)
{
	const gchar *search_text;
	const gchar *replace_text;
	gboolean case_sensitive;
	guint start_pos;
	guint end_pos;
	gint selected;
	
	gedit_debug (DEBUG_SEARCH, "");

	g_return_if_fail (search_verify_document ());

	search_text  = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));
	replace_text = gtk_entry_get_text (GTK_ENTRY (dialog->replace_entry));

	g_return_if_fail (search_text != NULL);
	g_return_if_fail (replace_text != NULL);
	
	if (*search_text == 0)
		return;
	
	/* Get the initial position */
	selected = gtk_radio_group_get_selected (GTK_RADIO_BUTTON(dialog->position)->group);
	switch (selected) {
	case 0:
		start_pos = 0;
		break;
	case 1:
		if (gedit_view_get_selection (dialog->view, &start_pos, &end_pos))
			start_pos = end_pos;
		else
			start_pos = gedit_view_get_position (dialog->view);
		break;
	default:
		g_warning ("Invalid radio button selection.");
		return;
	}

	case_sensitive = GTK_TOGGLE_BUTTON (dialog->case_sensitive)->active;
	switch (button) {
	case 2:
		action_replace_all (dialog, start_pos,
				    search_text, replace_text, case_sensitive);
		break;
	case 1:
		action_replace (dialog, start_pos,
				search_text, replace_text, case_sensitive);
		break;
	case 0:
		action_find (dialog, start_pos,
			     search_text, case_sensitive);
		break;
	}
}

/**
 * gedit_dialog_replace:
 * @replace: If true, it pops up a replace dialog. If false, it pops up a Find dialog
 * 
 * Takes care of creating and actions of the replace dialog, 
 **/
void
gedit_dialog_replace (gboolean replace)
{
	static GeditReplaceDialog *dialog = NULL;
	gint button;

	gedit_debug (DEBUG_SEARCH, "");

	if (dialog == NULL) {
		dialog = get_dialog (replace);
		if (dialog == NULL)
			return;
	}

	dialog_set (dialog, replace, TRUE);
	if (dialog->view == NULL)
		return;
	
	while (!dialog->not_found &&
	       (dialog->replacements < 0)) {

		button = gnome_dialog_run (dialog->dialog);
		if (button == -1)
			return;
		if (button == 3)
			break;
		
		gedit_search_start ();
		dialog_action (dialog, button, replace);
		gedit_search_end ();

		if (button == 2)
			break;

		dialog_set (dialog, replace, FALSE);
	}

	gnome_dialog_close (dialog->dialog);

	/* This is done like this, since we need to close the other
	   dialog before poping this dialogs*/
	if (dialog->not_found || dialog->replacements == 0)
		search_text_not_found_notify (dialog->view);

	if (dialog->replacements > 0) {
		gchar *msg;
		msg = g_strdup_printf (_("found and replaced %i occurrences."),
				       dialog->replacements);
		gnome_dialog_run_and_close ((GnomeDialog *)
					    gnome_message_box_new (msg,
								   GNOME_MESSAGE_BOX_INFO,
								   GNOME_STOCK_BUTTON_OK,
								   NULL));
		g_free (msg);
	}


	return;
}
