/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-preferences-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2003 Paolo Maggi 
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glade/glade-xml.h>
#include <libgnome/libgnome.h>
#include <gconf/gconf-client.h>
#include <libgnomeui/libgnomeui.h>

#include <gtksourceview/gtksourcetag.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

#include <gedit/gedit-prefs-manager.h>

#include "gedit-preferences-dialog.h"
#include "gedit2.h"
#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gedit-plugin-manager.h"
#include "gedit-languages-manager.h"

enum
{
	NAME_COLUMN = 0,
	ID_COLUMN,
	NUM_COLUMNS
};

typedef struct _GeditPreferencesDialog GeditPreferencesDialog;

struct _GeditPreferencesDialog 
{
	GtkWidget       *dialog;
	
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
	
	/* Undo*/
	GtkWidget	*undo_box;
	GtkWidget	*limited_undo_radiobutton;
	GtkWidget	*unlimited_undo_radiobutton;
	GtkWidget	*undo_levels_spinbutton;

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
	
	/* Right margin */
	GtkWidget	*right_margin_checkbutton;
	GtkWidget	*right_margin_position_spinbutton;
	GtkWidget	*right_margin_position_hbox;

	/* Syntax Highlighting */
	GtkWidget	*enable_syntax_hl_checkbutton;
	GtkWidget	*hl_vbox;
	GtkWidget	*hl_mode_optionmenu;
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


static void
dialog_destroyed (GtkObject  *obj,  
		  void      **dialog_pointer)
{
	gedit_debug (DEBUG_PREFS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog              *dlg, 
			 gint                    res_id,
			 GeditPreferencesDialog *dialog)
{
	gedit_debug (DEBUG_PREFS, "");

	switch (res_id) {
		case GTK_RESPONSE_HELP:
			/* TODO */;
			break;
			
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}

static void
tabs_width_spinbutton_value_changed (GtkSpinButton          *spin_button,
				     GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->tabs_width_spinbutton));

	gedit_prefs_manager_set_tabs_size (gtk_spin_button_get_value_as_int (spin_button));
}
	
static void
insert_spaces_checkbutton_toggled (GtkToggleButton        *button,
				   GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->insert_spaces_checkbutton));

	gedit_prefs_manager_set_insert_spaces (gtk_toggle_button_get_active (button));
}

static void
auto_indent_checkbutton_toggled (GtkToggleButton        *button,
				 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->auto_indent_checkbutton));

	gedit_prefs_manager_set_auto_indent (gtk_toggle_button_get_active (button));
}

static void
auto_save_checkbutton_toggled (GtkToggleButton        *button,
			       GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->auto_save_checkbutton));
	
	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->auto_save_spinbutton, 
					  gedit_prefs_manager_auto_save_interval_can_set());
		
		gedit_prefs_manager_set_auto_save (TRUE);
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->auto_save_spinbutton, FALSE);
		gedit_prefs_manager_set_auto_save (FALSE);
	}
}

static void
backup_copy_checkbutton_toggled (GtkToggleButton        *button,
				 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->backup_copy_checkbutton));
	
	gedit_prefs_manager_set_create_backup_copy (gtk_toggle_button_get_active (button));
}

static void
auto_save_spinbutton_value_changed (GtkSpinButton          *spin_button,
				    GeditPreferencesDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->auto_save_spinbutton));

	gedit_prefs_manager_set_auto_save_interval (
			MAX (1, gtk_spin_button_get_value_as_int (spin_button)));
}

static void
limited_undo_radiobutton_toggled (GtkToggleButton *button, GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->limited_undo_radiobutton));

	if (gtk_toggle_button_get_active (button))
	{
		gint undo_levels;
		
		gtk_widget_set_sensitive (dlg->undo_levels_spinbutton, 
					  gedit_prefs_manager_undo_actions_limit_can_set());
		
		undo_levels = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (dlg->undo_levels_spinbutton));
		g_return_if_fail (undo_levels >= 1);

		gedit_prefs_manager_set_undo_actions_limit (undo_levels);
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->undo_levels_spinbutton, FALSE);

		gedit_prefs_manager_set_undo_actions_limit (-1);
	}
}

static void
undo_levels_spinbutton_value_changed (GtkSpinButton          *spin_button,
				      GeditPreferencesDialog *dlg)
{
	gint undo_levels;

	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->undo_levels_spinbutton));

	undo_levels = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (dlg->undo_levels_spinbutton));
	g_return_if_fail (undo_levels >= 1);

	gedit_prefs_manager_set_undo_actions_limit (undo_levels);
}

