/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-preferences-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2005 Paolo Maggi 
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
 * Modified by the gedit Team, 2001-2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <gtksourceview/gtksourcetag.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

#include <gedit/gedit-prefs-manager.h>

#include "gedit-preferences-dialog.h"
#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-plugin-manager.h"
#include "gedit-languages-manager.h"
#include "gedit-help.h"

/*
 * gedit-preferences dialog is a singleton since we don't
 * want two dialogs showing an inconsistent state of the
 * preferences.
 * When gedit_show_preferences_dialog is called and there
 * is already a prefs dialog dialog open, it is reparented
 * and shown.
 */

static GtkWidget *preferences_dialog = NULL;


enum
{
	NAME_COLUMN = 0,
	ID_COLUMN,
	NUM_COLUMNS
};


#define GEDIT_PREFERENCES_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PREFERENCES_DIALOG, GeditPreferencesDialogPrivate))


struct _GeditPreferencesDialogPrivate
{
	GtkTooltips	*tooltips;

	/* Font & Colors */
	GtkWidget	*default_font_checkbutton;
	GtkWidget	*default_colors_checkbutton;
	GtkWidget	*font_button;
	GtkWidget	*text_colorbutton;
	GtkWidget	*background_colorbutton;
	GtkWidget	*seltext_colorbutton;
	GtkWidget	*selection_colorbutton;
	GtkWidget	*colors_table;
	GtkWidget	*font_hbox;

	/* Tabs */
	GtkWidget	*tabs_width_spinbutton;
	GtkWidget	*insert_spaces_checkbutton;
	GtkWidget	*tabs_width_hbox;

	/* Auto indentation */
	GtkWidget	*auto_indent_checkbutton;

	/* Text Wrapping */
	GtkWidget	*wrap_text_checkbutton;
	GtkWidget	*split_checkbutton;

	/* File Saving */
	GtkWidget	*backup_copy_checkbutton;
	GtkWidget	*auto_save_checkbutton;
	GtkWidget	*auto_save_spinbutton;
	GtkWidget	*autosave_hbox;
	
	/* Line numbers */
	GtkWidget	*display_line_numbers_checkbutton;

	/* Highlight current line */
	GtkWidget	*highlight_current_line_checkbutton;
	
	/* Highlight matching bracket */
	GtkWidget	*bracket_matching_checkbutton;
	
	/* Right margin */
	GtkWidget	*right_margin_checkbutton;
	GtkWidget	*right_margin_position_spinbutton;
	GtkWidget	*right_margin_position_hbox;

	/* Syntax Highlighting */
	GtkWidget	*enable_syntax_hl_checkbutton;
	GtkWidget	*hl_vbox;
	GtkWidget	*hl_mode_combobox;
	GtkListStore	*tags_treeview_model;
	GtkWidget	*tags_treeview;
	GtkWidget	*bold_togglebutton;
	GtkWidget	*italic_togglebutton;
	GtkWidget	*underline_togglebutton;
	GtkWidget	*strikethrough_togglebutton;
	GtkWidget	*foreground_checkbutton;
	GtkWidget	*foreground_colorbutton;
	GtkWidget	*background_checkbutton;
	GtkWidget	*background_colorbutton_2;
	GtkWidget	*reset_button;

	/* Plugins manager */
	GtkWidget	*plugin_manager_place_holder;
};


G_DEFINE_TYPE(GeditPreferencesDialog, gedit_preferences_dialog, GTK_TYPE_DIALOG)


static void
gedit_preferences_dialog_finalize (GObject *object)
{
	GeditPreferencesDialog *dialog = GEDIT_PREFERENCES_DIALOG(object);

	g_object_unref (dialog->priv->tooltips);
	G_OBJECT_CLASS (gedit_preferences_dialog_parent_class)->finalize (object);
}

static void 
gedit_preferences_dialog_class_init (GeditPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_preferences_dialog_finalize;
	g_type_class_add_private (object_class, sizeof (GeditPreferencesDialogPrivate));
}

static void
dialog_response_handler (GtkDialog *dlg, 
			 gint       res_id)
{
	gedit_debug (DEBUG_PREFS);

	switch (res_id)
	{
		case GTK_RESPONSE_HELP:
			gedit_help_display (GTK_WINDOW (dlg),
					    "gedit.xml",
					    "gedit-prefs");

			g_signal_stop_emission_by_name (dlg, "response");

			break;

		default:
			gtk_widget_destroy (GTK_WIDGET(dlg));
	}
}

static void
tabs_width_spinbutton_value_changed (GtkSpinButton          *spin_button,
				     GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->tabs_width_spinbutton));

	gedit_prefs_manager_set_tabs_size (gtk_spin_button_get_value_as_int (spin_button));
}
	
static void
insert_spaces_checkbutton_toggled (GtkToggleButton        *button,
				   GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->insert_spaces_checkbutton));

	gedit_prefs_manager_set_insert_spaces (gtk_toggle_button_get_active (button));
}

static void
auto_indent_checkbutton_toggled (GtkToggleButton        *button,
				 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->auto_indent_checkbutton));

	gedit_prefs_manager_set_auto_indent (gtk_toggle_button_get_active (button));
}

