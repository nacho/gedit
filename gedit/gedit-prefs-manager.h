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

#define GEDIT_BASE_KEY	"/apps/gedit-2"

#define GPM_PREFS_DIR			GEDIT_BASE_KEY "/preferences"

/* Editor */
#define GPM_FONT_DIR			GPM_PREFS_DIR "/editor/font"
#define GPM_USE_DEFAULT_FONT		GPM_FONT_DIR "/use_default_font"
#define GPM_EDITOR_FONT			GPM_FONT_DIR "/editor_font"

#define GPM_COLORS_DIR			GPM_PREFS_DIR  "/editor/colors"
#define GPM_USE_DEFAULT_COLORS		GPM_COLORS_DIR "/use_default_colors"
#define GPM_BACKGROUND_COLOR		GPM_COLORS_DIR "/background_color"
#define GPM_TEXT_COLOR			GPM_COLORS_DIR "/text_color"
#define GPM_SELECTED_TEXT_COLOR		GPM_COLORS_DIR "/selected_text_color"
#define GPM_SELECTION_COLOR		GPM_COLORS_DIR "/selection_color"

#define GPM_SAVE_DIR			GPM_PREFS_DIR  "/editor/save"
#define GPM_CREATE_BACKUP_COPY  	GPM_SAVE_DIR "/create_backup_copy"
#define GPM_BACKUP_COPY_EXTENSION	GPM_SAVE_DIR "/backup_copy_extension"

#define GPM_AUTO_SAVE			GPM_SAVE_DIR "/auto_save"
#define GPM_AUTO_SAVE_INTERVAL		GPM_SAVE_DIR "/auto_save_interval"

#define GPM_UNDO_DIR			GPM_PREFS_DIR  "/editor/undo"
#define GPM_UNDO_ACTIONS_LIMIT		GPM_UNDO_DIR "/undo_actions_limit"

#define GPM_WRAP_MODE_DIR		GPM_PREFS_DIR "/editor/wrap_mode"
#define GPM_WRAP_MODE			GPM_WRAP_MODE_DIR "/wrap_mode"

#define GPM_TABS_DIR			GPM_PREFS_DIR "/editor/tabs"
#define GPM_TABS_SIZE			GPM_TABS_DIR "/tabs_size"
#define GPM_INSERT_SPACES		GPM_TABS_DIR "/insert_spaces"

#define GPM_AUTO_INDENT_DIR		GPM_PREFS_DIR "/editor/auto_indent"
#define GPM_AUTO_INDENT			GPM_AUTO_INDENT_DIR "/auto_indent"

#define GPM_LINE_NUMBERS_DIR		GPM_PREFS_DIR "/editor/line_numbers"
#define GPM_DISPLAY_LINE_NUMBERS 	GPM_LINE_NUMBERS_DIR "/display_line_numbers"

#define GPM_RIGHT_MARGIN_DIR		GPM_PREFS_DIR "/editor/right_margin"
#define GPM_DISPLAY_RIGHT_MARGIN	GPM_RIGHT_MARGIN_DIR "/display_right_margin"
#define GPM_RIGHT_MARGIN_POSITION	GPM_RIGHT_MARGIN_DIR "/right_margin_position"


/* UI */
#define GPM_TOOLBAR_DIR			GPM_PREFS_DIR "/ui/toolbar"
#define GPM_TOOLBAR_VISIBLE	 	GPM_TOOLBAR_DIR "/toolbar_visible"
#define GPM_TOOLBAR_BUTTONS_STYLE 	GPM_TOOLBAR_DIR "/toolbar_buttons_style"

#define GPM_STATUSBAR_DIR		GPM_PREFS_DIR "/ui/statusbar"
#define GPM_STATUSBAR_VISIBLE		GPM_STATUSBAR_DIR "/statusbar_visible"

