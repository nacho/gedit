/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-viewer.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 Jeroen Zwartepoorte
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo/bonobo-ui-util.h>
#include <gconf/gconf-client.h>
#include <gtksourceview.h>
#include <gtksourcelanguagesmanager.h>
#include "gedit-persist-stream.h"
#include "gedit-viewer.h"
#include "gedit-prefs-manager.h"

static void
copy_cb (BonoboUIComponent *component,
	 gpointer callback_data,
	 const char *verb)
{
	GtkWidget *source_view;
	GdkDisplay *display;

	source_view = GTK_WIDGET (callback_data);

	display = gtk_widget_get_display (source_view);

	gtk_text_buffer_copy_clipboard (GTK_TEXT_VIEW (source_view)->buffer,
					gtk_clipboard_get_for_display (display, GDK_NONE));
}

static void
activate_cb (BonoboObject *control,
	     gboolean state,
	     gpointer callback_data)
{
	GtkWidget *source_view;
	BonoboUIComponent *ui_component;
	Bonobo_UIContainer ui_container;

	BonoboUIVerb verbs[] = {
		BONOBO_UI_VERB ("Copy Text", copy_cb),
		BONOBO_UI_VERB_END
	};

	if (state) {
		source_view = GTK_WIDGET (callback_data);

		/* Get the UI component that's pre-made by the control. */
		ui_component = bonobo_control_get_ui_component (BONOBO_CONTROL (control));

		/* Connect the UI component to the control frame's UI container. */
		ui_container = bonobo_control_get_remote_ui_container (BONOBO_CONTROL (control), NULL);
		bonobo_ui_component_set_container (ui_component, ui_container, NULL);
		bonobo_object_release_unref (ui_container, NULL);

		/* Set up the UI from an XML file. */
		bonobo_ui_util_set_ui (ui_component, DATADIR, "gedit-viewer-ui.xml",
				       "gedit-viewer", NULL);

		bonobo_ui_component_add_verb_list_with_data (ui_component,
							     verbs,
							     source_view);
	}
}

BonoboControl *
gedit_viewer_new (void)
{
	GtkWidget *source_view, *scrolled;
	GtkSourceLanguagesManager *lm;
	GConfClient *conf_client;
	char *monospace_font;
	PangoFontDescription *monospace_font_desc;
	BonoboControl *control;
	BonoboPersistStream *persist_stream;

	gedit_prefs_manager_init ();

	source_view = gtk_source_view_new ();

	lm = gtk_source_languages_manager_new ();
	g_object_ref (lm);
	g_object_set_data_full (G_OBJECT (source_view), "languages-manager",
				lm, (GDestroyNotify) g_object_unref);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (source_view), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (source_view), GTK_WRAP_NONE);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (source_view), 3);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (source_view), 3);

	/* Pick up the monospace font from desktop preferences */
	conf_client = gconf_client_get_default ();
	monospace_font = gconf_client_get_string (conf_client, "/desktop/gnome/interface/monospace_font_name", NULL);
	if (monospace_font) {
		monospace_font_desc = pango_font_description_from_string (monospace_font);
		gtk_widget_modify_font (source_view, monospace_font_desc);
		pango_font_description_free (monospace_font_desc);
	}
	g_object_unref (conf_client);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scrolled), source_view);
	gtk_widget_show_all (scrolled);

	control = bonobo_control_new (scrolled);

	persist_stream = gedit_persist_stream_new (GTK_SOURCE_VIEW (source_view));
	bonobo_object_add_interface (BONOBO_OBJECT (control),
				     BONOBO_OBJECT (persist_stream));

	g_signal_connect_object (control, "activate",
				 G_CALLBACK (activate_cb), source_view, 0);

	return control;
}