static void
setup_editor_page (GeditPreferencesDialog *dlg)
{
	gboolean auto_save;
	gint undo_levels;
	gboolean can_set;

	gedit_debug (DEBUG_PREFS, "");

	/* Set initial state */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->tabs_width_spinbutton),
				   (guint) gedit_prefs_manager_get_tabs_size ());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->insert_spaces_checkbutton), 
				      gedit_prefs_manager_get_insert_spaces ());
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->auto_indent_checkbutton), 
				      gedit_prefs_manager_get_auto_indent ());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->backup_copy_checkbutton),
				      gedit_prefs_manager_get_create_backup_copy ());

	auto_save = gedit_prefs_manager_get_auto_save ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->auto_save_checkbutton),
				      auto_save );
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->auto_save_spinbutton),
				   gedit_prefs_manager_get_auto_save_interval ());

	undo_levels = gedit_prefs_manager_get_undo_actions_limit ();
	
	if (undo_levels > 0)
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->undo_levels_spinbutton),
					   (guint) undo_levels);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->limited_undo_radiobutton), 
				              TRUE);
	}
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->unlimited_undo_radiobutton), 
				              TRUE);


	/* Set widget sensitivity */
	gtk_widget_set_sensitive (dlg->tabs_width_hbox, 
				  gedit_prefs_manager_tabs_size_can_set ());
	gtk_widget_set_sensitive (dlg->insert_spaces_checkbutton,
				  gedit_prefs_manager_insert_spaces_can_set ());
	
	gtk_widget_set_sensitive (dlg->auto_indent_checkbutton,
				  gedit_prefs_manager_auto_indent_can_set ());

	gtk_widget_set_sensitive (dlg->backup_copy_checkbutton,
				  gedit_prefs_manager_create_backup_copy_can_set ());

	gtk_widget_set_sensitive (dlg->autosave_hbox, 
				  gedit_prefs_manager_auto_save_can_set ()); 
	
	gtk_widget_set_sensitive (dlg->auto_save_spinbutton, 
			          auto_save &&
				  gedit_prefs_manager_auto_save_interval_can_set ());

	can_set = gedit_prefs_manager_undo_actions_limit_can_set ();

	gtk_widget_set_sensitive (dlg->undo_box, can_set);
	gtk_widget_set_sensitive (dlg->undo_levels_spinbutton, can_set && (undo_levels > 0));

	/* Connect signal */
	g_signal_connect (G_OBJECT (dlg->tabs_width_spinbutton), "value_changed",
			  G_CALLBACK (tabs_width_spinbutton_value_changed),
			  dlg);

	g_signal_connect (G_OBJECT (dlg->insert_spaces_checkbutton), "toggled", 
			  G_CALLBACK (insert_spaces_checkbutton_toggled), 
			  dlg);
	
	g_signal_connect (G_OBJECT (dlg->auto_indent_checkbutton), "toggled", 
			  G_CALLBACK (auto_indent_checkbutton_toggled), 
			  dlg);
	
	g_signal_connect (G_OBJECT (dlg->auto_save_checkbutton), "toggled", 
			  G_CALLBACK (auto_save_checkbutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->backup_copy_checkbutton), "toggled", 
			  G_CALLBACK (backup_copy_checkbutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->auto_save_spinbutton), "value_changed",
			  G_CALLBACK (auto_save_spinbutton_value_changed),
			  dlg);

	g_signal_connect (G_OBJECT (dlg->limited_undo_radiobutton), "toggled", 
		  	  G_CALLBACK (limited_undo_radiobutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->undo_levels_spinbutton), "value_changed",
		  	  G_CALLBACK (undo_levels_spinbutton_value_changed),
		  	  dlg);
}

static void
display_line_numbers_checkbutton_toggled (GtkToggleButton        *button,
					  GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == 
			GTK_TOGGLE_BUTTON (dlg->display_line_numbers_checkbutton));

	gedit_prefs_manager_set_display_line_numbers (gtk_toggle_button_get_active (button));
}

static gboolean split_button_state = TRUE;

static void
wrap_mode_checkbutton_toggled (GtkToggleButton        *button, 
			       GeditPreferencesDialog *dlg)
{
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton)))
	{
		gedit_prefs_manager_set_wrap_mode (GTK_WRAP_NONE);
		
		gtk_widget_set_sensitive (dlg->split_checkbutton, 
					  FALSE);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->split_checkbutton), TRUE);

	}
	else
	{
		gtk_widget_set_sensitive (dlg->split_checkbutton, 
					  TRUE);

		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->split_checkbutton), FALSE);


		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->split_checkbutton)))
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
	
	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->right_margin_checkbutton));

	active = gtk_toggle_button_get_active (button);
	
	gedit_prefs_manager_set_display_right_margin (active);

	gtk_widget_set_sensitive (dlg->right_margin_position_hbox,
				  active && 
				  gedit_prefs_manager_right_margin_position_can_set ());
}

