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

GtkWidget *replace_text_dialog;

static void replace_text_clicked_cb (GtkWidget *widget, gint button);

static void
replace_text_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug("\n", DEBUG_SEARCH);
	search_end();
	replace_text_dialog = NULL;
}


static void
replace_text_entry_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	/* behave as if the user clicked Find/Find next button */
	gedit_debug("\n", DEBUG_SEARCH);
	replace_text_clicked_cb (dialog, 0);
}

static void
replace_text_clicked_cb (GtkWidget *widget, gint button)
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

	if (button == 3) /* Close */
		gnome_dialog_close (GNOME_DIALOG (widget));
	
	if (button == 2) /* Replace All */
		g_print("Replacing All ....\n");
	 
	if (button == 1) /* Replace */
		g_print("Replacing ....\n");
	
	if (button == 0) /* Find Next */
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
			search_text_not_found_notify (text);
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
dialog_replace (void)
{
	GtkWidget *text_entry;
	GtkWidget *case_sensitive;
	GtkWidget *radio_button_1;
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

	replace_text_dialog = glade_xml_get_widget (gui, "dialog");
	text_entry     = glade_xml_get_widget (gui, "text_entry");
	case_sensitive = glade_xml_get_widget (gui, "case_sensitive");
	radio_button_1 = glade_xml_get_widget (gui, "radio_button_1");
	
	if (!replace_text_dialog || !text_entry || !case_sensitive || !radio_button_1)
	{
		g_warning ("Corrupted search.glade detected, reinstall gedit.");
		return;
	}

	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "text_entry", text_entry);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "case_sensitive", case_sensitive);
	gtk_object_set_data (GTK_OBJECT (replace_text_dialog), "radio_button_1", radio_button_1);

	search_start();
	
	gtk_signal_connect (GTK_OBJECT (replace_text_dialog), "clicked",
			    GTK_SIGNAL_FUNC (replace_text_clicked_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (replace_text_dialog), "destroy",
			    GTK_SIGNAL_FUNC (replace_text_destroyed_cb), replace_text_dialog);
	gtk_signal_connect (GTK_OBJECT (text_entry), "activate",
			    GTK_SIGNAL_FUNC (replace_text_entry_activate_cb), replace_text_dialog);

	gnome_dialog_set_parent (GNOME_DIALOG (replace_text_dialog),
				 GTK_WINDOW (mdi->active_window));
	gtk_window_set_modal (GTK_WINDOW (replace_text_dialog), TRUE);
	gtk_widget_show_all (replace_text_dialog);
	gtk_object_destroy (GTK_OBJECT (gui));
}







/*
 * dialog-replace.c: Dialog for replacing text in gedit.
 *
 * Author:
 *  Jason Leach <leach@wam.umd.edu>
 *
 */

/*
 * TODO:
 * [ ] libglade-ify me
 */

#if 0

#include <config.h>
#include <gnome.h>

#include "view.h"
#include "document.h"
#include "search.h"
#include "gedit.h"

static GtkWidget *replace_dialog;

       void dialog_replace (void);
static void clicked_cb (GtkWidget *widget, gint button, Document *doc);
static gint ask_replace (void);
GtkWidget*  create_replace_dialog (void);
       void dialog_replace_impl (void);

void
dialog_replace (void)
{
	Document *doc;

	doc = gedit_document_current ();

	if (!replace_dialog)
		replace_dialog = create_replace_dialog ();

	gtk_signal_connect (GTK_OBJECT (replace_dialog), "clicked",
			    GTK_SIGNAL_FUNC (clicked_cb), doc);
	gtk_widget_show (replace_dialog);
}

/* Callback on the dialog's "clicked" signal */
static void
clicked_cb (GtkWidget *widget, gint button, Document *doc)
{
	GtkWidget *datalink;
	gint pos, dowhat = 0;
	gulong options;
	gchar *str, *replace;
	gboolean confirm = FALSE;

#if 0
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
			if (confirm)
			{
				search_select (doc, str, pos, options);
				dowhat = ask_replace();
				if (dowhat == 2) 
					break;
				if (dowhat == 1)
				{
					if (!(options | SEARCH_BACKWARDS))
						pos += num_widechars (str);

					continue;
				}
			}

			gedit_search_replace (doc, pos, num_widechars (str),
					      replace);
			if (!(options | SEARCH_BACKWARDS))
				pos += num_widechars (replace);

		} while (button); /* FIXME: this is so busted, its an
                                     infinite loop */
	}
	else
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
					       GTK_SIGNAL_FUNC (clicked_cb),
					       doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
#endif	
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

GtkWidget *
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


void
dialog_replace_impl (void)
{

}

#endif