static void
auto_save_checkbutton_toggled (GtkToggleButton        *button,
			       GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->auto_save_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->priv->auto_save_spinbutton, 
					  gedit_prefs_manager_auto_save_interval_can_set());

		gedit_prefs_manager_set_auto_save (TRUE);
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->priv->auto_save_spinbutton, FALSE);
		gedit_prefs_manager_set_auto_save (FALSE);
	}
}

static void
backup_copy_checkbutton_toggled (GtkToggleButton        *button,
				 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->backup_copy_checkbutton));
	
	gedit_prefs_manager_set_create_backup_copy (gtk_toggle_button_get_active (button));
}

static void
auto_save_spinbutton_value_changed (GtkSpinButton          *spin_button,
				    GeditPreferencesDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->auto_save_spinbutton));

	gedit_prefs_manager_set_auto_save_interval (
			MAX (1, gtk_spin_button_get_value_as_int (spin_button)));
}

static void
setup_editor_page (GeditPreferencesDialog *dlg)
{
	gboolean auto_save;
	gint auto_save_interval;

	gedit_debug (DEBUG_PREFS);

	/* Set initial state */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->tabs_width_spinbutton),
				   (guint) gedit_prefs_manager_get_tabs_size ());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->insert_spaces_checkbutton), 
				      gedit_prefs_manager_get_insert_spaces ());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->auto_indent_checkbutton), 
				      gedit_prefs_manager_get_auto_indent ());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->backup_copy_checkbutton),
				      gedit_prefs_manager_get_create_backup_copy ());

	auto_save = gedit_prefs_manager_get_auto_save ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->auto_save_checkbutton),
				      auto_save);

	auto_save_interval = gedit_prefs_manager_get_auto_save_interval ();
	if (auto_save_interval <= 0)
		auto_save_interval = GPM_DEFAULT_AUTO_SAVE_INTERVAL;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->auto_save_spinbutton),
				   auto_save_interval);

	/* Set widget sensitivity */
	gtk_widget_set_sensitive (dlg->priv->tabs_width_hbox, 
				  gedit_prefs_manager_tabs_size_can_set ());
	gtk_widget_set_sensitive (dlg->priv->insert_spaces_checkbutton,
				  gedit_prefs_manager_insert_spaces_can_set ());
	gtk_widget_set_sensitive (dlg->priv->auto_indent_checkbutton,
				  gedit_prefs_manager_auto_indent_can_set ());
	gtk_widget_set_sensitive (dlg->priv->backup_copy_checkbutton,
				  gedit_prefs_manager_create_backup_copy_can_set ());
	gtk_widget_set_sensitive (dlg->priv->autosave_hbox, 
				  gedit_prefs_manager_auto_save_can_set ()); 
	gtk_widget_set_sensitive (dlg->priv->auto_save_spinbutton, 
			          auto_save &&
				  gedit_prefs_manager_auto_save_interval_can_set ());

	/* Connect signal */
	g_signal_connect (dlg->priv->tabs_width_spinbutton,
			  "value_changed",
			  G_CALLBACK (tabs_width_spinbutton_value_changed),
			  dlg);
	g_signal_connect (dlg->priv->insert_spaces_checkbutton,
			 "toggled",
			  G_CALLBACK (insert_spaces_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->auto_indent_checkbutton,
			  "toggled",
			  G_CALLBACK (auto_indent_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->auto_save_checkbutton,
			  "toggled",
			  G_CALLBACK (auto_save_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->backup_copy_checkbutton,
			  "toggled",
			  G_CALLBACK (backup_copy_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->auto_save_spinbutton,
			  "value_changed",
			  G_CALLBACK (auto_save_spinbutton_value_changed),
			  dlg);
}

static void
display_line_numbers_checkbutton_toggled (GtkToggleButton        *button,
					  GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == 
			GTK_TOGGLE_BUTTON (dlg->priv->display_line_numbers_checkbutton));

	gedit_prefs_manager_set_display_line_numbers (gtk_toggle_button_get_active (button));
}

static void
highlight_current_line_checkbutton_toggled (GtkToggleButton        *button,
					    GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == 
			GTK_TOGGLE_BUTTON (dlg->priv->highlight_current_line_checkbutton));

	gedit_prefs_manager_set_highlight_current_line (gtk_toggle_button_get_active (button));
}

static void
bracket_matching_checkbutton_toggled (GtkToggleButton        *button,
				      GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == 
			GTK_TOGGLE_BUTTON (dlg->priv->bracket_matching_checkbutton));

	gedit_prefs_manager_set_bracket_matching (
				gtk_toggle_button_get_active (button));
}

static gboolean split_button_state = TRUE;

static void
wrap_mode_checkbutton_toggled (GtkToggleButton        *button, 
			       GeditPreferencesDialog *dlg)
{
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton)))
	{
		gedit_prefs_manager_set_wrap_mode (GTK_WRAP_NONE);
		
		gtk_widget_set_sensitive (dlg->priv->split_checkbutton, 
					  FALSE);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->split_checkbutton, 
					  TRUE);

		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), FALSE);


		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton)))
		{
			split_button_state = TRUE;
			
			gedit_prefs_manager_set_wrap_mode (GTK_WRAP_WORD);
		}
		else
		{
			split_button_state = FALSE;
			
			gedit_prefs_manager_set_wrap_mode (GTK_WRAP_CHAR);
		}
	}
}

