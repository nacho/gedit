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

typedef struct _GeditDialogReplace GeditDialogReplace;

struct _GeditDialogReplace {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *replace_hbox;
	GtkWidget *case_sensitive;
	GtkWidget *position;
};

static void find_button_pressed (GeditDialogReplace * dialog);
static void help_button_pressed (GeditDialogReplace * dialog);

static GeditDialogReplace *dialog_replace_get_dialog (gboolean replace);

static GeditDialogReplace *
dialog_replace_get_dialog (gboolean replace)
{
	static GeditDialogReplace *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	if (dialog != NULL)
	{
		if (replace)
			gtk_widget_show (dialog->replace_hbox);
		else
			gtk_widget_hide (dialog->replace_hbox);

		return dialog;
	}

	/* FIXME */
	gui = glade_xml_new ( /*GEDIT_GLADEDIR */ "./dialogs/replace.glade2",
			     "replace_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find replace.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogReplace, 1);

	dialog->dialog = gtk_dialog_new_with_buttons ("Find",
						      window,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_FIND,
						      GTK_RESPONSE_OK,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content = glade_xml_get_widget (gui, "replace_dialog_content");
	dialog->search_entry       = glade_xml_get_widget (gui, "search_for_text_entry");
	dialog->replace_entry      = glade_xml_get_widget (gui, "replace_with_text_entry");
	dialog->replace_hbox       = glade_xml_get_widget (gui, "hbox_replace_with");
	dialog->case_sensitive     = glade_xml_get_widget (gui, "case_sensitive");
	dialog->position           = glade_xml_get_widget (gui, "radio_button_1");


	if (!content			||
	    !dialog->search_entry 	||
	    !dialog->replace_entry  	|| 
	    !dialog->replace_hbox 	||
	    !dialog->case_sensitive 	||
	    !dialog->position 	) 
	{
		g_print
		    ("Could not find the required widgets inside replace.glade2.\n");
		return NULL;
	}

	if (replace)
		gtk_widget_show (dialog->replace_hbox);
	else
		gtk_widget_hide (dialog->replace_hbox);



	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_object_unref (gui);
	
	return dialog;
}


void
gedit_dialog_find (void)
{
	GeditDialogReplace *dialog;
	gint response;

	dialog = dialog_replace_get_dialog (FALSE);
	if (dialog == NULL) {
		g_warning ("Could not create the Find dialog");
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	gtk_widget_grab_focus (dialog->search_entry);

	do
	{
		response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (response) {
			case GTK_RESPONSE_OK:
				find_button_pressed (dialog);
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
find_button_pressed (GeditDialogReplace *dialog)
{
	/* FIXME */
	gtk_widget_hide (dialog->dialog);
}

static void
help_button_pressed (GeditDialogReplace * dialog)
{
	/* FIXME */
}