static void
right_margin_position_spinbutton_value_changed (GtkSpinButton          *spin_button, 
						GeditPreferencesDialog *dlg)
{
	gint value;
	
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->right_margin_position_spinbutton));

	value = CLAMP (gtk_spin_button_get_value_as_int (spin_button), 1, 160);

	gedit_prefs_manager_set_right_margin_position (value);
}

static void
setup_view_page (GeditPreferencesDialog *dlg)
{
	GtkWrapMode wrap_mode;
	gboolean display_right_margin;
	gboolean wrap_mode_can_set;

	gedit_debug (DEBUG_PREFS, "");
	
	/* Set initial state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->display_line_numbers_checkbutton),
				gedit_prefs_manager_get_display_line_numbers ());

	wrap_mode = gedit_prefs_manager_get_wrap_mode ();
	switch (wrap_mode )
	{
		case GTK_WRAP_WORD:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), TRUE);
			break;
		case GTK_WRAP_CHAR:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), FALSE);
			break;
		default:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton), FALSE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), split_button_state);
			gtk_toggle_button_set_inconsistent (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), TRUE);

	}

	display_right_margin = gedit_prefs_manager_get_display_right_margin ();
	
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (dlg->right_margin_checkbutton), 
		display_right_margin);
	
	gtk_spin_button_set_value (
		GTK_SPIN_BUTTON (dlg->right_margin_position_spinbutton),
		(guint)CLAMP (gedit_prefs_manager_get_right_margin_position (), 1, 160));
		
	/* Set widgets sensitivity */
	gtk_widget_set_sensitive (dlg->display_line_numbers_checkbutton,
				  gedit_prefs_manager_display_line_numbers_can_set ());

	wrap_mode_can_set = gedit_prefs_manager_wrap_mode_can_set ();

	gtk_widget_set_sensitive (dlg->wrap_text_checkbutton, 
				  wrap_mode_can_set);

	gtk_widget_set_sensitive (dlg->split_checkbutton, 
				  wrap_mode_can_set && 
				  (wrap_mode != GTK_WRAP_NONE));

	gtk_widget_set_sensitive (dlg->right_margin_checkbutton,
				  gedit_prefs_manager_display_right_margin_can_set ());

	gtk_widget_set_sensitive (dlg->right_margin_position_hbox,
				  display_right_margin && 
				  gedit_prefs_manager_right_margin_position_can_set ());
				  
	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->display_line_numbers_checkbutton), "toggled", 
			  G_CALLBACK (display_line_numbers_checkbutton_toggled), 
			  dlg);
	g_signal_connect (G_OBJECT (dlg->wrap_text_checkbutton), "toggled", 
			  G_CALLBACK (wrap_mode_checkbutton_toggled), 
			  dlg);
	g_signal_connect (G_OBJECT (dlg->split_checkbutton), "toggled", 
			  G_CALLBACK (wrap_mode_checkbutton_toggled), 
			  dlg);
	g_signal_connect (G_OBJECT (dlg->right_margin_checkbutton), "toggled", 
			  G_CALLBACK (right_margin_checkbutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->right_margin_position_spinbutton), "value_changed",
			  G_CALLBACK (right_margin_position_spinbutton_value_changed), 
			  dlg);

}

static void
default_font_font_checkbutton_toggled (GtkToggleButton        *button,
				       GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->default_font_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->font_hbox, FALSE);
		gedit_prefs_manager_set_use_default_font (TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->font_hbox, 
					  gedit_prefs_manager_editor_font_can_set ());
		gedit_prefs_manager_set_use_default_font (FALSE);
	}
}