static void
right_margin_checkbutton_toggled (GtkToggleButton        *button,
				  GeditPreferencesDialog *dlg)
{
	gboolean active;
	
	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->right_margin_checkbutton));

	active = gtk_toggle_button_get_active (button);
	
	gedit_prefs_manager_set_display_right_margin (active);

	gtk_widget_set_sensitive (dlg->priv->right_margin_position_hbox,
				  active && 
				  gedit_prefs_manager_right_margin_position_can_set ());
}

static void
right_margin_position_spinbutton_value_changed (GtkSpinButton          *spin_button, 
						GeditPreferencesDialog *dlg)
{
	gint value;
	
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->right_margin_position_spinbutton));

	value = CLAMP (gtk_spin_button_get_value_as_int (spin_button), 1, 160);

	gedit_prefs_manager_set_right_margin_position (value);
}

static void
setup_view_page (GeditPreferencesDialog *dlg)
{
	GtkWrapMode wrap_mode;
	gboolean display_right_margin;
	gboolean wrap_mode_can_set;

	gedit_debug (DEBUG_PREFS);
	
	/* Set initial state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->display_line_numbers_checkbutton),
				      gedit_prefs_manager_get_display_line_numbers ());
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->highlight_current_line_checkbutton),
				      gedit_prefs_manager_get_highlight_current_line ());
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->bracket_matching_checkbutton),
				      gedit_prefs_manager_get_bracket_matching ());

	wrap_mode = gedit_prefs_manager_get_wrap_mode ();
	switch (wrap_mode )
	{
		case GTK_WRAP_WORD:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), TRUE);
			break;
		case GTK_WRAP_CHAR:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), FALSE);
			break;
		default:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_text_checkbutton), FALSE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), split_button_state);
			gtk_toggle_button_set_inconsistent (
				GTK_TOGGLE_BUTTON (dlg->priv->split_checkbutton), TRUE);

	}

	display_right_margin = gedit_prefs_manager_get_display_right_margin ();
	
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (dlg->priv->right_margin_checkbutton), 
		display_right_margin);
	
	gtk_spin_button_set_value (
		GTK_SPIN_BUTTON (dlg->priv->right_margin_position_spinbutton),
		(guint)CLAMP (gedit_prefs_manager_get_right_margin_position (), 1, 160));
		
	/* Set widgets sensitivity */
	gtk_widget_set_sensitive (dlg->priv->display_line_numbers_checkbutton,
				  gedit_prefs_manager_display_line_numbers_can_set ());
	gtk_widget_set_sensitive (dlg->priv->highlight_current_line_checkbutton,
				  gedit_prefs_manager_highlight_current_line_can_set ());
	gtk_widget_set_sensitive (dlg->priv->bracket_matching_checkbutton,
				  gedit_prefs_manager_bracket_matching_can_set ());
	wrap_mode_can_set = gedit_prefs_manager_wrap_mode_can_set ();
	gtk_widget_set_sensitive (dlg->priv->wrap_text_checkbutton, 
				  wrap_mode_can_set);
	gtk_widget_set_sensitive (dlg->priv->split_checkbutton, 
				  wrap_mode_can_set && 
				  (wrap_mode != GTK_WRAP_NONE));
	gtk_widget_set_sensitive (dlg->priv->right_margin_checkbutton,
				  gedit_prefs_manager_display_right_margin_can_set ());
	gtk_widget_set_sensitive (dlg->priv->right_margin_position_hbox,
				  display_right_margin && 
				  gedit_prefs_manager_right_margin_position_can_set ());
				  
	/* Connect signals */
	g_signal_connect (dlg->priv->display_line_numbers_checkbutton,
			  "toggled",
			  G_CALLBACK (display_line_numbers_checkbutton_toggled), 
			  dlg);
	g_signal_connect (dlg->priv->highlight_current_line_checkbutton,
			  "toggled", 
			  G_CALLBACK (highlight_current_line_checkbutton_toggled), 
			  dlg);
	g_signal_connect (dlg->priv->bracket_matching_checkbutton,
			  "toggled", 
			  G_CALLBACK (bracket_matching_checkbutton_toggled), 
			  dlg);
	g_signal_connect (dlg->priv->wrap_text_checkbutton,
			  "toggled", 
			  G_CALLBACK (wrap_mode_checkbutton_toggled), 
			  dlg);
	g_signal_connect (dlg->priv->split_checkbutton,
			  "toggled", 
			  G_CALLBACK (wrap_mode_checkbutton_toggled), 
			  dlg);
	g_signal_connect (dlg->priv->right_margin_checkbutton,
			  "toggled", 
			  G_CALLBACK (right_margin_checkbutton_toggled), 
			  dlg);
	g_signal_connect (dlg->priv->right_margin_position_spinbutton,
			  "value_changed",
			  G_CALLBACK (right_margin_position_spinbutton_value_changed), 
			  dlg);
}

static void
default_font_font_checkbutton_toggled (GtkToggleButton        *button,
				       GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->default_font_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->priv->font_hbox, FALSE);
		gedit_prefs_manager_set_use_default_font (TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->font_hbox, 
					  gedit_prefs_manager_editor_font_can_set ());
		gedit_prefs_manager_set_use_default_font (FALSE);
	}
}

