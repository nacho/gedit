/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-uri.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glade/glade-xml.h>
#include <libgnome/gnome-help.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-utils.h"
#include "gedit-file.h"
#include "gedit-dialogs.h"

typedef struct _GeditDialogOpenUri GeditDialogOpenUri;

struct _GeditDialogOpenUri {
	GtkWidget *dialog;

	GtkWidget *uri;
	GtkWidget *uri_list;
};

static void open_button_pressed (GeditDialogOpenUri * dialog);
static void help_button_pressed (GeditDialogOpenUri * dialog);
static GeditDialogOpenUri *dialog_open_uri_get_dialog (void);
static void dialog_open_uri_list_activate (GtkWidget *uri_list, 
		GeditDialogOpenUri *dlg);

static void
dialog_open_uri_list_activate (GtkWidget *uri_list, GeditDialogOpenUri *dlg)
{
	g_return_if_fail (dlg != NULL);
	gtk_dialog_response (GTK_DIALOG (dlg->dialog), GTK_RESPONSE_OK);
}

static GeditDialogOpenUri *
dialog_open_uri_get_dialog (void)
{
	static GeditDialogOpenUri *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	if (dialog != NULL)
		return dialog;

	gui = glade_xml_new (GEDIT_GLADEDIR "uri.glade2",
			     "open_uri_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find uri.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogOpenUri, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Open from URI"),
						      window,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OPEN,
						      GTK_RESPONSE_OK,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content = glade_xml_get_widget (gui, "open_uri_dialog_content");

	dialog->uri = glade_xml_get_widget (gui, "uri");
	dialog->uri_list = glade_xml_get_widget (gui, "uri_list");

	if (!dialog->uri || !dialog->uri_list) {
		g_print
		    ("Could not find the required widgets inside uri.glade.\n");
		return NULL;
	}

	g_signal_connect (G_OBJECT (dialog->uri_list), "activate", 
			G_CALLBACK (dialog_open_uri_list_activate), dialog);

	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;
}


void
gedit_dialog_open_uri (void)
{
	GeditDialogOpenUri *dialog;
	gint response;

	dialog = dialog_open_uri_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Open URI dialog");
		return;
	}

	gtk_widget_grab_focus (dialog->uri);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	gtk_entry_set_text (GTK_ENTRY (dialog->uri), "");

	do {
		response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (response) {
		case GTK_RESPONSE_OK:
			open_button_pressed (dialog);
			break;

		case GTK_RESPONSE_HELP:
			help_button_pressed (dialog);
			break;

		default:
			gtk_widget_hide (dialog->dialog);
		}

	} while (response == GTK_RESPONSE_HELP);
}

static void
open_button_pressed (GeditDialogOpenUri * dialog)
{
	gchar *file_name = NULL;

	g_return_if_fail (dialog != NULL);

	file_name =
	    gtk_editable_get_chars (GTK_EDITABLE (dialog->uri), 0, -1);

	gtk_widget_hide (dialog->dialog);

	gedit_file_open_single_uri (file_name);

	g_free (file_name);
}

static void
help_button_pressed (GeditDialogOpenUri * dialog)
{
	GError *error = NULL;

	gnome_help_display ("gedit.xml", "gedit-open-from-uri", &error);
	
	if (error != NULL)
	{
		g_warning (error->message);

		g_error_free (error);
	}

}