static void
default_font_colors_checkbutton_toggled (GtkToggleButton        *button,
					 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->default_colors_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->colors_table, FALSE);
		gedit_prefs_manager_set_use_default_colors (TRUE);
	}
	else
	{
		gedit_prefs_manager_set_use_default_colors (FALSE);
		gtk_widget_set_sensitive (dlg->colors_table, 
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

	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (font_button == GTK_FONT_BUTTON (dlg->font_button));

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

	if (button == GTK_COLOR_BUTTON (dlg->text_colorbutton))
	{
		gedit_prefs_manager_set_text_color (color);
		return;
	}

	if (button == GTK_COLOR_BUTTON (dlg->background_colorbutton))
	{
		gedit_prefs_manager_set_background_color (color);
		return;
	}

	if (button == GTK_COLOR_BUTTON (dlg->seltext_colorbutton))
	{
		gedit_prefs_manager_set_selected_text_color (color);
		return;
	}

	if (button == GTK_COLOR_BUTTON (dlg->selection_colorbutton))
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

	gedit_debug (DEBUG_PREFS, "");

	gtk_tooltips_set_tip (dlg->tooltips, dlg->font_button, 
			_("Push this button to select the font to be used by the editor"), NULL);
	gtk_tooltips_set_tip (dlg->tooltips, dlg->text_colorbutton, 
			_("Push this button to configure text color"), NULL);
	gtk_tooltips_set_tip (dlg->tooltips, dlg->background_colorbutton, 
			_("Push this button to configure background color"), NULL);
	gtk_tooltips_set_tip (dlg->tooltips, dlg->seltext_colorbutton, 
			_("Push this button to configure the color in which the selected "
			  "text should appear"), NULL);
	gtk_tooltips_set_tip (dlg->tooltips, dlg->selection_colorbutton, 
			_("Push this button to configure the color in which the selected "
			  "text should be marked"), NULL);

	gedit_utils_set_atk_relation (dlg->font_button, dlg->default_font_checkbutton, 
                                                          ATK_RELATION_CONTROLLED_BY);
	gedit_utils_set_atk_relation (dlg->default_font_checkbutton, dlg->font_button, 
                                                         ATK_RELATION_CONTROLLER_FOR);

	/* read current config and setup initial state */
	use_default_font = gedit_prefs_manager_get_use_default_font ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->default_font_checkbutton),
				      use_default_font);

	use_default_colors = gedit_prefs_manager_get_use_default_colors ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->default_colors_checkbutton),
				      use_default_colors);

	editor_font = gedit_prefs_manager_get_editor_font ();
	if (editor_font != NULL)
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (dlg->font_button),
					       editor_font);
	g_free (editor_font);

	color = gedit_prefs_manager_get_text_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->text_colorbutton),
				    &color);

	color = gedit_prefs_manager_get_background_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->background_colorbutton),
				    &color);

	color = gedit_prefs_manager_get_selected_text_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->seltext_colorbutton),
				    &color);

	color = gedit_prefs_manager_get_selection_color ();
	gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->selection_colorbutton),
				    &color);

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->default_font_checkbutton), "toggled", 
			  G_CALLBACK (default_font_font_checkbutton_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->default_colors_checkbutton), "toggled", 
			  G_CALLBACK (default_font_colors_checkbutton_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->font_button), "font_set", 
			  G_CALLBACK (editor_font_button_font_set), dlg);
	g_signal_connect (G_OBJECT (dlg->text_colorbutton), "color_set",
			  G_CALLBACK (editor_color_button_color_set), dlg);
	g_signal_connect (G_OBJECT (dlg->background_colorbutton), "color_set",
			  G_CALLBACK (editor_color_button_color_set), dlg);
	g_signal_connect (G_OBJECT (dlg->seltext_colorbutton), "color_set",
			  G_CALLBACK (editor_color_button_color_set), dlg);
	g_signal_connect (G_OBJECT (dlg->selection_colorbutton), "color_set",
			  G_CALLBACK (editor_color_button_color_set), dlg);

	/* Set initial widget sensitivity */
	gtk_widget_set_sensitive (dlg->default_font_checkbutton, 
				  gedit_prefs_manager_use_default_font_can_set ());

	gtk_widget_set_sensitive (dlg->default_colors_checkbutton, 
				  gedit_prefs_manager_use_default_colors_can_set ());

	if (use_default_font)
		gtk_widget_set_sensitive (dlg->font_hbox, FALSE);
	else
		gtk_widget_set_sensitive (dlg->font_hbox, 
					  gedit_prefs_manager_editor_font_can_set ());

	if (use_default_colors)
		gtk_widget_set_sensitive (dlg->colors_table, FALSE);
	else
		gtk_widget_set_sensitive (dlg->colors_table, 
					  gedit_prefs_manager_background_color_can_set () &&
				  	  gedit_prefs_manager_text_color_can_set () &&
				  	  gedit_prefs_manager_selection_color_can_set () &&
				  	  gedit_prefs_manager_selected_text_color_can_set ());
}