static void
default_font_colors_checkbutton_toggled (GtkToggleButton        *button,
					 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->default_colors_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->priv->colors_table, FALSE);
		gedit_prefs_manager_set_use_default_colors (TRUE);
	}
	else
	{
		gedit_prefs_manager_set_use_default_colors (FALSE);
		gtk_widget_set_sensitive (dlg->priv->colors_table, 
					  gedit_prefs_manager_background_color_can_set () &&
				  	  gedit_prefs_manager_text_color_can_set () &&
				  	  gedit_prefs_manager_selection_color_can_set () &&
				  	  gedit_prefs_manager_selected_text_color_can_set ());
	}
}

static void
editor_font_button_font_set (GtkFontButton          *font_button,
			     GeditPreferencesDialog *dlg)
{
	const gchar *font_name;

	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (font_button == GTK_FONT_BUTTON (dlg->priv->font_button));

	/* FIXME: Can this fail? Gtk docs are a bit terse... 21-02-2004 pbor */
	font_name = gtk_font_button_get_font_name (font_button);
	if (!font_name)
	{
		g_warning ("Could not get font name");
		return;
	}

	gedit_prefs_manager_set_editor_font (font_name);
}

static void 
editor_color_button_color_set (GtkColorButton         *button, 
			       GeditPreferencesDialog *dlg)
{
	GdkColor color;

	gtk_color_button_get_color (button, &color);

	if (button == GTK_COLOR_BUTTON (dlg->priv->text_colorbutton))
	{
		gedit_prefs_manager_set_text_color (color);
		return;
	}

	if (button == GTK_COLOR_BUTTON (dlg->priv->background_colorbutton))
	{
		gedit_prefs_manager_set_background_color (color);
		return;
	}

	if (button == GTK_COLOR_BUTTON (dlg->priv->seltext_colorbutton))
	{
		gedit_prefs_manager_set_selected_text_color (color);
		return;
	}

	if (button == GTK_COLOR_BUTTON (dlg->priv->selection_colorbutton))
	{
		gedit_prefs_manager_set_selection_color (color);
		return;
	}

	g_return_if_reached ();	
}

static void
setup_font_colors_page (GeditPreferencesDialog *dlg)
{
	gboolean use_default_font;
	gboolean use_default_colors;
	gchar *editor_font = NULL;
	GdkColor color;

	gedit_debug (DEBUG_PREFS);

	gtk_tooltips_set_tip (dlg->priv->tooltips, dlg->priv->font_button, 
			_("Push this button to select the font to be used by the editor"), NULL);
	gtk_tooltips_set_tip (dlg->priv->tooltips, dlg->priv->text_colorbutton, 
			_("Push this button to configure text color"), NULL);
	gtk_tooltips_set_tip (dlg->priv->tooltips, dlg->priv->background_colorbutton, 
			_("Push this button to configure background color"), NULL);
	gtk_tooltips_set_tip (dlg->priv->tooltips, dlg->priv->seltext_colorbutton, 
			_("Push this button to configure the color in which the selected "
			  "text should appear"), NULL);
	gtk_tooltips_set_tip (dlg->priv->tooltips, dlg->priv->selection_colorbutton, 
			_("Push this button to configure the color in which the selected "
			  "text should be marked"), NULL);

	gedit_utils_set_atk_relation (dlg->priv->font_button, dlg->priv->default_font_checkbutton, 
                                                          ATK_RELATION_CONTROLLED_BY);
	gedit_utils_set_atk_relation (dlg->priv->default_font_checkbutton, dlg->priv->font_button, 
                                                         ATK_RELATION_CONTROLLER_FOR);

	/* read current config and setup initial state */
	use_default_font = gedit_prefs_manager_get_use_default_font ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->default_font_checkbutton),
				      use_default_font);

	use_default_colors = gedit_prefs_manager_get_use_default_colors ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->default_colors_checkbutton),
				      use_default_colors);

	editor_font = gedit_prefs_manager_get_editor_font ();
	if (editor_font != NULL)
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (dlg->priv->font_button),
					       editor_font);
	g_free (editor_font);

	color = gedit_prefs_manager_get_text_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->text_colorbutton),
				    &color);

	color = gedit_prefs_manager_get_background_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->background_colorbutton),
				    &color);

	color = gedit_prefs_manager_get_selected_text_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->seltext_colorbutton),
				    &color);

	color = gedit_prefs_manager_get_selection_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->selection_colorbutton),
				    &color);

	/* Connect signals */
	g_signal_connect (dlg->priv->default_font_checkbutton,
			  "toggled",
			  G_CALLBACK (default_font_font_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->default_colors_checkbutton,
			  "toggled",
			  G_CALLBACK (default_font_colors_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->priv->font_button,
			  "font_set",
			  G_CALLBACK (editor_font_button_font_set),
			  dlg);
	g_signal_connect (dlg->priv->text_colorbutton,
			  "color_set",
			  G_CALLBACK (editor_color_button_color_set),
			  dlg);
	g_signal_connect (dlg->priv->background_colorbutton,
			  "color_set",
			  G_CALLBACK (editor_color_button_color_set),
			  dlg);
	g_signal_connect (dlg->priv->seltext_colorbutton,
			  "color_set",
			  G_CALLBACK (editor_color_button_color_set),
			  dlg);
	g_signal_connect (dlg->priv->selection_colorbutton,
			  "color_set",
			  G_CALLBACK (editor_color_button_color_set),
			 dlg);

	/* Set initial widget sensitivity */
	gtk_widget_set_sensitive (dlg->priv->default_font_checkbutton, 
				  gedit_prefs_manager_use_default_font_can_set ());

	gtk_widget_set_sensitive (dlg->priv->default_colors_checkbutton, 
				  gedit_prefs_manager_use_default_colors_can_set ());

	if (use_default_font)
		gtk_widget_set_sensitive (dlg->priv->font_hbox, FALSE);
	else
		gtk_widget_set_sensitive (dlg->priv->font_hbox, 
					  gedit_prefs_manager_editor_font_can_set ());

	if (use_default_colors)
		gtk_widget_set_sensitive (dlg->priv->colors_table, FALSE);
	else
		gtk_widget_set_sensitive (dlg->priv->colors_table, 
					  gedit_prefs_manager_background_color_can_set () &&
				  	  gedit_prefs_manager_text_color_can_set () &&
				  	  gedit_prefs_manager_selection_color_can_set () &&
				  	  gedit_prefs_manager_selected_text_color_can_set ());
}

static void
enable_syntax_hl_button_toggled (GtkWidget              *button,
				 GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == dlg->priv->enable_syntax_hl_checkbutton);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	{
		gedit_prefs_manager_set_enable_syntax_highlighting (TRUE);
		gtk_widget_set_sensitive (dlg->priv->hl_vbox, TRUE);
	}
	else
	{
		gedit_prefs_manager_set_enable_syntax_highlighting (FALSE);
		gtk_widget_set_sensitive (dlg->priv->hl_vbox, FALSE);
	}
}

