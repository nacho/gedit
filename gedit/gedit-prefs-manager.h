/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs-manager.h
 * This file is part of gedit
 *
 * Copyright (C) 2002  Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_PREFS_MANAGER_H__
#define __GEDIT_PREFS_MANAGER_H__

#include <gdk/gdkcolor.h>
#include <gtk/gtkenums.h>
#include <glib/gslist.h>

#include <bonobo/bonobo-window.h>

#define GEDIT_BASE_KEY	"/apps/gedit-2"

typedef enum {
	GEDIT_TOOLBAR_SYSTEM = 0,
	GEDIT_TOOLBAR_ICONS,
	GEDIT_TOOLBAR_ICONS_AND_TEXT
} GeditToolbarSetting;

typedef enum {
	GEDIT_SAVE_ALWAYS_UTF8 = 0,
	GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE,
	GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE,
	GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL
} GeditSaveEncodingSetting;

/** LIFE CYCLE MANAGEMENT FUNCTIONS **/

gboolean		 gedit_prefs_manager_init (void);

/* This function must be called before exiting gedit */
void			 gedit_prefs_manager_shutdown (void);


/** PREFS MANAGEMENT FUNCTIONS **/

/* Use default font */
gboolean 		 gedit_prefs_manager_get_use_default_font 	(void);
void			 gedit_prefs_manager_set_use_default_font 	(gboolean udf);
gboolean		 gedit_prefs_manager_use_default_font_can_set	(void);

/* Editor font */
gchar 			*gedit_prefs_manager_get_editor_font		(void);
void			 gedit_prefs_manager_set_editor_font 		(const gchar *font);
gboolean		 gedit_prefs_manager_editor_font_can_set	(void);

/* Use default colors */
gboolean 		 gedit_prefs_manager_get_use_default_colors 	(void);
void			 gedit_prefs_manager_set_use_default_colors 	(gboolean udc);
gboolean		 gedit_prefs_manager_use_default_colors_can_set	(void);

/* Background color */	
GdkColor		 gedit_prefs_manager_get_background_color	(void);
void 			 gedit_prefs_manager_set_background_color	(GdkColor color);
gboolean		 gedit_prefs_manager_background_color_can_set	(void);

/* Text color */	
GdkColor		 gedit_prefs_manager_get_text_color		(void);
void 			 gedit_prefs_manager_set_text_color		(GdkColor color);
gboolean		 gedit_prefs_manager_text_color_can_set		(void);

/* Selection color */	
GdkColor		 gedit_prefs_manager_get_selection_color	(void);
void 			 gedit_prefs_manager_set_selection_color	(GdkColor color);
gboolean		 gedit_prefs_manager_selection_color_can_set	(void);

/* Selected text color */	
GdkColor		 gedit_prefs_manager_get_selected_text_color	(void);
void 			 gedit_prefs_manager_set_selected_text_color	(GdkColor color);
gboolean		 gedit_prefs_manager_selected_text_color_can_set(void);

/* Create backup copy */
gboolean		 gedit_prefs_manager_get_create_backup_copy	(void);
void			 gedit_prefs_manager_set_create_backup_copy	(gboolean cbc);
gboolean		 gedit_prefs_manager_create_backup_copy_can_set	(void);

/* Backup extension. This is configurable only using gconftool or gconf-editor */
gchar			*gedit_prefs_manager_get_backup_extension	(void);

/* Auto save */
gboolean		 gedit_prefs_manager_get_auto_save		(void);
void			 gedit_prefs_manager_set_auto_save		(gboolean as);
gboolean		 gedit_prefs_manager_auto_save_can_set		(void);

/* Auto save interval */
gint			 gedit_prefs_manager_get_auto_save_interval	(void);
void			 gedit_prefs_manager_set_auto_save_interval	(gint asi);
gboolean		 gedit_prefs_manager_auto_save_interval_can_set	(void);

/* Save encoding */
GeditSaveEncodingSetting gedit_prefs_manager_get_save_encoding		(void);
void			 gedit_prefs_manager_set_save_encoding		(GeditSaveEncodingSetting se);
gboolean 		 gedit_prefs_manager_save_encoding_can_set	(void);

/* Undo actions limit: if < 1 then no limits */
gint 			 gedit_prefs_manager_get_undo_actions_limit	(void);
void			 gedit_prefs_manager_set_undo_actions_limit	(gint ual);
gboolean		 gedit_prefs_manager_undo_actions_limit_can_set	(void);

/* Wrap mode */
GtkWrapMode		 gedit_prefs_manager_get_wrap_mode		(void);
void			 gedit_prefs_manager_set_wrap_mode		(GtkWrapMode wp);
gboolean		 gedit_prefs_manager_wrap_mode_can_set		(void);

/* Tabs size */
gint			 gedit_prefs_manager_get_tabs_size		(void);
void			 gedit_prefs_manager_set_tabs_size		(gint ts);
gboolean		 gedit_prefs_manager_tabs_size_can_set		(void);

/* Insert spaces */
gboolean		 gedit_prefs_manager_get_insert_spaces	 	(void);
void			 gedit_prefs_manager_set_insert_spaces	 	(gboolean ai);
gboolean		 gedit_prefs_manager_insert_spaces_can_set 	(void);