static void
enable_syntax_hl_button_toggled (GtkWidget              *button,
				 GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == dlg->enable_syntax_hl_checkbutton);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	{
		gedit_prefs_manager_set_enable_syntax_highlighting (TRUE);
		gtk_widget_set_sensitive (dlg->hl_vbox, TRUE);
	}
	else
	{
		gedit_prefs_manager_set_enable_syntax_highlighting (FALSE);
		gtk_widget_set_sensitive (dlg->hl_vbox, FALSE);
	}
}

static void
language_changed_cb (GtkOptionMenu *optionmenu,
		     GeditPreferencesDialog *dlg)
{
	const GSList *languages;
	GSList *tags, *l;
	GtkSourceLanguage *lang;
	GtkTreeIter iter;
	GtkTreePath *path;

	languages = gtk_source_languages_manager_get_available_languages (
						gedit_get_languages_manager ());

	lang = g_slist_nth_data ((GSList*)languages, gtk_option_menu_get_history (optionmenu));

	gtk_list_store_clear (dlg->tags_treeview_model);

	tags = gtk_source_language_get_tags (lang);
	l = tags;

	while (l != NULL)
	{
		gchar *name;
		gchar *id;
		GtkSourceTag *tag;
	       
		tag = GTK_SOURCE_TAG (l->data);
		
		g_object_get (tag, "name", &name, "id", &id, NULL);
		gtk_list_store_append (dlg->tags_treeview_model, &iter);
		gtk_list_store_set (dlg->tags_treeview_model, 
				    &iter, 
				    NAME_COLUMN, name, 
				    ID_COLUMN, id,
				    -1);

		l = g_slist_next (l);
	}
	
	g_slist_foreach (tags, (GFunc)g_object_unref, NULL);
	g_slist_free (tags);
	
	/* Trigger styles_cb on first item so color & font widgets get set. */
	path = gtk_tree_path_new_first ();
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (dlg->tags_treeview), path, NULL, FALSE);
	gtk_tree_path_free (path);
}

static GtkSourceLanguage *
get_selected_language (GeditPreferencesDialog *dlg)
{
	const GSList *languages;
	GtkSourceLanguage *lang;
	
	languages = gtk_source_languages_manager_get_available_languages (
						gedit_get_languages_manager ());
	lang = g_slist_nth_data ((GSList*)languages,
		gtk_option_menu_get_history (GTK_OPTION_MENU (dlg->hl_mode_optionmenu)));

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

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dlg->tags_treeview), &path, NULL);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (dlg->tags_treeview_model),
				 &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (dlg->tags_treeview_model),
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
styles_cb (GtkWidget              *treeview,
	   GeditPreferencesDialog *dlg)
{
	GtkSourceTagStyle *style;

	style = get_selected_style (dlg);
	g_return_if_fail (style != NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->bold_togglebutton),
				      style->bold);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->italic_togglebutton),
				      style->italic);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->underline_togglebutton),
				      style->underline);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->strikethrough_togglebutton),
				      style->strikethrough);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->foreground_checkbutton),
				      style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND);

	if ((style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND) == GTK_SOURCE_TAG_STYLE_USE_FOREGROUND)
	{
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->foreground_colorbutton),
					    &style->foreground);
	}
	else
	{
		GdkColor text_color;
		
		text_color = gedit_prefs_manager_get_text_color ();
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->foreground_colorbutton),
					    &text_color);
	}
	
	gtk_widget_set_sensitive (dlg->foreground_colorbutton, 
				  style->mask & GTK_SOURCE_TAG_STYLE_USE_FOREGROUND);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->background_checkbutton),
				      style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND);
	
	if ((style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND) == GTK_SOURCE_TAG_STYLE_USE_BACKGROUND)
	{
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->background_colorbutton_2),
					    &style->background);
	}
	else
	{
		GdkColor background_color;
		
		background_color = gedit_prefs_manager_get_background_color ();
		gtk_color_button_set_color (GTK_COLOR_BUTTON (dlg->background_colorbutton_2),
					    &background_color);
	}

	gtk_widget_set_sensitive (dlg->background_colorbutton_2, 
				  style->mask & GTK_SOURCE_TAG_STYLE_USE_BACKGROUND);

	gtk_widget_set_sensitive (dlg->reset_button, !style->is_default);

	gtk_source_tag_style_free (style);
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

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dlg->tags_treeview), &path, NULL);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (dlg->tags_treeview_model),
				 &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (dlg->tags_treeview_model),
			    &iter, ID_COLUMN, &id, -1);

	lang = get_selected_language (dlg);

	style = gtk_source_language_get_tag_style (lang, id);
	if (style == NULL)
	{
		style = gtk_source_tag_style_new ();
	}

	new_style = gtk_source_tag_style_copy (style);
	
	new_style->bold = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->bold_togglebutton));
	new_style->italic = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->italic_togglebutton));
	new_style->underline = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->underline_togglebutton));
	new_style->strikethrough = gtk_toggle_button_get_active (
					GTK_TOGGLE_BUTTON (dlg->strikethrough_togglebutton));

	if (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (dlg->foreground_checkbutton)))
	{
		new_style->mask |= GTK_SOURCE_TAG_STYLE_USE_FOREGROUND;
		gtk_color_button_get_color (GTK_COLOR_BUTTON (dlg->foreground_colorbutton),
					    &new_style->foreground);
		gtk_widget_set_sensitive (dlg->foreground_colorbutton,
					  TRUE);
	}
	else
	{
		new_style->mask &= ~GTK_SOURCE_TAG_STYLE_USE_FOREGROUND;
		gtk_widget_set_sensitive (dlg->foreground_colorbutton,
					  FALSE);
	}

	if (gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (dlg->background_checkbutton)))
	{
		new_style->mask |= GTK_SOURCE_TAG_STYLE_USE_BACKGROUND;
		gtk_color_button_get_color (GTK_COLOR_BUTTON (dlg->background_colorbutton_2),
					    &new_style->background);
		gtk_widget_set_sensitive (dlg->background_colorbutton_2,
					  TRUE);
	}
	else
	{
		new_style->mask &= ~GTK_SOURCE_TAG_STYLE_USE_BACKGROUND;
		gtk_widget_set_sensitive (dlg->background_colorbutton_2,
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
		
		gtk_widget_set_sensitive (dlg->reset_button, 
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
reset_button_clicked (GtkButton              *button, 
		      GeditPreferencesDialog *dlg)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *id;
	GtkSourceLanguage *lang;

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dlg->tags_treeview), &path, NULL);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (dlg->tags_treeview_model),
				 &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (GTK_TREE_MODEL (dlg->tags_treeview_model),
			    &iter, ID_COLUMN, &id, -1);

	lang = get_selected_language (dlg);

	gedit_language_set_tag_style (lang, id, NULL);

	styles_cb (NULL, dlg);
}