static void
language_changed_cb (GtkComboBox            *combobox,
		     GeditPreferencesDialog *dlg)
{
	const GSList *languages;
	gint active;
	GSList *tags, *l;
	GtkSourceLanguage *lang;
	GtkTreeIter iter;
	GtkTreePath *path;

	languages = gedit_languages_manager_get_available_languages_sorted (
						gedit_get_languages_manager ());

	active = gtk_combo_box_get_active (combobox);
	if (active < 0)
	{
		/* no active language: no lang files found */
		return;
	}

	lang = g_slist_nth_data ((GSList*)languages, active);

	gtk_list_store_clear (dlg->priv->tags_treeview_model);

	tags = gtk_source_language_get_tags (lang);
	l = tags;

	while (l != NULL)
	{
		gchar *name;
		gchar *id;
		GtkSourceTag *tag;

		tag = GTK_SOURCE_TAG (l->data);

		g_object_get (tag, "name", &name, "id", &id, NULL);
		gtk_list_store_append (dlg->priv->tags_treeview_model, &iter);
		gtk_list_store_set (dlg->priv->tags_treeview_model, 
				    &iter, 
				    NAME_COLUMN, name, 
				    ID_COLUMN, id,
				    -1);
		g_free (name);
		g_free (id);

		l = g_slist_next (l);
	}

	g_slist_foreach (tags, (GFunc)g_object_unref, NULL);
	g_slist_free (tags);
	
	/* Trigger styles_cb on first item so color & font widgets get set. */
	path = gtk_tree_path_new_first ();
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (dlg->priv->tags_treeview), path, NULL, FALSE);
	gtk_tree_path_free (path);
}

static GtkSourceLanguage *
get_selected_language (GeditPreferencesDialog *dlg)
{
	const GSList *languages;
	GtkSourceLanguage *lang;
	
	languages = gedit_languages_manager_get_available_languages_sorted (
						gedit_get_languages_manager ());
	lang = g_slist_nth_data ((GSList*)languages,
		gtk_combo_box_get_active (GTK_COMBO_BOX (dlg->priv->hl_mode_combobox)));

	return lang;
}

static GtkSourceTagStyle *
get_selected_style (GeditPreferencesDialog *dlg)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *id;
	GtkSourceLanguage *lang;
	GtkSourceTagStyle *style;
	GtkSourceTagStyle *def_style;

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dlg->priv->tags_treeview), &path, NULL);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (dlg->priv->tags_treeview_model),
				 &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (dlg->priv->tags_treeview_model),
			    &iter, ID_COLUMN, &id, -1);

	lang = get_selected_language (dlg);

	style = gtk_source_language_get_tag_style (lang, id);
	def_style = gtk_source_language_get_tag_default_style (lang, id);

	if (style == NULL)
		return def_style;

	style->is_default = TRUE;
	def_style->is_default = TRUE;

	style->is_default = (memcmp (style, def_style, sizeof (GtkSourceTagStyle)) == 0);
	gtk_source_tag_style_free (def_style);
	
	g_free (id);

	return style;
}