/* Auto indent */
gboolean		 gedit_prefs_manager_get_auto_indent	 	(void);
void			 gedit_prefs_manager_set_auto_indent	 	(gboolean ai);
gboolean		 gedit_prefs_manager_auto_indent_can_set 	(void);

/* Display line numbers */
gboolean		 gedit_prefs_manager_get_display_line_numbers 	(void);
void			 gedit_prefs_manager_set_display_line_numbers 	(gboolean dln);
gboolean		 gedit_prefs_manager_display_line_numbers_can_set (void);

/* Toolbar visible */
gboolean		 gedit_prefs_manager_get_toolbar_visible	(void);
void			 gedit_prefs_manager_set_toolbar_visible	(gboolean tv);
gboolean		 gedit_prefs_manager_toolbar_visible_can_set	(void);

/* Toolbar buttons style */
GeditToolbarSetting 	 gedit_prefs_manager_get_toolbar_buttons_style	(void); 
void 			 gedit_prefs_manager_set_toolbar_buttons_style	(GeditToolbarSetting tbs); 
gboolean		 gedit_prefs_manager_toolbar_buttons_style_can_set (void);

/* Statusbar visible */
gboolean		 gedit_prefs_manager_get_statusbar_visible	(void);
void			 gedit_prefs_manager_set_statusbar_visible	(gboolean sv);
gboolean		 gedit_prefs_manager_statusbar_visible_can_set	(void);

/* Show cursor position in statusbar */
gboolean		 gedit_prefs_manager_get_statusbar_show_cursor_position	(void);
void			 gedit_prefs_manager_set_statusbar_show_cursor_position	(gboolean scp);
gboolean		 gedit_prefs_manager_statusbar_show_cursor_position_can_set (void);
									
/* Show overwrite mode in statusbar */
gboolean		 gedit_prefs_manager_get_statusbar_show_overwrite_mode (void);
void			 gedit_prefs_manager_set_statusbar_show_overwrite_mode (gboolean som);
gboolean		 gedit_prefs_manager_statusbar_show_overwrite_mode_can_set (void);

/* Print header */
gboolean		 gedit_prefs_manager_get_print_header		(void);
void			 gedit_prefs_manager_set_print_header		(gboolean ph);
gboolean		 gedit_prefs_manager_print_header_can_set	(void);
	
/* Wrap mode while printing */
GtkWrapMode		 gedit_prefs_manager_get_print_wrap_mode	(void);
void			 gedit_prefs_manager_set_print_wrap_mode	(GtkWrapMode pwm);
gboolean		 gedit_prefs_manager_print_wrap_mode_can_set	(void);

/* Print line numbers */	
gint		 	 gedit_prefs_manager_get_print_line_numbers	(void);
void 			 gedit_prefs_manager_set_print_line_numbers	(gint pln);
gboolean		 gedit_prefs_manager_print_line_numbers_can_set	(void);

/* Font used to print the body of documents */
gchar			*gedit_prefs_manager_get_print_font_body	(void);
void			 gedit_prefs_manager_set_print_font_body	(const gchar *font);
gboolean		 gedit_prefs_manager_print_font_body_can_set	(void);
const gchar		*gedit_prefs_manager_get_default_print_font_body (void);

/* Font used to print headers */
gchar			*gedit_prefs_manager_get_print_font_header	(void);
void			 gedit_prefs_manager_set_print_font_header	(const gchar *font);
gboolean		 gedit_prefs_manager_print_font_header_can_set	(void);
const gchar		*gedit_prefs_manager_get_default_print_font_header (void);

/* Font used to print line numbers */
gchar			*gedit_prefs_manager_get_print_font_numbers	(void);
void			 gedit_prefs_manager_set_print_font_numbers	(const gchar *font);
gboolean		 gedit_prefs_manager_print_font_numbers_can_set	(void);
const gchar		*gedit_prefs_manager_get_default_print_font_numbers (void);

/* Max number of files in "Recent Files" menu. 
 * This is configurable only using gconftool or gconf-editor 
 */
gint		 	 gedit_prefs_manager_get_max_recents		(void);

/* Window state */
gint		 	 gedit_prefs_manager_get_window_state		(void);
void 			 gedit_prefs_manager_set_window_state		(gint ws);
gboolean		 gedit_prefs_manager_window_state_can_set	(void);

/* Window height */
gint		 	 gedit_prefs_manager_get_window_height		(void);
gint		 	 gedit_prefs_manager_get_default_window_height	(void);
void 			 gedit_prefs_manager_set_window_height		(gint wh);
gboolean		 gedit_prefs_manager_window_height_can_set	(void);

/* Window width */
gint		 	 gedit_prefs_manager_get_window_width		(void);
gint		 	 gedit_prefs_manager_get_default_window_width	(void);
void 			 gedit_prefs_manager_set_window_width		(gint ww);
gboolean		 gedit_prefs_manager_window_width_can_set	(void);

void			 gedit_prefs_manager_save_window_size_and_state (BonoboWindow *window);

/* Encodings */
GSList 			*gedit_prefs_manager_get_encodings		(void);
void			 gedit_prefs_manager_set_encodings		(const GSList *encs);
gboolean 		 gedit_prefs_manager_encodings_can_set		(void);

#endif  /* __GEDIT_PREFS_MANAGER_H__ */