static void
setup_syntax_highlighting_page (GeditPreferencesDialog *dlg)
{
	gboolean hl_enabled;
	GtkWidget *menu;
	const GSList *languages, *l;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Set initial state */
	hl_enabled = gedit_prefs_manager_get_enable_syntax_highlighting ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->enable_syntax_hl_checkbutton),
				      hl_enabled);
	gtk_widget_set_sensitive (dlg->hl_vbox, hl_enabled);

	/* Create GtkListStore for styles & setup treeview. */
	dlg->tags_treeview_model = gtk_list_store_new (NUM_COLUMNS, 
						       G_TYPE_STRING, 
						       G_TYPE_STRING);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dlg->tags_treeview_model),
					      0, 
					      GTK_SORT_ASCENDING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (dlg->tags_treeview),
				 GTK_TREE_MODEL (dlg->tags_treeview_model));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Elements"), renderer,
							   "text", NAME_COLUMN, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->tags_treeview), column);

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->hl_mode_optionmenu), "changed",
			  G_CALLBACK (language_changed_cb), dlg);

	g_signal_connect (G_OBJECT (dlg->tags_treeview), "cursor-changed",
			  G_CALLBACK (styles_cb), dlg);

	/* Add languages to optionmenu. */
	menu = gtk_menu_new ();

	languages = gtk_source_languages_manager_get_available_languages (
						gedit_get_languages_manager ());

	l = languages;

	while (l != NULL)
	{
		GtkSourceLanguage *lang = GTK_SOURCE_LANGUAGE (l->data);
		GtkWidget *menuitem;
		gchar *name;

		lang = GTK_SOURCE_LANGUAGE (l->data);

		name = gtk_source_language_get_name (lang);
		menuitem = gtk_menu_item_new_with_label (name);
		g_free (name);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

		l = g_slist_next (l);
	}
	
	gtk_widget_show_all (menu);
	
	gtk_option_menu_set_menu (GTK_OPTION_MENU (dlg->hl_mode_optionmenu), menu);

	g_signal_connect (G_OBJECT (dlg->enable_syntax_hl_checkbutton), "toggled",
			  G_CALLBACK (enable_syntax_hl_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->bold_togglebutton), "toggled",
			  G_CALLBACK (style_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->italic_togglebutton), "toggled",
			  G_CALLBACK (style_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->underline_togglebutton), "toggled",
			  G_CALLBACK (style_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->strikethrough_togglebutton), "toggled",
			  G_CALLBACK (style_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->foreground_checkbutton), "toggled",
			  G_CALLBACK (style_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->background_checkbutton), "toggled",
			  G_CALLBACK (style_button_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->foreground_colorbutton), "color_set",
			  G_CALLBACK (style_color_set), dlg);
	g_signal_connect (G_OBJECT (dlg->background_colorbutton_2), "color_set",
			  G_CALLBACK (style_color_set), dlg);
	g_signal_connect (G_OBJECT (dlg->reset_button), "clicked",
			  G_CALLBACK (reset_button_clicked), dlg);
}