#define GRM_RECENTS_DIR			GPM_PREFS_DIR "/ui/recents"
#define GPM_MAX_RECENTS			GRM_RECENTS_DIR "/max_recents"

/* Print */
#define GPM_PRINT_PAGE_DIR		GPM_PREFS_DIR "/print/page"
#define GPM_PRINT_SYNTAX		GPM_PRINT_PAGE_DIR "/print_syntax_highlighting"
#define GPM_PRINT_HEADER		GPM_PRINT_PAGE_DIR "/print_header"
#define GPM_PRINT_WRAP_MODE		GPM_PRINT_PAGE_DIR "/print_wrap_mode"
#define GPM_PRINT_LINE_NUMBERS		GPM_PRINT_PAGE_DIR "/print_line_numbers"

#define GPM_PRINT_FONT_DIR		GPM_PREFS_DIR "/print/fonts"
#define GPM_PRINT_FONT_BODY		GPM_PRINT_FONT_DIR "/print_font_body"
#define GPM_PRINT_FONT_HEADER		GPM_PRINT_FONT_DIR "/print_font_header"
#define GPM_PRINT_FONT_NUMBERS		GPM_PRINT_FONT_DIR "/print_font_numbers"

#define GPM_WINDOW_DIR			"/gedit-2/window"
#define GPM_WINDOW_STATE		GPM_WINDOW_DIR "/state"
#define GPM_WINDOW_WIDTH		GPM_WINDOW_DIR "/width"
#define GPM_WINDOW_HEIGHT		GPM_WINDOW_DIR "/height"

/* Encodings */
#define GPM_ENCODINGS_DIR		GPM_PREFS_DIR "/encodings"
#define GPM_AUTO_DETECTED_ENCODINGS	GPM_ENCODINGS_DIR "/auto_detected"
#define GPM_SHOWN_IN_MENU_ENCODINGS	GPM_ENCODINGS_DIR "/shown_in_menu"

/* Syntax highlighting */
#define GPM_SYNTAX_HL_DIR		GPM_PREFS_DIR "/syntax_highlighting"
#define GPM_SYNTAX_HL_ENABLE		GPM_SYNTAX_HL_DIR "/enable"

/* Fallback default values. Keep in sync with gedit.schemas */

#define GPM_DEFAULT_USE_DEFAULT_FONT 	0 /* FALSE */
#define GPM_DEFAULT_EDITOR_FONT 	(const gchar*) "Monospace 12"

#define GPM_DEFAULT_USE_DEFAULT_COLORS 	1 /* TRUE */
#define GPM_DEFAULT_BACKGROUND_COLOR	(const gchar*) "#ffffffffffff"
#define GPM_DEFAULT_TEXT_COLOR		(const gchar*) "#000000000000"
#define GPM_DEFAULT_SELECTED_TEXT_COLOR	(const gchar*) "#ffffffffffff"
#define GPM_DEFAULT_SELECTION_COLOR	(const gchar*) "#000000009c9c"

#define GPM_DEFAULT_CREATE_BACKUP_COPY	1 /* TRUE */
#define GPM_DEFAULT_BACKUP_COPY_EXTENSION (const gchar*) "~"

#define GPM_DEFAULT_AUTO_SAVE		0 /* FALSE */
#define GPM_DEFAULT_AUTO_SAVE_INTERVAL	10 /* minutes */

#define GPM_DEFAULT_UNDO_ACTIONS_LIMIT	25 /* actions */

#define GPM_DEFAULT_WRAP_MODE		"GTK_WRAP_WORD"

#define GPM_DEFAULT_TABS_SIZE		8
#define GPM_DEFAULT_INSERT_SPACES	0 /* FALSE */

#define GPM_DEFAULT_AUTO_INDENT		0 /* FALSE */

#define GPM_DEFAULT_DISPLAY_LINE_NUMBERS 0 /* FALSE */

#define GPM_DEFAULT_AUTO_DETECTED_ENCODINGS {"UTF-8", "CURRENT", "ISO-8859-15", NULL}
       	