static void
style_button_toggled (GtkToggleButton        *button,
		      GeditPreferencesDialog *dlg)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *id;
	GtkSourceLanguage *lang;
	GtkSourceTagStyle *style;
	GtkSourceTagStyle *new_style;

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dlg->priv->tags_treeview), &path, NULL);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (dlg->priv->tags_treeview_model),
				 &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (dlg->priv->tags_treeview_model),
			    &iter, ID_COLUMN, &id, -1);

	lang = get_selected_language (dlg);

	style = gtk_source_language_get_tag_style (lang, id);
	if (style == NULL)
	{
		style = gtk_source_tag_style_new ();
	}

	new_style = gtk_source_tag_style_copy (style);
	
	new_style->bold = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->priv->bold_togglebutton));
	new_style->italic = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->priv->italic_togglebutton));
	new_style->underline = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->priv->underline_togglebutton));
	new_style->strikethrough = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->priv->strikethrough_togglebutton));

	if (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (dlg->priv->foreground_checkbutton)))
	{
		new_style->mask |= GTK_SOURCE_TAG_STYLE_USE_FOREGROUND;
		gtk_color_button_get_color (GTK_COLOR_BUTTON (dlg->priv->foreground_colorbutton),
					    &new_style->foreground);
		gtk_widget_set_sensitive (dlg->priv->foreground_colorbutton,
					  TRUE);
	}
	else
	{
		new_style->mask &= ~GTK_SOURCE_TAG_STYLE_USE_FOREGROUND;
		gtk_widget_set_sensitive (dlg->priv->foreground_colorbutton,
					  FALSE);
	}

	if (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (dlg->priv->background_checkbutton)))
	{
		new_style->mask |= GTK_SOURCE_TAG_STYLE_USE_BACKGROUND;
		gtk_color_button_get_color (GTK_COLOR_BUTTON (dlg->priv->background_colorbutton_2),
					    &new_style->background);
		gtk_widget_set_sensitive (dlg->priv->background_colorbutton_2,
					  TRUE);
	}
	else
	{
		new_style->mask &= ~GTK_SOURCE_TAG_STYLE_USE_BACKGROUND;
		gtk_widget_set_sensitive (dlg->priv->background_colorbutton_2,
					  FALSE);
	}

	if (memcmp (style, new_style, sizeof (GtkSourceTagStyle)) != 0)
	{
		GtkSourceTagStyle *def_style;

		def_style = gtk_source_language_get_tag_default_style (lang, id);

		if (!(new_style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND))
		{
			new_style->background = def_style->background;
		}

		if (!(new_style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND))
		{
			new_style->foreground = def_style->foreground;
		}
		
		gtk_widget_set_sensitive (dlg->priv->reset_button, 
					  memcmp (new_style, def_style, sizeof (GtkSourceTagStyle)) != 0);

		gedit_language_set_tag_style (lang, id, new_style);

		gtk_source_tag_style_free (def_style);
	}

	gtk_source_tag_style_free (style);
	gtk_source_tag_style_free (new_style);

	g_free (id);
}

static void
style_color_set (GtkColorButton         *button,
		 GeditPreferencesDialog *dlg)
{
	style_button_toggled (NULL, dlg);
}

static void
styles_cb (GtkWidget              *treeview,
	   GeditPreferencesDialog *dlg)
{
	GtkSourceTagStyle *style;

	style = get_selected_style (dlg);
	g_return_if_fail (style != NULL);

	/* we must block callbacks while setting the new values */
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->bold_togglebutton),
					 G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->italic_togglebutton),
					 G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->underline_togglebutton),
					 G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->strikethrough_togglebutton),
					 G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->foreground_checkbutton), 
					 G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->background_checkbutton), 
					 G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->foreground_colorbutton),
					 G_CALLBACK (style_color_set), dlg);
	g_signal_handlers_block_by_func (G_OBJECT (dlg->priv->background_colorbutton_2), 
					 G_CALLBACK (style_color_set), dlg);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->bold_togglebutton),
				      style->bold);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->italic_togglebutton),
				      style->italic);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->underline_togglebutton),
				      style->underline);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->strikethrough_togglebutton),
				      style->strikethrough);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->foreground_checkbutton),
				      style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND);

	if ((style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND) == GTK_SOURCE_TAG_STYLE_USE_FOREGROUND)
	{
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->foreground_colorbutton),
					    &style->foreground);
	}
	else
	{
		GdkColor text_color;
		
		text_color = gedit_prefs_manager_get_text_color ();
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->foreground_colorbutton),
					    &text_color);
	}
	
	gtk_widget_set_sensitive (dlg->priv->foreground_colorbutton, 
				  style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->background_checkbutton),
				      style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND);
	
	if ((style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND) == GTK_SOURCE_TAG_STYLE_USE_BACKGROUND)
	{
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->background_colorbutton_2),
					    &style->background);
	}
	else
	{
		GdkColor background_color;
		
		background_color = gedit_prefs_manager_get_background_color ();
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->priv->background_colorbutton_2),
					    &background_color);
	}

	gtk_widget_set_sensitive (dlg->priv->background_colorbutton_2, 
				  style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND);

	gtk_widget_set_sensitive (dlg->priv->reset_button, !style->is_default);

	/* reenable callbacks */
	g_signal_handlers_unblock_by_func (dlg->priv->bold_togglebutton,
					   G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->italic_togglebutton,
					   G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->underline_togglebutton,
					   G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->strikethrough_togglebutton,
					   G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->foreground_checkbutton, 
					   G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->background_checkbutton, 
					   G_CALLBACK (style_button_toggled), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->foreground_colorbutton,
					   G_CALLBACK (style_color_set), dlg);
	g_signal_handlers_unblock_by_func (dlg->priv->background_colorbutton_2, 
					   G_CALLBACK (style_color_set), dlg);

	gtk_source_tag_style_free (style);
}