static void
setup_plugins_page (GeditPreferencesDialog *dlg)
{
	GtkWidget *page_content;
	
	gedit_debug (DEBUG_PREFS, "");

	page_content = gedit_plugin_manager_get_page ();
	g_return_if_fail (page_content != NULL);
	
	gtk_box_pack_start (GTK_BOX (dlg->plugin_manager_place_holder), page_content, TRUE, TRUE, 0);

}

static GeditPreferencesDialog *
get_preferences_dialog (GtkWindow *parent)
{
	static GeditPreferencesDialog *dialog = NULL;
	GladeXML *gui;
	
	gedit_debug (DEBUG_PREFS, "");

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      parent);
		gtk_window_present (GTK_WINDOW (dialog->dialog));

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "gedit-preferences.glade2",
			     "preferences_dialog", NULL);
	if (!gui)
	{
		gedit_warning (parent,
			       MISSING_FILE,
			       GEDIT_GLADEDIR "gedit-preferences.glade2");
		return NULL;
	}

	dialog = g_new0 (GeditPreferencesDialog, 1);

	dialog->dialog = glade_xml_get_widget (gui, "preferences_dialog");
	dialog->display_line_numbers_checkbutton = glade_xml_get_widget (gui, "display_line_numbers_checkbutton");
	dialog->wrap_text_checkbutton = glade_xml_get_widget (gui, "wrap_text_checkbutton");
	dialog->split_checkbutton = glade_xml_get_widget (gui, "split_checkbutton");

	dialog->default_font_checkbutton = glade_xml_get_widget (gui, "default_font_checkbutton");
	dialog->default_colors_checkbutton = glade_xml_get_widget (gui, "default_colors_checkbutton");

	dialog->font_button = glade_xml_get_widget (gui, "font_button");
	dialog->text_colorbutton = glade_xml_get_widget (gui, "text_colorbutton");
	dialog->background_colorbutton = glade_xml_get_widget (gui, "background_colorbutton");
	dialog->seltext_colorbutton = glade_xml_get_widget (gui, "seltext_colorbutton");
	dialog->selection_colorbutton = glade_xml_get_widget (gui, "selection_colorbutton");

	dialog->colors_table = glade_xml_get_widget (gui, "colors_table");
	dialog->font_hbox = glade_xml_get_widget (gui, "font_hbox");	

	dialog->right_margin_checkbutton = glade_xml_get_widget (gui, "right_margin_checkbutton");
	dialog->right_margin_position_spinbutton = glade_xml_get_widget (gui, "right_margin_position_spinbutton");
	dialog->right_margin_position_hbox = glade_xml_get_widget (gui, "right_margin_position_hbox");

	dialog->tabs_width_spinbutton = glade_xml_get_widget (gui, "tabs_width_spinbutton");
	dialog->tabs_width_hbox = glade_xml_get_widget (gui, "tabs_width_hbox");
	dialog->insert_spaces_checkbutton = glade_xml_get_widget (gui, "insert_spaces_checkbutton");
	
	dialog->auto_indent_checkbutton = glade_xml_get_widget (gui, "auto_indent_checkbutton");

	dialog->autosave_hbox = glade_xml_get_widget (gui, "autosave_hbox");
	dialog->backup_copy_checkbutton = glade_xml_get_widget (gui, "backup_copy_checkbutton");
	dialog->auto_save_checkbutton = glade_xml_get_widget (gui, "auto_save_checkbutton");
	dialog->auto_save_spinbutton = glade_xml_get_widget (gui, "auto_save_spinbutton");

	dialog->limited_undo_radiobutton = glade_xml_get_widget (gui, "limited_undo_radiobutton");
	dialog->unlimited_undo_radiobutton = glade_xml_get_widget (gui, "unlimited_undo_radiobutton");
	dialog->undo_levels_spinbutton = glade_xml_get_widget (gui, "undo_levels_spinbutton");
	dialog->undo_box = glade_xml_get_widget (gui, "undo_box");

	dialog->plugin_manager_place_holder = glade_xml_get_widget (gui, "plugin_manager_place_holder");

	dialog->enable_syntax_hl_checkbutton = glade_xml_get_widget (gui, "enable_syntax_hl_checkbutton");
	dialog->hl_vbox = glade_xml_get_widget (gui, "hl_vbox");
	dialog->hl_mode_optionmenu = glade_xml_get_widget (gui, "hl_mode_optionmenu");
	dialog->tags_treeview = glade_xml_get_widget (gui, "tags_treeview");
	
	dialog->bold_togglebutton = glade_xml_get_widget (gui, "bold_togglebutton");
	dialog->italic_togglebutton = glade_xml_get_widget (gui, "italic_togglebutton");
	dialog->underline_togglebutton = glade_xml_get_widget (gui, "underline_togglebutton");
	dialog->strikethrough_togglebutton = glade_xml_get_widget (gui, "strikethrough_togglebutton");
	dialog->foreground_checkbutton = glade_xml_get_widget (gui, "foreground_checkbutton");
	dialog->foreground_colorbutton = glade_xml_get_widget (gui, "foreground_colorbutton");
	dialog->background_checkbutton = glade_xml_get_widget (gui, "background_checkbutton");
	dialog->background_colorbutton_2 = glade_xml_get_widget (gui, "background_colorbutton_2");
	dialog->reset_button = glade_xml_get_widget (gui, "reset_button");

	if (!dialog->dialog ||
	    !dialog->display_line_numbers_checkbutton ||
	    !dialog->wrap_text_checkbutton ||
	    !dialog->split_checkbutton ||
	    !dialog->default_font_checkbutton ||
	    !dialog->default_colors_checkbutton ||
	    !dialog->font_button ||
	    !dialog->text_colorbutton ||
	    !dialog->background_colorbutton ||
	    !dialog->seltext_colorbutton ||
	    !dialog->selection_colorbutton ||
	    !dialog->colors_table ||
	    !dialog->font_hbox ||
	    !dialog->right_margin_checkbutton ||
	    !dialog->right_margin_position_spinbutton ||
	    !dialog->right_margin_position_hbox ||
	    !dialog->tabs_width_spinbutton ||
	    !dialog->tabs_width_hbox ||
	    !dialog->insert_spaces_checkbutton ||
	    !dialog->auto_indent_checkbutton ||
	    !dialog->autosave_hbox ||
	    !dialog->backup_copy_checkbutton ||
	    !dialog->auto_save_checkbutton ||
	    !dialog->auto_save_spinbutton ||
	    !dialog->limited_undo_radiobutton ||
	    !dialog->unlimited_undo_radiobutton ||
	    !dialog->undo_levels_spinbutton ||
	    !dialog->undo_box ||
	    !dialog->plugin_manager_place_holder ||
	    !dialog->enable_syntax_hl_checkbutton ||
	    !dialog->hl_vbox ||
	    !dialog->hl_mode_optionmenu ||
	    !dialog->tags_treeview ||
	    !dialog->bold_togglebutton ||
	    !dialog->italic_togglebutton ||
	    !dialog->underline_togglebutton ||
	    !dialog->strikethrough_togglebutton ||
	    !dialog->foreground_checkbutton ||
	    !dialog->foreground_colorbutton ||
	    !dialog->background_checkbutton ||
	    !dialog->background_colorbutton_2 ||
	    !dialog->reset_button)
	{
		gedit_warning (parent,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "gedit-preferences.glade2");

		if (!dialog->dialog)
			gtk_widget_destroy (dialog->dialog);
		
		g_object_unref (gui);
		g_free (dialog);
		dialog = NULL;

		return NULL;
	}
	
	dialog->tooltips = gtk_tooltips_new ();
		
	setup_editor_page (dialog);
	setup_view_page (dialog);
	setup_font_colors_page (dialog);
	setup_syntax_highlighting_page (dialog);
	setup_plugins_page (dialog);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      parent);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			  G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;

}

void
gedit_show_preferences_dialog (GtkWindow *parent)
{
	GeditPreferencesDialog *dialog;

	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (parent != NULL);

	dialog = get_preferences_dialog (parent);
	if (!dialog) 
		return;

	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show_all (dialog->dialog);
}

