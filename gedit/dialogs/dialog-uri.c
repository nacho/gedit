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


static GtkWidget *open_uri_dialog = NULL;

static GtkWidget *open_button = NULL;
static GtkWidget *cancel_button = NULL;
static GtkWidget *help_button = NULL;

static GtkWidget *uri = NULL;
static GtkWidget *uri_list = NULL;

static void open_button_pressed (GtkWidget *widget, gpointer data);
static void cancel_button_pressed (GtkWidget *widget, gpointer data);
static void help_button_pressed (GtkWidget *widget, gpointer data);

static gboolean
dialog_open_uri_get_dialog (void)
{
	GladeXML *gui;

	gui = glade_xml_new (GEDIT_GLADEDIR "/uri.glade", "open_uri_dialog");

	if (!gui) {
		g_warning ("Could not find uri.glade, reinstall gedit.\n");
		return FALSE;
	}

	open_uri_dialog = glade_xml_get_widget (gui, "open_uri_dialog");
	open_button = glade_xml_get_widget (gui, "open_button");
	cancel_button = glade_xml_get_widget (gui, "cancel_button");
	help_button = glade_xml_get_widget (gui, "help_button");

	uri = glade_xml_get_widget (gui, "uri");
	uri_list = glade_xml_get_widget (gui, "uri_list");

	
	if (!open_uri_dialog ||
	    !open_button || !cancel_button || !help_button ||
	    !uri || !uri_list){
		g_print ("Could not find the required widgets inside uri.glade.\n");
		return FALSE;
	}
	gtk_object_unref (GTK_OBJECT (gui));

	/*
	gnome_entry_load_history (GNOME_ENTRY (uri_list));
	*/
	
	gtk_signal_connect (GTK_OBJECT (open_button), "clicked",
			    GTK_SIGNAL_FUNC (open_button_pressed), NULL);

	gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (cancel_button_pressed), NULL);

	gtk_signal_connect (GTK_OBJECT (help_button), "clicked",
			    GTK_SIGNAL_FUNC (help_button_pressed), NULL);

	gtk_window_set_modal         (GTK_WINDOW (open_uri_dialog), TRUE);
	gnome_dialog_set_default     (GNOME_DIALOG (open_uri_dialog), 0);
	gnome_dialog_editable_enters (GNOME_DIALOG (open_uri_dialog), GTK_EDITABLE (uri));
	gnome_dialog_close_hides     (GNOME_DIALOG (open_uri_dialog), TRUE);

	return TRUE;
}

void
gedit_dialog_open_uri (void)
{
	if (open_uri_dialog == NULL)
	{
		if (!dialog_open_uri_get_dialog ())
		{
			return;
		}
	}

	/* re-parent the dialog */
	gnome_dialog_set_parent (GNOME_DIALOG (open_uri_dialog),
				 GTK_WINDOW (gedit_window_active_app()));

	/* set the entry text to "" */
	gtk_entry_set_text(GTK_ENTRY(uri), "");

	gtk_widget_show (open_uri_dialog);
	
}

static void
open_button_pressed (GtkWidget *widget, gpointer data)
{
	gchar * file_name = NULL;

	file_name = gtk_editable_get_chars (GTK_EDITABLE (uri), 0, -1);

	if((file_name != NULL) && (strlen(file_name) != 0))
	{
		gtk_widget_hide(open_uri_dialog);

		if (gedit_document_new_with_file (file_name))
		{
			gnome_entry_append_history ( GNOME_ENTRY (uri_list), FALSE, file_name);	

			gedit_flash_va (_("Loaded file %s"), file_name);
			
			if (gedit_document_current())
			{
				g_assert(gedit_window_active_app());
				gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), 
					gedit_document_current()->readonly);
			}				
		}
	}
	else
	{
		gtk_widget_hide(open_uri_dialog);
	}

	g_free (file_name);
}

static void
cancel_button_pressed (GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(open_uri_dialog);
}

static void
help_button_pressed (GtkWidget *widget, gpointer data)
{
	/* FIXME: Paolo - change to point to the right help page */

	static GnomeHelpMenuEntry help_entry = { "gedit", "index.html" };

	gnome_help_display (NULL, &help_entry);
}