static void
reset_button_clicked (GtkButton              *button, 
		      GeditPreferencesDialog *dlg)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *id;
	GtkSourceLanguage *lang;

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dlg->priv->tags_treeview), &path, NULL);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (dlg->priv->tags_treeview_model),
				 &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (dlg->priv->tags_treeview_model),
			    &iter, ID_COLUMN, &id, -1);

	lang = get_selected_language (dlg);

	gedit_language_set_tag_style (lang, id, NULL);

	styles_cb (NULL, dlg);
}

static void
select_default_language (GeditPreferencesDialog *dlg)
{
	const GSList *languages, *l;
	GeditDocument *current_document;
	GtkSourceLanguage *current_document_language;
	int current_item;

	current_document = gedit_window_get_active_document (GEDIT_WINDOW(
						gtk_window_get_transient_for (GTK_WINDOW(dlg))));
	if (current_document != NULL)
		current_document_language = gedit_document_get_language (current_document);
	else
		current_document_language = NULL;

	languages = gedit_languages_manager_get_available_languages_sorted (
						gedit_get_languages_manager ());

	l = languages;
	current_item = 0;

	while (l != NULL)
	{
		GtkSourceLanguage *lang = GTK_SOURCE_LANGUAGE (l->data);

		if (lang == current_document_language)
			break;

		current_item++;
		l = g_slist_next (l);
	}

	if (l != NULL)
		gtk_combo_box_set_active (GTK_COMBO_BOX (dlg->priv->hl_mode_combobox),
					  current_item);
}

