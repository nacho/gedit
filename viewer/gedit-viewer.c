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

static void 
gedit_viewer_set_colors (GtkWidget *view, gboolean def, GdkColor *backgroud, GdkColor *text,
		GdkColor *selection, GdkColor *sel_text)
{
	if (!def)
	{	
		if (backgroud != NULL)
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_NORMAL, backgroud);

		if (text != NULL)			
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_NORMAL, text);
	
		if (selection != NULL)
		{
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_SELECTED, selection);

			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_ACTIVE, selection);
		}

		if (sel_text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_SELECTED, sel_text);		

			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_ACTIVE, sel_text);		
		}
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		rc_style->color_flags [GTK_STATE_NORMAL] = 0;
		rc_style->color_flags [GTK_STATE_SELECTED] = 0;
		rc_style->color_flags [GTK_STATE_ACTIVE] = 0;

		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);
	}
}

static void
gedit_viewer_set_font (GtkWidget *view, gboolean def, const gchar *font_name)
{
	if (!def)
	{
		PangoFontDescription *font_desc = NULL;

		g_return_if_fail (font_name != NULL);
		
		font_desc = pango_font_description_from_string (font_name);
		g_return_if_fail (font_desc != NULL);

		gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
		
		pango_font_description_free (font_desc);		
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		if (rc_style->font_desc)
			pango_font_description_free (rc_style->font_desc);

		rc_style->font_desc = NULL;
		
		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);
	}
}

BonoboControl *
gedit_viewer_new (void)
{
	GtkWidget *source_view, *scrolled;
	GtkSourceLanguagesManager *lm;
	BonoboControl *control;
	BonoboPersistStream *persist_stream;

	gedit_prefs_manager_init ();

	source_view = gtk_source_view_new ();

	lm = gtk_source_languages_manager_new ();
	g_object_ref (lm);
	g_object_set_data_full (G_OBJECT (source_view), "languages-manager",
				lm, (GDestroyNotify) g_object_unref);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (source_view), FALSE);

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
	if (!gedit_prefs_manager_get_use_default_font ())
	{
		gchar *editor_font = gedit_prefs_manager_get_editor_font ();
		
		gedit_viewer_set_font (source_view, FALSE, editor_font);

		g_free (editor_font);
	}

	if (!gedit_prefs_manager_get_use_default_colors ())
	{
		GdkColor background, text, selection, sel_text;

		background = gedit_prefs_manager_get_background_color ();
		text = gedit_prefs_manager_get_text_color ();
		selection = gedit_prefs_manager_get_selection_color ();
		sel_text = gedit_prefs_manager_get_selected_text_color ();

		gedit_viewer_set_colors (source_view, FALSE,
				&background, &text, &selection, &sel_text);
	}	

	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (source_view), 
				     gedit_prefs_manager_get_wrap_mode ());
	
	g_object_set (G_OBJECT (source_view), 
		      "tabs_width", gedit_prefs_manager_get_tabs_size (),
		      "show_line_numbers", gedit_prefs_manager_get_display_line_numbers (),
		      NULL);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_IN);

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
