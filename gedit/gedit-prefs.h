/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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

#ifndef __GEDIT_PREFS_H__
#define __GEDIT_PREFS_H__

#include <gdk/gdkcolor.h>
#include <gtk/gtkwidget.h>

typedef struct _GeditPreferences GeditPreferences;

typedef enum {
	GEDIT_TOOLBAR_SYSTEM = 0,
	GEDIT_TOOLBAR_ICONS,
	GEDIT_TOOLBAR_ICONS_AND_TEXT
} GeditToolbarSetting;

typedef enum {
	GEDIT_SAVE_ALWAYS_UTF8 = 0,
	GEDIT_SAVE_CURRENT_LOCALE_WHEN_POSSIBLE,
	GEDIT_SAVE_CURRENT_LOCALE_IF_USED
} GeditSaveEncodingSetting;


struct _GeditPreferences
{
	/* Editor/Fonts&Color */
	gboolean	use_default_font;
	gchar 		*editor_font;
	
	gboolean	use_default_colors;
	GdkColor	background_color;
	GdkColor 	text_color;
	GdkColor 	selection_color;
	GdkColor	selected_text_color;

	/* Editor/Save */
	gboolean	create_backup_copy;
	gchar		*backup_extension; /* Advanced: configurable only using
					      gconftool */

	gboolean	auto_save;
	gint		auto_save_interval;

	GeditSaveEncodingSetting save_encoding;

	/* Editor/Undo */
	gint 		undo_levels; /* If < 1 then no limits */

	/* Editor/Wrap Mode*/
	gint		wrap_mode;

	/* Editor/Tabs */
	gint		tab_size;

	/* Editor/Line numbers */
	gboolean	show_line_numbers;

	/* User Interface/Toolbar */
	gboolean		toolbar_visible;
	GeditToolbarSetting 	toolbar_buttons_style; 
	gboolean		toolbar_view_tooltips;

	/* User Interface/Statusbar */
	gboolean	statusbar_visible;
	gboolean	statusbar_view_cursor_position;
	gboolean	statusbar_view_overwrite_mode;

	/* User Interface/MDI Mode */
	gint		mdi_mode;
	gint		mdi_tabs_position; /* Tabs position when mdi_mode is notebook */
	
	/* Window geometry */
	gint 		window_width;
	gint		window_height;

	/* Printing stuff */
	gboolean 	 print_header;
	gboolean	 print_wrap_lines;
	gint		 print_line_numbers;

	gchar		*print_font_body;
	gchar		*print_font_header;
	gchar		*print_font_numbers;
};

extern GeditPreferences *gedit_settings;

void gedit_prefs_save_settings (void);
void gedit_prefs_load_settings (void);
void gedit_prefs_init (void);

gboolean gedit_prefs_configure_event_handler (GtkWidget	     *widget,
					      GdkEventConfigure   *event);

#endif /* __GEDIT_PREFS_H__ */

