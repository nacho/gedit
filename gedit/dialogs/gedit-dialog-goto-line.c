/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-goto-line.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-utils.h"
#include "gedit-dialogs.h"
#include "gedit-document.h"
#include "gedit-view.h"
#include "gedit-debug.h"

typedef struct _GeditDialogGotoLine GeditDialogGotoLine;

struct _GeditDialogGotoLine {
	GtkWidget *dialog;

	GtkWidget *entry;
};

static void goto_button_pressed (GeditDialogGotoLine * dialog);
static GeditDialogGotoLine *dialog_goto_line_get_dialog (void);
static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_SEARCH, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogGotoLine *dialog)
{
	gedit_debug (DEBUG_SEARCH, "");

	switch (res_id) {
		case GTK_RESPONSE_OK:
			goto_button_pressed (dialog);
			break;
			
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}

static void
entry_insert_text (GtkEditable *editable, const char *text, gint length, gint *position)
{
	gunichar c;
	const gchar *p;
 	const gchar *end;

	p = text;
	end = text + length;

	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);

		c = g_utf8_get_char (p);

		if (!g_unichar_isdigit (c)) {
			g_signal_stop_emission_by_name (editable, "insert_text");
			break;
		}

		p = next;
	}
}

static void 
entry_changed (GtkEditable *editable, GeditDialogGotoLine *dialog)
{
	const gchar *line_string;
	
	line_string = gtk_entry_get_text (GTK_ENTRY (dialog->entry));		
	g_return_if_fail (line_string != NULL);

	if (strlen (line_string) <= 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GTK_RESPONSE_OK, FALSE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
			GTK_RESPONSE_OK, TRUE);
}

static GeditDialogGotoLine *
dialog_goto_line_get_dialog (void)
{
	static GeditDialogGotoLine *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	gedit_debug (DEBUG_SEARCH, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      GTK_WINDOW (window));
		gtk_window_present (GTK_WINDOW (dialog->dialog));

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "goto-line.glade2",
			     "goto_line_dialog_content", NULL);
	if (!gui)
	{
		gedit_warning (window,
			       MISSING_FILE,
			       GEDIT_GLADEDIR "goto-line.glade2");
		return NULL;
	}

	dialog = g_new0 (GeditDialogGotoLine, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Go to Line"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog->dialog), FALSE);

	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog),
				 _("_Go to Line"), GTK_STOCK_JUMP_TO,
				 GTK_RESPONSE_OK);

	content = glade_xml_get_widget (gui, "goto_line_dialog_content");

	dialog->entry = glade_xml_get_widget (gui, "entry");

	if (!dialog->entry)
	{
		gedit_warning (window,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "goto-line.glade2");
		g_object_unref (gui);
		g_free (dialog);
		return NULL;
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->dialog), 
					   GTK_RESPONSE_OK, FALSE);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->entry), "insert_text",
			  G_CALLBACK (entry_insert_text), NULL);

	g_signal_connect (G_OBJECT (dialog->entry), "changed",
			  G_CALLBACK (entry_changed), dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			  G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	return dialog;
}

void
gedit_dialog_goto_line (void)
{
	GeditDialogGotoLine *dialog;

	gedit_debug (DEBUG_SEARCH, "");

	dialog = dialog_goto_line_get_dialog ();
	if (!dialog)
		return;

	gtk_widget_grab_focus (dialog->entry);

	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show (dialog->dialog);
}

static void
goto_button_pressed (GeditDialogGotoLine *dialog)
{
	const gchar* text;
	GeditView* active_view;
	GeditDocument* active_document;

	gedit_debug (DEBUG_SEARCH, "");

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);

	active_document = gedit_view_get_document (active_view);
	g_return_if_fail (active_document);

	text = gtk_entry_get_text (GTK_ENTRY (dialog->entry));
	
	if (text != NULL && text [0] != 0)
	{
		guint line = MAX (atoi (text)- 1, 0);		
		gedit_document_goto_line (active_document, line);
		gedit_view_scroll_to_cursor (active_view);
		gtk_widget_grab_focus (GTK_WIDGET (active_view));
	}
}