#define GPM_DEFAULT_TOOLBAR_VISIBLE	1 /* TRUE */
#define GPM_DEFAULT_TOOLBAR_BUTTONS_STYLE "GEDIT_TOOLBAR_SYSTEM"
#define GPM_DEFAULT_TOOLBAR_SHOW_TOOLTIPS 1 /* TRUE */

#define GPM_DEFAULT_STATUSBAR_VISIBLE	1 /* TRUE */

#define GPM_DEFAULT_PRINT_SYNTAX	1 /* TRUE */
#define GPM_DEFAULT_PRINT_HEADER	1 /* TRUE */
#define GPM_DEFAULT_PRINT_WRAP_MODE	"GTK_WRAP_WORD"
#define GPM_DEFAULT_PRINT_LINE_NUMBERS	0 /* No numbers */

#define GPM_DEFAULT_PRINT_FONT_BODY 	(const gchar*) "Monospace Regular 9"
#define GPM_DEFAULT_PRINT_FONT_HEADER	(const gchar*) "Sans Regular 11"
#define GPM_DEFAULT_PRINT_FONT_NUMBERS	(const gchar*) "Sans Regular 8"

#define GPM_DEFAULT_MAX_RECENTS		5

#define GPM_DEFAULT_WINDOW_STATE	0
#define GPM_DEFAULT_WINDOW_WIDTH	650
#define GPM_DEFAULT_WINDOW_HEIGHT	500

#define GPM_DEFAULT_WINDOW_STATE_STR	"0"
#define GPM_DEFAULT_WINDOW_WIDTH_STR	"650"
#define GPM_DEFAULT_WINDOW_HEIGHT_STR	"500"

#define GPM_DEFAULT_DISPLAY_RIGHT_MARGIN  0 /* FALSE */
#define GPM_DEFAULT_RIGHT_MARGIN_POSITION 80

#define GPM_DEFAULT_SYNTAX_HL_ENABLE	1 /* TRUE */

typedef enum {
	GEDIT_TOOLBAR_SYSTEM = 0,
	GEDIT_TOOLBAR_ICONS,
	GEDIT_TOOLBAR_ICONS_AND_TEXT,
	GEDIT_TOOLBAR_ICONS_BOTH_HORIZ
} GeditToolbarSetting;

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

/* Print syntax highlighting */
gboolean		 gedit_prefs_manager_get_print_syntax_hl	(void);
void			 gedit_prefs_manager_set_print_syntax_hl	(gboolean ps);
gboolean		 gedit_prefs_manager_print_syntax_hl_can_set	(void);

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

/* Encodings */
GSList 			*gedit_prefs_manager_get_auto_detected_encodings (void);

GSList			*gedit_prefs_manager_get_shown_in_menu_encodings (void);
void			 gedit_prefs_manager_set_shown_in_menu_encodings (const GSList *encs);
gboolean 		 gedit_prefs_manager_shown_in_menu_encodings_can_set (void);

/* Display right margin */
gboolean		 gedit_prefs_manager_get_display_right_margin	(void);
void			 gedit_prefs_manager_set_display_right_margin	(gboolean drm);
gboolean		 gedit_prefs_manager_display_right_margin_can_set (void);

/* Right margin position */
gint		 	 gedit_prefs_manager_get_right_margin_position	(void);
void 			 gedit_prefs_manager_set_right_margin_position	(gint rmp);
gboolean		 gedit_prefs_manager_right_margin_position_can_set (void);

/* Enable syntax highlighting */
gboolean 		 gedit_prefs_manager_get_enable_syntax_highlighting (void);
void			 gedit_prefs_manager_set_enable_syntax_highlighting (gboolean esh);
gboolean		 gedit_prefs_manager_enable_syntax_highlighting_can_set (void);

#endif  /* __GEDIT_PREFS_MANAGER_H__ */