static void
setup_syntax_highlighting_page (GeditPreferencesDialog *dlg)
{
	gboolean hl_enabled;
	const GSList *languages, *l;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Set initial state */
	hl_enabled = gedit_prefs_manager_get_enable_syntax_highlighting ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->enable_syntax_hl_checkbutton),
				      hl_enabled);
	gtk_widget_set_sensitive (dlg->priv->hl_vbox, hl_enabled);

	/* Create GtkListStore for styles & setup treeview. */
	dlg->priv->tags_treeview_model = gtk_list_store_new (NUM_COLUMNS, 
							     G_TYPE_STRING, 
							     G_TYPE_STRING);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dlg->priv->tags_treeview_model),
					      0, 
					      GTK_SORT_ASCENDING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (dlg->priv->tags_treeview),
				 GTK_TREE_MODEL (dlg->priv->tags_treeview_model));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Elements"), renderer,
							   "text", NAME_COLUMN, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->tags_treeview), column);

	/* Connect signals */
	g_signal_connect (dlg->priv->hl_mode_combobox,
			  "changed",
			  G_CALLBACK (language_changed_cb),
			  dlg);
	g_signal_connect (dlg->priv->tags_treeview,
			  "cursor-changed",
			  G_CALLBACK (styles_cb),
			  dlg);

	languages = gedit_languages_manager_get_available_languages_sorted (
						gedit_get_languages_manager ());

	l = languages;

	while (l != NULL)
	{
		GtkSourceLanguage *lang = GTK_SOURCE_LANGUAGE (l->data);
		gchar *name = gtk_source_language_get_name (lang);

		gtk_combo_box_append_text (GTK_COMBO_BOX (dlg->priv->hl_mode_combobox), name);
		g_free (name);

		l = g_slist_next (l);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (dlg->priv->hl_mode_combobox), 0);

	g_signal_connect (dlg->priv->enable_syntax_hl_checkbutton,
			 "toggled",
			  G_CALLBACK (enable_syntax_hl_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->bold_togglebutton,
			  "toggled",
			  G_CALLBACK (style_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->italic_togglebutton,
			  "toggled",
			  G_CALLBACK (style_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->underline_togglebutton,
			  "toggled",
			  G_CALLBACK (style_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->strikethrough_togglebutton,
			  "toggled",
			  G_CALLBACK (style_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->foreground_checkbutton,
			  "toggled",
			  G_CALLBACK (style_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->background_checkbutton,
			  "toggled",
			  G_CALLBACK (style_button_toggled),
			  dlg);
	g_signal_connect (dlg->priv->foreground_colorbutton,
			  "color_set",
			  G_CALLBACK (style_color_set),
			  dlg);
	g_signal_connect (dlg->priv->background_colorbutton_2,
			  "color_set",
			  G_CALLBACK (style_color_set),
			  dlg);
	g_signal_connect (dlg->priv->reset_button,
			  "clicked",
			  G_CALLBACK (reset_button_clicked),
			  dlg);
}

static void
setup_plugins_page (GeditPreferencesDialog *dlg)
{
	GtkWidget *page_content;

	gedit_debug (DEBUG_PREFS);

	page_content = gedit_plugin_manager_new ();
	g_return_if_fail (page_content != NULL);

	gtk_box_pack_start (GTK_BOX (dlg->priv->plugin_manager_place_holder),
			    page_content,
			    TRUE,
			    TRUE,
			    0);

	gtk_widget_show_all (page_content);
}

static void
gedit_preferences_dialog_init (GeditPreferencesDialog *dlg)
{
	GtkWidget *content;
	GtkWidget *error_widget;
	gboolean ret;

	gedit_debug (DEBUG_PREFS);

	dlg->priv = GEDIT_PREFERENCES_DIALOG_GET_PRIVATE (dlg);

	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CLOSE,
				GTK_RESPONSE_CLOSE,
				GTK_STOCK_HELP,
				GTK_RESPONSE_HELP,
				NULL);

	gtk_window_set_title (GTK_WINDOW (dlg), _("gedit Preferences"));
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	g_signal_connect (dlg, "response",
			  G_CALLBACK (dialog_response_handler), NULL);

	dlg->priv->tooltips = gtk_tooltips_new ();
	g_object_ref (dlg->priv->tooltips);
	gtk_object_sink (GTK_OBJECT (dlg->priv->tooltips));

	ret = gedit_utils_get_glade_widgets (GEDIT_GLADEDIR "gedit-preferences-dialog.glade",
		"notebook",
		&error_widget,

		"notebook", &content,
		"display_line_numbers_checkbutton", &dlg->priv->display_line_numbers_checkbutton,
		"highlight_current_line_checkbutton", &dlg->priv->highlight_current_line_checkbutton,
				
		"bracket_matching_checkbutton", &dlg->priv->bracket_matching_checkbutton,
		"wrap_text_checkbutton", &dlg->priv->wrap_text_checkbutton,
		"split_checkbutton", &dlg->priv->split_checkbutton,
		"default_font_checkbutton", &dlg->priv->default_font_checkbutton,
		"default_colors_checkbutton", &dlg->priv->default_colors_checkbutton,

		"font_button", &dlg->priv->font_button,
		"text_colorbutton", &dlg->priv->text_colorbutton,
		"background_colorbutton", &dlg->priv->background_colorbutton,
		"seltext_colorbutton", &dlg->priv->seltext_colorbutton,
		"selection_colorbutton", &dlg->priv->selection_colorbutton,

		"colors_table", &dlg->priv->colors_table,
		"font_hbox", &dlg->priv->font_hbox,

		"right_margin_checkbutton", &dlg->priv->right_margin_checkbutton,
		"right_margin_position_spinbutton", &dlg->priv->right_margin_position_spinbutton,
		"right_margin_position_hbox", &dlg->priv->right_margin_position_hbox,

		"tabs_width_spinbutton", &dlg->priv->tabs_width_spinbutton,
		"tabs_width_hbox", &dlg->priv->tabs_width_hbox,
		"insert_spaces_checkbutton", &dlg->priv->insert_spaces_checkbutton,

		"auto_indent_checkbutton", &dlg->priv->auto_indent_checkbutton,

		"autosave_hbox", &dlg->priv->autosave_hbox,
		"backup_copy_checkbutton", &dlg->priv->backup_copy_checkbutton,
		"auto_save_checkbutton", &dlg->priv->auto_save_checkbutton,
		"auto_save_spinbutton", &dlg->priv->auto_save_spinbutton,

		"plugin_manager_place_holder", &dlg->priv->plugin_manager_place_holder,

		"enable_syntax_hl_checkbutton", &dlg->priv->enable_syntax_hl_checkbutton,
		"hl_vbox", &dlg->priv->hl_vbox,
		"hl_mode_combobox", &dlg->priv->hl_mode_combobox,
		"tags_treeview", &dlg->priv->tags_treeview,

		"bold_togglebutton", &dlg->priv->bold_togglebutton,
		"italic_togglebutton", &dlg->priv->italic_togglebutton,
		"underline_togglebutton", &dlg->priv->underline_togglebutton,
		"strikethrough_togglebutton", &dlg->priv->strikethrough_togglebutton,
		"foreground_checkbutton", &dlg->priv->foreground_checkbutton,
		"foreground_colorbutton", &dlg->priv->foreground_colorbutton,
		"background_checkbutton", &dlg->priv->background_checkbutton,
		"background_colorbutton_2", &dlg->priv->background_colorbutton_2,
		"reset_button", &dlg->priv->reset_button,
		NULL);

	if (!ret)
	{
		gtk_widget_show (error_widget);
			
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox),
					     error_widget);

		return;
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    content, FALSE, FALSE, 0);

	setup_editor_page (dlg);
	setup_view_page (dlg);
	setup_font_colors_page (dlg);
	setup_syntax_highlighting_page (dlg);
	setup_plugins_page (dlg);
}

void
gedit_show_preferences_dialog (GeditWindow *parent)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (GEDIT_IS_WINDOW (parent));

	if (preferences_dialog == NULL)
	{
		preferences_dialog = GTK_WIDGET (g_object_new (GEDIT_TYPE_PREFERENCES_DIALOG, NULL));
		g_signal_connect (preferences_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &preferences_dialog);
	}

	if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (preferences_dialog)))
	{
		gtk_window_set_transient_for (GTK_WINDOW (preferences_dialog),
					      GTK_WINDOW (parent));
		select_default_language (GEDIT_PREFERENCES_DIALOG (preferences_dialog));
	}

	gtk_window_present (GTK_WINDOW (preferences_dialog));
}
