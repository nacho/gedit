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

#define GEDIT_RESPONSE_FIND		101
#define GEDIT_RESPONSE_REPLACE		102
#define GEDIT_RESPONSE_REPLACE_ALL	103

typedef struct _GeditDialogReplace GeditDialogReplace;
typedef struct _GeditDialogFind GeditDialogFind;

struct _GeditDialogReplace {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *replace_entry;
	GtkWidget *replace_hbox;
	GtkWidget *case_sensitive;
	GtkWidget *position;
};

struct _GeditDialogFind {
	GtkWidget *dialog;

	GtkWidget *search_entry;
	GtkWidget *case_sensitive;
	GtkWidget *position;
};

static void find_dlg_find_button_pressed (GeditDialogFind * dialog);

static void replace_dlg_find_button_pressed (GeditDialogReplace * dialog);
static void replace_dlg_replace_button_pressed (GeditDialogReplace * dialog);
static void replace_dlg_replace_all_button_pressed (GeditDialogReplace * dialog);

static GeditDialogReplace *dialog_replace_get_dialog 	(void);
static GeditDialogFind    *dialog_find_get_dialog 	(void);

static GeditDialogReplace *
dialog_replace_get_dialog (void)
{
	static GeditDialogReplace *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	if (dialog != NULL)
	{
		return dialog;
	}

	gui = glade_xml_new ( GEDIT_GLADEDIR "replace.glade2",
			     "replace_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find replace.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogReplace, 1);

	dialog->dialog = gtk_dialog_new_with_buttons ("Replace",
						      window,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,						      
						      "Replace _All",
						      GEDIT_RESPONSE_REPLACE_ALL,
						      "_Replace",
						      GEDIT_RESPONSE_REPLACE,
						      GTK_STOCK_FIND,
						      GEDIT_RESPONSE_FIND,
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

	gtk_widget_show (dialog->replace_hbox);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_FIND);

	g_object_unref (G_OBJECT (gui));
	
	return dialog;
}


static GeditDialogFind *
dialog_find_get_dialog (void)
{
	static GeditDialogFind *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *replace_hbox;
	
	if (dialog != NULL)
	{
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "replace.glade2",
			     "replace_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find replace.glade2, reinstall gedit.\n");
		return NULL;
	}

	window = GTK_WINDOW (bonobo_mdi_get_active_window
			     (BONOBO_MDI (gedit_mdi)));

	dialog = g_new0 (GeditDialogFind, 1);

	dialog->dialog = gtk_dialog_new_with_buttons ("Find",
						      window,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      GTK_STOCK_FIND,
						      GEDIT_RESPONSE_FIND,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content = glade_xml_get_widget (gui, "replace_dialog_content");
	dialog->search_entry       = glade_xml_get_widget (gui, "search_for_text_entry");
	
	replace_hbox       	   = glade_xml_get_widget (gui, "hbox_replace_with");
	
	dialog->case_sensitive     = glade_xml_get_widget (gui, "case_sensitive");
	dialog->position           = glade_xml_get_widget (gui, "radio_button_1");


	if (!content			||
	    !dialog->search_entry 	||
	    !replace_hbox 		||
	    !dialog->case_sensitive 	||
	    !dialog->position 	) 
	{
		g_print
		    ("Could not find the required widgets inside replace.glade2.\n");
		return NULL;
	}

	gtk_widget_hide (replace_hbox);


	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GEDIT_RESPONSE_FIND);

	g_object_unref (G_OBJECT (gui));
	
	return dialog;
}

void
gedit_dialog_find (void)
{
	GeditDialogFind *dialog;
	gint response;

	dialog = dialog_find_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Find dialog");
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	gtk_widget_grab_focus (dialog->search_entry);

	response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

	switch (response) {
		case GEDIT_RESPONSE_FIND:
			find_dlg_find_button_pressed (dialog);
			break;

		default:
			gtk_widget_hide (dialog->dialog);
	}
}

void
gedit_dialog_replace (void)
{
	GeditDialogReplace *dialog;
	gint response;

	dialog = dialog_replace_get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Replace dialog");
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	gtk_widget_grab_focus (dialog->search_entry);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GEDIT_RESPONSE_REPLACE, FALSE);
	
	do
	{
		response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (response) {
			case GEDIT_RESPONSE_FIND:
				replace_dlg_find_button_pressed (dialog);
				break;

			case GEDIT_RESPONSE_REPLACE:
				replace_dlg_replace_button_pressed (dialog);
				break;

			case GEDIT_RESPONSE_REPLACE_ALL:
				replace_dlg_replace_all_button_pressed (dialog);
				break;

			default:
				gtk_widget_hide (dialog->dialog);
		}
				
	} while (GTK_WIDGET_VISIBLE (dialog->dialog));
}

static void
find_dlg_find_button_pressed (GeditDialogFind *dialog)
{
	/* FIXME */
	gtk_widget_hide (dialog->dialog);
}


static void
replace_dlg_find_button_pressed (GeditDialogReplace *dialog)
{
	/* FIXME */
}

static void
replace_dlg_replace_button_pressed (GeditDialogReplace *dialog)
{
	/* FIXME */
}

static void
replace_dlg_replace_all_button_pressed (GeditDialogReplace *dialog)
{
	/* FIXME */
	gtk_widget_hide (dialog->dialog);
}

