/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author :
 *    Paolo Maggi <maggi@athena.polito.it>
 * 
 */


#include <config.h>
#include <stdlib.h>
#include <gnome.h>
#include <glade/glade-xml.h>

#include "view.h"
#include "utils.h"
#include "window.h"
#include "dialogs/dialogs.h"

typedef struct _GeditDialogOpenUri GeditDialogOpenUri;

struct _GeditDialogOpenUri {
	GtkWidget *dialog;

	GtkWidget *open_button;
	GtkWidget *cancel_button;
	GtkWidget *help_button;

	GtkWidget *uri;
	GtkWidget *uri_list;
};

static void open_button_pressed   (GtkWidget *widget, GeditDialogOpenUri *dialog);
static void cancel_button_pressed (GtkWidget *widget, GeditDialogOpenUri *dialog);
static void help_button_pressed   (GtkWidget *widget, GeditDialogOpenUri *dialog);

static GeditDialogOpenUri *
dialog_open_uri_get_dialog (void)
{
	static GeditDialogOpenUri *dialog = NULL;
	GladeXML *gui;

	if (dialog != NULL)
		return dialog;
	
	gui = glade_xml_new (GEDIT_GLADEDIR "/uri.glade", "open_uri_dialog");

	if (!gui) {
		g_warning ("Could not find uri.glade, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (GeditDialogOpenUri, 1);

	dialog->dialog        = glade_xml_get_widget (gui, "open_uri_dialog");
	dialog->open_button   = glade_xml_get_widget (gui, "open_button");
	dialog->cancel_button = glade_xml_get_widget (gui, "cancel_button");
	dialog->help_button   = glade_xml_get_widget (gui, "help_button");
	dialog->uri           = glade_xml_get_widget (gui, "uri");
	dialog->uri_list      = glade_xml_get_widget (gui, "uri_list");

	
	if (!dialog->dialog ||
	    !dialog->open_button || !dialog->cancel_button || !dialog->help_button ||
	    !dialog->uri || !dialog->uri_list){
		g_print ("Could not find the required widgets inside uri.glade.\n");
		return NULL;
	}

	/*
	gnome_entry_load_history (GNOME_ENTRY (uri_list));
	*/
	
	gtk_signal_connect (GTK_OBJECT (dialog->open_button), "clicked",
			    GTK_SIGNAL_FUNC (open_button_pressed), dialog);

	gtk_signal_connect (GTK_OBJECT (dialog->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (cancel_button_pressed), dialog);

	gtk_signal_connect (GTK_OBJECT (dialog->help_button), "clicked",
			    GTK_SIGNAL_FUNC (help_button_pressed), dialog);

	gtk_window_set_modal         (GTK_WINDOW (dialog->dialog), TRUE);
	gnome_dialog_set_default     (GNOME_DIALOG (dialog->dialog), 0);
	gnome_dialog_editable_enters (GNOME_DIALOG (dialog->dialog), GTK_EDITABLE (dialog->uri));
	gnome_dialog_close_hides     (GNOME_DIALOG (dialog->dialog), TRUE);

	return dialog;
}


void
gedit_dialog_open_uri (void)
{
	GeditDialogOpenUri *dialog;
	
	dialog = dialog_open_uri_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Open URI dialog");
		return;
	}

	gnome_dialog_set_parent (GNOME_DIALOG (dialog->dialog),
				 GTK_WINDOW (gedit_window_active_app()));
	gtk_entry_set_text (GTK_ENTRY(dialog->uri), "");
	
	gtk_widget_show (dialog->dialog);
	
}


static void
open_button_pressed (GtkWidget *widget, GeditDialogOpenUri *dialog)
{
	gchar * file_name = NULL;

	g_return_if_fail (dialog != NULL);
	
	file_name = gtk_editable_get_chars (GTK_EDITABLE (dialog->uri), 0, -1);
	gtk_widget_hide (dialog->dialog);
	
	if (gedit_document_new_with_file (file_name))
	{
		gnome_entry_append_history (GNOME_ENTRY (dialog->uri_list),
					    FALSE, file_name);	
		gedit_flash_va (_("Loaded file %s"), file_name);
		gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), 
							 gedit_document_current()->readonly);
	}

	g_free (file_name);
}

static void
cancel_button_pressed (GtkWidget *widget, GeditDialogOpenUri *dialog)
{
	gtk_widget_hide (dialog->dialog);
}

static void
help_button_pressed (GtkWidget *widget, GeditDialogOpenUri *dialog)
{
	/* FIXME: change to point to the right help page. Paolo */
	static GnomeHelpMenuEntry help_entry = { "gedit", "index.html" };

	gnome_help_display (NULL, &help_entry);
}




