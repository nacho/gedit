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

#include <glade/glade-xml.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-utils.h"
#include "gedit-dialogs.h"
#include "gedit-document.h"
#include "gedit-view.h"

typedef struct _GeditDialogGotoLine GeditDialogGotoLine;

struct _GeditDialogGotoLine {
	GtkWidget *dialog;

	GtkWidget *entry;
};

static void goto_button_pressed (GeditDialogGotoLine * dialog);
static GeditDialogGotoLine *dialog_goto_line_get_dialog (void);

static GeditDialogGotoLine *
dialog_goto_line_get_dialog (void)
{
	static GeditDialogGotoLine *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	if (dialog != NULL)
		return dialog;

	gui = glade_xml_new (GEDIT_GLADEDIR "goto-line.glade2",
			     "goto_line_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find goto-line.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogGotoLine, 1);

	dialog->dialog = gtk_dialog_new_with_buttons ("Goto line...",
						      window,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_OK,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content = glade_xml_get_widget (gui, "goto_line_dialog_content");

	dialog->entry = glade_xml_get_widget (gui, "entry");

	if (!dialog->entry) {
		g_print
		    ("Could not find the required widgets inside goto-line.glade2.\n");
		return NULL;
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_object_unref (gui);
	
	return dialog;
}


void
gedit_dialog_goto_line (void)
{
	GeditDialogGotoLine *dialog;
	gint response;

	dialog = dialog_goto_line_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Goto Line dialog");
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	gtk_widget_grab_focus (dialog->entry);

	response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

	switch (response) {
		case GTK_RESPONSE_OK:
			goto_button_pressed (dialog);
			break;

		default:
			gtk_widget_hide (dialog->dialog);
	}
}

static void
goto_button_pressed (GeditDialogGotoLine *dialog)
{
	gchar* text;
	GeditView* active_view;
	GeditDocument* active_document;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);

	active_document = gedit_view_get_document (active_view);
	g_return_if_fail (active_document);

	text = gtk_entry_get_text (GTK_ENTRY (dialog->entry));
	
	if (text != NULL && text [0] != 0)
	{
		guint line = MAX (atoi (text), 0);		
		gedit_document_goto_line (active_document, line);
		gedit_view_scroll_to_cursor (active_view);
	}

	gtk_widget_hide (dialog->dialog);
}

