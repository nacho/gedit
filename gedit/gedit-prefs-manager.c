/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs-manager.c
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

#include <string.h>

#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-config.h>

#include "gedit-prefs-manager.h"
#include "gedit-debug.h"

#include "gedit-view.h"
#include "gedit-mdi.h"
#include "gedit2.h"
#include "gedit-encodings.h"

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

#define GPM_SAVE_ENCODING		GPM_SAVE_DIR "/save_encoding"

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

#define GPM_LOAD_DIR			GPM_PREFS_DIR "/editor/load"
#define GPM_ENCODINGS			GPM_LOAD_DIR "/encodings"

/* UI */
#define GPM_TOOLBAR_DIR			GPM_PREFS_DIR "/ui/toolbar"
#define GPM_TOOLBAR_VISIBLE	 	GPM_TOOLBAR_DIR "/toolbar_visible"
#define GPM_TOOLBAR_BUTTONS_STYLE 	GPM_TOOLBAR_DIR "/toolbar_buttons_style"

#define GPM_STATUSBAR_DIR		GPM_PREFS_DIR "/ui/statusbar"
#define GPM_STATUSBAR_VISIBLE		GPM_STATUSBAR_DIR "/statusbar_visible"
#define GPM_STATUSBAR_SHOW_CURSOR_POSITION GPM_STATUSBAR_DIR "/statusbar_show_cursor_position"
#define GPM_STATUSBAR_SHOW_OVERWRITE_MODE  GPM_STATUSBAR_DIR "/statusbar_show_overwrite_mode"

#define GRM_RECENTS_DIR			GPM_PREFS_DIR "/ui/recents"
#define GPM_MAX_RECENTS			GRM_RECENTS_DIR "/max_recents"

/* Print*/
#define GPM_PRINT_PAGE_DIR		GPM_PREFS_DIR "/print/page"
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

#define GPM_DEFAULT_SAVE_ENCODING	(const gchar*) "GEDIT_SAVE_ALWAYS_UTF8"

#define GPM_DEFAULT_UNDO_ACTIONS_LIMIT	25 /* actions */

#define GPM_DEFAULT_WRAP_MODE		"GTK_WRAP_WORD"

#define GPM_DEFAULT_TABS_SIZE		8
#define GPM_DEFAULT_INSERT_SPACES	0 /* FALSE */

#define GPM_DEFAULT_AUTO_INDENT		0 /* FALSE */

#define GPM_DEFAULT_DISPLAY_LINE_NUMBERS 0 /* FALSE */

#define GPM_DEFAULT_ENCODINGS		{"ISO-8859-15", NULL}
       	
#define GPM_DEFAULT_TOOLBAR_VISIBLE	1 /* TRUE */
#define GPM_DEFAULT_TOOLBAR_BUTTONS_STYLE "GEDIT_TOOLBAR_SYSTEM"
#define GPM_DEFAULT_TOOLBAR_SHOW_TOOLTIPS 1 /* TRUE */

#define GPM_DEFAULT_STATUSBAR_VISIBLE	1 /* TRUE */
#define GPM_DEFAULT_STATUSBAR_SHOW_CURSOR_POSITION 1 /* TRUE */
#define GPM_DEFAULT_STATUSBAR_SHOW_OVERWRITE_MODE 1 /* TRUE */

#define GPM_DEFAULT_PRINT_HEADER	1 /* TRUE */
#define GPM_DEFAULT_PRINT_WRAP_MODE	"GTK_WRAP_CHAR"
#define GPM_DEFAULT_PRINT_LINE_NUMBERS	0 /* No numbers */

#define GPM_DEFAULT_PRINT_FONT_BODY 	(const gchar*) "Monospace 9"
#define GPM_DEFAULT_PRINT_FONT_HEADER	(const gchar*) "Sans Normal 11"
#define GPM_DEFAULT_PRINT_FONT_NUMBERS	(const gchar*) "Sans Normal 8"

#define GPM_DEFAULT_MAX_RECENTS		5

#define GPM_DEFAULT_WINDOW_STATE	0
#define GPM_DEFAULT_WINDOW_WIDTH	650
#define GPM_DEFAULT_WINDOW_HEIGHT	500

#define GPM_DEFAULT_WINDOW_STATE_STR	"0"
#define GPM_DEFAULT_WINDOW_WIDTH_STR	"650"
#define GPM_DEFAULT_WINDOW_HEIGHT_STR	"500"



#define DEFINE_BOOL_PREF(name, key, def) gboolean 			\
gedit_prefs_manager_get_ ## name (void)					\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_get_bool (key,	\
					     (def));			\
}									\
									\
void 									\
gedit_prefs_manager_set_ ## name (gboolean v)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	gedit_prefs_manager_set_bool (key,		\
				      v);				\
}									\
				      					\
gboolean								\
gedit_prefs_manager_ ## name ## _can_set (void)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_key_is_writable (key);\
}	



#define DEFINE_INT_PREF(name, key, def) gint	 			\
gedit_prefs_manager_get_ ## name (void)			 		\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_get_int (key,		\
					    (def));			\
}									\
									\
void 									\
gedit_prefs_manager_set_ ## name (gint v)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	gedit_prefs_manager_set_int (key,		\
				     v);				\
}									\
				      					\
gboolean								\
gedit_prefs_manager_ ## name ## _can_set (void)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_key_is_writable (key);\
}		



#define DEFINE_STRING_PREF(name, key, def) gchar*	 		\
gedit_prefs_manager_get_ ## name (void)			 		\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_get_string (key,	\
					       def);			\
}									\
									\
void 									\
gedit_prefs_manager_set_ ## name (const gchar* v)			\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	gedit_prefs_manager_set_string (key,		\
				        v);				\
}									\
				      					\
gboolean								\
gedit_prefs_manager_ ## name ## _can_set (void)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_key_is_writable (key);\
}		


typedef struct _GeditPrefsManager 	GeditPrefsManager;

struct _GeditPrefsManager {
	GConfClient *gconf_client;
};

static GeditPrefsManager *gedit_prefs_manager = NULL;


static GtkWrapMode 	get_wrap_mode_from_string 		(const gchar* str);

static gboolean 	 gconf_client_get_bool_with_default 	(GConfClient* client, 
								 const gchar* key, 
								 gboolean def, 
								 GError** err);

static gchar		*gconf_client_get_string_with_default 	(GConfClient* client, 
								 const gchar* key,
								 const gchar* def, 
								 GError** err);

static gint		 gconf_client_get_int_with_default 	(GConfClient* client, 
								 const gchar* key,
								 gint def, 
								 GError** err);

static gboolean		 gedit_prefs_manager_get_bool		(const gchar* key, 
								 gboolean def);

static gint		 gedit_prefs_manager_get_int		(const gchar* key, 
								 gint def);

static gchar		*gedit_prefs_manager_get_string		(const gchar* key, 
								 const gchar* def);

gboolean		 gconf_client_set_color 		(GConfClient* client, 
								 const gchar* key,
								 GdkColor val, 
								 GError** err);


static void		gedit_prefs_manager_editor_font_changed	(GConfClient *client,
				   				 guint cnxn_id,
				   				 GConfEntry *entry,
				   				 gpointer user_data);

static void		gedit_prefs_manager_editor_colors_changed (GConfClient *client,
				   				   guint cnxn_id,
				   				   GConfEntry *entry,
				   				   gpointer user_data);
static void 		gedit_prefs_manager_tabs_size_changed 	(GConfClient *client,
							       	 guint cnxn_id, 
							       	 GConfEntry *entry, 
							       	 gpointer user_data);
static void 		gedit_prefs_manager_wrap_mode_changed 	(GConfClient *client,
								 guint cnxn_id, 
								 GConfEntry *entry, 
								 gpointer user_data);
static void 		gedit_prefs_manager_line_numbers_changed (GConfClient *client,
								  guint cnxn_id, 
								  GConfEntry *entry, 
								  gpointer user_data);

gboolean
gedit_prefs_manager_init (void)
{
	gedit_debug (DEBUG_PREFS, "");

	if (gedit_prefs_manager == NULL)
	{
		GConfClient *gconf_client;

		gconf_client = gconf_client_get_default ();
		if (gconf_client == NULL)
		{
			g_warning (_("Cannot initialize preferences manager."));
			return FALSE;
		}
		
		gedit_prefs_manager = g_new0 (GeditPrefsManager, 1);

		gedit_prefs_manager->gconf_client = gconf_client;

		/* TODO: notify, add dirs */
		gconf_client_add_dir (gedit_prefs_manager->gconf_client,
				GPM_PREFS_DIR,
				GCONF_CLIENT_PRELOAD_RECURSIVE,
				NULL);
		
		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_FONT_DIR,
				gedit_prefs_manager_editor_font_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_COLORS_DIR,
				gedit_prefs_manager_editor_colors_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_TABS_DIR,
				gedit_prefs_manager_tabs_size_changed,
				NULL, NULL, NULL);
		
		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_WRAP_MODE_DIR,
				gedit_prefs_manager_wrap_mode_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_LINE_NUMBERS_DIR,
				gedit_prefs_manager_line_numbers_changed,
				NULL, NULL, NULL);
	}

	if (gedit_prefs_manager->gconf_client == NULL)
	{
		g_free (gedit_prefs_manager);
		gedit_prefs_manager = NULL;
	}

	return gedit_prefs_manager != NULL;
	
}

/* This function must be called before exiting gedit */
void
gedit_prefs_manager_shutdown ()
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);

	gnome_config_sync ();

	g_object_unref (gedit_prefs_manager->gconf_client);
	gedit_prefs_manager->gconf_client = NULL;
}

static gboolean		 
gedit_prefs_manager_get_bool (const gchar* key, gboolean def)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, def);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, def);

	return gconf_client_get_bool_with_default (gedit_prefs_manager->gconf_client,
						   key,
						   def,
						   NULL);
}	

static gint 
gedit_prefs_manager_get_int (const gchar* key, gint def)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, def);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, def);

	return gconf_client_get_int_with_default (gedit_prefs_manager->gconf_client,
						  key,
						  def,
						  NULL);
}	

static gchar *
gedit_prefs_manager_get_string (const gchar* key, const gchar* def)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, 
			      def ? g_strdup (def) : NULL);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, 
			      def ? g_strdup (def) : NULL);

	return gconf_client_get_string_with_default (gedit_prefs_manager->gconf_client,
						     key,
						     def,
						     NULL);
}	

static void		 
gedit_prefs_manager_set_bool (const gchar* key, gboolean value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_bool (gedit_prefs_manager->gconf_client, key, value, NULL);
}

static void		 
gedit_prefs_manager_set_int (const gchar* key, gint value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_int (gedit_prefs_manager->gconf_client, key, value, NULL);
}

static void		 
gedit_prefs_manager_set_string (const gchar* key, const gchar* value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (value != NULL);
	
	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_string (gedit_prefs_manager->gconf_client, key, value, NULL);
}

static gboolean 
gedit_prefs_manager_key_is_writable (const gchar* key)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, FALSE);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, FALSE);

	return gconf_client_key_is_writable (gedit_prefs_manager->gconf_client, key, NULL);
}

static gchar* 
gdk_color_to_string (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");

	return g_strdup_printf ("#%04x%04x%04x",
				color.red, 
				color.green,
				color.blue);
}

static GdkColor
gconf_client_get_color_with_default (GConfClient* client, const gchar* key,
                      		     const gchar* def, GError** err)
{
	gchar *str_color = NULL;
	GdkColor color;
	
	gedit_debug (DEBUG_PREFS, "");

      	g_return_val_if_fail (client != NULL, color);
      	g_return_val_if_fail (GCONF_IS_CLIENT (client), color);  
	g_return_val_if_fail (key != NULL, color);
	g_return_val_if_fail (def != NULL, color);

	str_color = gconf_client_get_string_with_default (client, 
							  key, 
							  def,
							  NULL);

	g_return_val_if_fail (str_color != NULL, color);

	gdk_color_parse (str_color, &color);
	g_free (str_color);
	
	return color;
}

gboolean
gconf_client_set_color (GConfClient* client, const gchar* key,
                        GdkColor val, GError** err)
{
	gchar *str_color = NULL;
	gboolean res;
	
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (client != NULL, FALSE);
	g_return_val_if_fail (GCONF_IS_CLIENT (client), FALSE);  
	g_return_val_if_fail (key != NULL, FALSE);

	str_color = gdk_color_to_string (val);
	g_return_val_if_fail (str_color != NULL, FALSE);

	res = gconf_client_set_string (client,
				       key,
				       str_color,
				       err);
	g_free (str_color);

	return res;
}

static GdkColor
gedit_prefs_manager_get_color (const gchar* key, const gchar* def)
{
	GdkColor color;

	gedit_debug (DEBUG_PREFS, "");

	if (def != NULL)
		gdk_color_parse (def, &color);

	g_return_val_if_fail (gedit_prefs_manager != NULL, color);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, color);

	return gconf_client_get_color_with_default (gedit_prefs_manager->gconf_client,
						    key,
						    def,
						    NULL);
}	

static void		 
gedit_prefs_manager_set_color (const gchar* key, GdkColor value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_color (gedit_prefs_manager->gconf_client, key, value, NULL);
}


/* Use default font */
DEFINE_BOOL_PREF (use_default_font,
		  GPM_USE_DEFAULT_FONT,
		  GPM_DEFAULT_USE_DEFAULT_FONT)

/* Editor font */
DEFINE_STRING_PREF (editor_font,
		    GPM_EDITOR_FONT,
		    GPM_DEFAULT_EDITOR_FONT);


/* Use default colors */
DEFINE_BOOL_PREF (use_default_colors,
		  GPM_USE_DEFAULT_COLORS,
		  GPM_DEFAULT_USE_DEFAULT_COLORS);


/* Background color */	
GdkColor
gedit_prefs_manager_get_background_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_BACKGROUND_COLOR,
					      GPM_DEFAULT_BACKGROUND_COLOR);
}

void
gedit_prefs_manager_set_background_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_BACKGROUND_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_background_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_BACKGROUND_COLOR);
}
	
/* Text color */	
GdkColor
gedit_prefs_manager_get_text_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_TEXT_COLOR,
					      GPM_DEFAULT_TEXT_COLOR);
}

void
gedit_prefs_manager_set_text_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_TEXT_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_text_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_TEXT_COLOR);
}

/* Selected text color */	
GdkColor
gedit_prefs_manager_get_selected_text_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_SELECTED_TEXT_COLOR,
					      GPM_DEFAULT_SELECTED_TEXT_COLOR);
}

void
gedit_prefs_manager_set_selected_text_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_SELECTED_TEXT_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_selected_text_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_SELECTED_TEXT_COLOR);
}

/* Selection color */	
GdkColor
gedit_prefs_manager_get_selection_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_SELECTION_COLOR,
					      GPM_DEFAULT_SELECTION_COLOR);
}

void
gedit_prefs_manager_set_selection_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_SELECTION_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_selection_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_SELECTION_COLOR);
}


/* Create backup copy */
DEFINE_BOOL_PREF (create_backup_copy,
		  GPM_CREATE_BACKUP_COPY,
		  GPM_DEFAULT_CREATE_BACKUP_COPY);

/* Backup extension. This is configurable only using gconftool or gconf-editor */
gchar*	 		\
gedit_prefs_manager_get_backup_extension (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_string (GPM_BACKUP_COPY_EXTENSION,	
					       GPM_DEFAULT_BACKUP_COPY_EXTENSION);
}

/* Auto save */
DEFINE_BOOL_PREF (auto_save,
		  GPM_AUTO_SAVE,
		  GPM_DEFAULT_AUTO_SAVE);

/* Auto save interval */
DEFINE_INT_PREF (auto_save_interval,
		 GPM_AUTO_SAVE_INTERVAL,
		 GPM_DEFAULT_AUTO_SAVE_INTERVAL);


/* Save encoding */
GeditSaveEncodingSetting 
gedit_prefs_manager_get_save_encoding (void)
{
	gchar *str;
	GeditSaveEncodingSetting res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_SAVE_ENCODING,
					      GPM_DEFAULT_SAVE_ENCODING);

	if (strcmp (str, "GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE") == 0)
		res = GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE;
	else
	{
		if (strcmp (str, "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE") == 0)
			res = GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE;
		else
		{
			if (strcmp (str, "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL") == 0)
				res = GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL;
			else	
				res = GEDIT_SAVE_ALWAYS_UTF8;
		}
	}

	g_free (str);

	return res;
}


void
gedit_prefs_manager_set_save_encoding (GeditSaveEncodingSetting se)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (se)
	{
		case GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE:
			str = "GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE";
			break;

		case GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE:
			str = "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE";
			break;

		case GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL:
			str = "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL";
			break;

		default: /* GEDIT_SAVE_ALWAYS_UTF8 */
			str = "GEDIT_SAVE_ALWAYS_UTF8";
	}

	gedit_prefs_manager_set_string (GPM_SAVE_ENCODING,
					str);
}

gboolean
gedit_prefs_manager_save_encoding_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_SAVE_ENCODING);
}

/* Undo actions limit: if < 1 then no limits */
DEFINE_INT_PREF (undo_actions_limit,
		 GPM_UNDO_ACTIONS_LIMIT,
		 GPM_DEFAULT_UNDO_ACTIONS_LIMIT)

static GtkWrapMode 
get_wrap_mode_from_string (const gchar* str)
{
	GtkWrapMode res;

	g_return_val_if_fail (str != NULL, GTK_WRAP_WORD);
	
	if (strcmp (str, "GTK_WRAP_NONE") == 0)
		res = GTK_WRAP_NONE;
	else
	{
		if (strcmp (str, "GTK_WRAP_CHAR") == 0)
			res = GTK_WRAP_CHAR;
		else
			res = GTK_WRAP_WORD;
	}

	return res;
}

/* Wrap mode */
GtkWrapMode
gedit_prefs_manager_get_wrap_mode (void)
{
	gchar *str;
	GtkWrapMode res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_WRAP_MODE,
					      GPM_DEFAULT_WRAP_MODE);

	res = get_wrap_mode_from_string (str);

	g_free (str);

	return res;
}
	
void
gedit_prefs_manager_set_wrap_mode (GtkWrapMode wp)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (wp)
	{
		case GTK_WRAP_NONE:
			str = "GTK_WRAP_NONE";
			break;

		case GTK_WRAP_CHAR:
			str = "GTK_WRAP_CHAR";
			break;

		default: /* GTK_WRAP_WORD */
			str = "GTK_WRAP_WORD";
	}

	gedit_prefs_manager_set_string (GPM_WRAP_MODE,
					str);
}

	
gboolean
gedit_prefs_manager_wrap_mode_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_WRAP_MODE);
}


/* Tabs size */
DEFINE_INT_PREF (tabs_size, 
		 GPM_TABS_SIZE, 
		 GPM_DEFAULT_TABS_SIZE);

/* Insert spaces */
DEFINE_BOOL_PREF (insert_spaces, 
		  GPM_INSERT_SPACES, 
		  GPM_DEFAULT_INSERT_SPACES)

/* Auto indent */
DEFINE_BOOL_PREF (auto_indent, 
		  GPM_AUTO_INDENT, 
		  GPM_DEFAULT_AUTO_INDENT)

/* Display line numbers */
DEFINE_BOOL_PREF (display_line_numbers, 
		  GPM_DISPLAY_LINE_NUMBERS, 
		  GPM_DEFAULT_DISPLAY_LINE_NUMBERS)

/* Toolbar visibility */
DEFINE_BOOL_PREF (toolbar_visible,
		  GPM_TOOLBAR_VISIBLE,
		  GPM_DEFAULT_TOOLBAR_VISIBLE);


/* Toolbar suttons style */
GeditToolbarSetting 
gedit_prefs_manager_get_toolbar_buttons_style (void) 
{
	gchar *str;
	GeditToolbarSetting res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_TOOLBAR_BUTTONS_STYLE,
					      GPM_DEFAULT_TOOLBAR_BUTTONS_STYLE);

	if (strcmp (str, "GEDIT_TOOLBAR_ICONS") == 0)
		res = GEDIT_TOOLBAR_ICONS;
	else
	{
		if (strcmp (str, "GEDIT_TOOLBAR_ICONS_AND_TEXT") == 0)
			res = GEDIT_TOOLBAR_ICONS_AND_TEXT;
		else 
		{
			if (strcmp (str, "GEDIT_TOOLBAR_ICONS_BOTH_HORIZ") == 0)
				res = GEDIT_TOOLBAR_ICONS_BOTH_HORIZ;
			else
				res = GEDIT_TOOLBAR_SYSTEM;
		}
	}

	g_free (str);

	return res;

}

void
gedit_prefs_manager_set_toolbar_buttons_style (GeditToolbarSetting tbs)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (tbs)
	{
		case GEDIT_TOOLBAR_ICONS:
			str = "GEDIT_TOOLBAR_ICONS";
			break;

		case GEDIT_TOOLBAR_ICONS_AND_TEXT:
			str = "GEDIT_TOOLBAR_ICONS_AND_TEXT";
			break;

	        case GEDIT_TOOLBAR_ICONS_BOTH_HORIZ:
			str = "GEDIT_TOOLBAR_ICONS_BOTH_HORIZ";
			break;
		default: /* GEDIT_TOOLBAR_SYSTEM */
			str = "GEDIT_TOOLBAR_SYSTEM";
	}

	gedit_prefs_manager_set_string (GPM_TOOLBAR_BUTTONS_STYLE,
					str);

}

gboolean
gedit_prefs_manager_toolbar_buttons_style_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_TOOLBAR_BUTTONS_STYLE);

}

/* Statusbar visiblity */
DEFINE_BOOL_PREF (statusbar_visible,
		  GPM_STATUSBAR_VISIBLE,
		  GPM_DEFAULT_STATUSBAR_VISIBLE)

/* Show cursor position in statusbar */
DEFINE_BOOL_PREF (statusbar_show_cursor_position,
		  GPM_STATUSBAR_SHOW_CURSOR_POSITION,
		  GPM_DEFAULT_STATUSBAR_SHOW_CURSOR_POSITION)

/* Show overwrite mode in statusbar */
DEFINE_BOOL_PREF (statusbar_show_overwrite_mode,
		  GPM_STATUSBAR_SHOW_OVERWRITE_MODE,
		  GPM_DEFAULT_STATUSBAR_SHOW_OVERWRITE_MODE)

/* Print header */
DEFINE_BOOL_PREF (print_header,
		  GPM_PRINT_HEADER,
		  GPM_DEFAULT_PRINT_HEADER)


/* Print Wrap mode */
GtkWrapMode
gedit_prefs_manager_get_print_wrap_mode (void)
{
	gchar *str;
	GtkWrapMode res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_PRINT_WRAP_MODE,
					      GPM_DEFAULT_PRINT_WRAP_MODE);

	if (strcmp (str, "GTK_WRAP_NONE") == 0)
		res = GTK_WRAP_NONE;
	else
	{
		if (strcmp (str, "GTK_WRAP_WORD") == 0)
			res = GTK_WRAP_WORD;
		else
			res = GTK_WRAP_CHAR;
	}

	g_free (str);

	return res;
}
	
void
gedit_prefs_manager_set_print_wrap_mode (GtkWrapMode pwp)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (pwp)
	{
		case GTK_WRAP_NONE:
			str = "GTK_WRAP_NONE";
			break;

		case GTK_WRAP_WORD:
			str = "GTK_WRAP_WORD";
			break;

		default: /* GTK_WRAP_CHAR */
			str = "GTK_WRAP_CHAR";
	}

	gedit_prefs_manager_set_string (GPM_PRINT_WRAP_MODE,
					str);
}

	
gboolean
gedit_prefs_manager_print_wrap_mode_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_PRINT_WRAP_MODE);
}

/* Print line numbers */	
DEFINE_INT_PREF (print_line_numbers,
		 GPM_PRINT_LINE_NUMBERS,
		 GPM_DEFAULT_PRINT_LINE_NUMBERS)
		 

/* Font used to print the body of documents */
DEFINE_STRING_PREF (print_font_body,
		    GPM_PRINT_FONT_BODY,
		    GPM_DEFAULT_PRINT_FONT_BODY);

const gchar *
gedit_prefs_manager_get_default_print_font_body (void)
{
	return GPM_DEFAULT_PRINT_FONT_BODY;
}

/* Font used to print headers */
DEFINE_STRING_PREF (print_font_header,
		    GPM_PRINT_FONT_HEADER,
		    GPM_DEFAULT_PRINT_FONT_HEADER);

const gchar *
gedit_prefs_manager_get_default_print_font_header (void)
{
	return GPM_DEFAULT_PRINT_FONT_HEADER;
}

/* Font used to print line numbers */
DEFINE_STRING_PREF (print_font_numbers,
		    GPM_PRINT_FONT_NUMBERS,
		    GPM_DEFAULT_PRINT_FONT_NUMBERS);

const gchar *
gedit_prefs_manager_get_default_print_font_numbers (void)
{
	return GPM_DEFAULT_PRINT_FONT_NUMBERS;
}


/* Max number of files in "Recent Files" menu. 
 * This is configurable only using gconftool or gconf-editor 
 */
gint
gedit_prefs_manager_get_max_recents (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_int (GPM_MAX_RECENTS,	
					    GPM_DEFAULT_MAX_RECENTS);

}



static void 
gedit_prefs_manager_editor_font_changed (GConfClient *client,
	guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	GList *children;
	gchar *font = NULL;
	gboolean def = TRUE;
	
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_USE_DEFAULT_FONT) == 0)
	{
		if (entry->value->type == GCONF_VALUE_BOOL)
			def = gconf_value_get_bool (entry->value);
		else
			def = GPM_DEFAULT_USE_DEFAULT_FONT;
		
		font = NULL;
	}
	else
	if (strcmp (entry->key, GPM_EDITOR_FONT) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			font = g_strdup (gconf_value_get_string (entry->value));
		else
			font = g_strdup (GPM_DEFAULT_EDITOR_FONT);
				
		def = gedit_prefs_manager_get_use_default_font ();
	}
	else
		return;
	
	if ((font == NULL) && !def)
		font = gedit_prefs_manager_get_editor_font ();
	
	children = bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi));

	while (children != NULL)
	{
		gint ts;
		
		GList *views = bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (children->data));

		ts = gedit_prefs_manager_get_tabs_size ();
		while (views != NULL)
		{
			GeditView *v =	GEDIT_VIEW (views->data);
			
			gedit_view_set_font (v, def, font);
			gedit_view_set_tab_size (v, ts);

			views = views->next;
		}
		
		children = children->next;
	}

	if (font != NULL)
		g_free (font);
}


static void 
set_colors (gboolean def, GdkColor* backgroud, GdkColor* text,
		GdkColor* selection, GdkColor* sel_text)
{
	GList *children;

	children = bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi));

	while (children != NULL)
	{
		GList *views = bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (children->data));

		while (views != NULL)
		{
			GeditView *v =	GEDIT_VIEW (views->data);
			
			gedit_view_set_colors (v, 
					       def,
					       backgroud,
					       text,
					       selection,
					       sel_text);
			views = views->next;
		}
		
		children = children->next;
	}
}

static void 
gedit_prefs_manager_editor_colors_changed (GConfClient *client,
	guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	gboolean def = TRUE;
	gchar *str_color;
	GdkColor color;

	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_USE_DEFAULT_COLORS) == 0)
	{
		if (entry->value->type == GCONF_VALUE_BOOL)
			def = gconf_value_get_bool (entry->value);
		else
			def = GPM_DEFAULT_USE_DEFAULT_COLORS;

		if (def)
			set_colors (TRUE, NULL, NULL, NULL, NULL);
		else
		{
			GdkColor background, text, selection, sel_text;

			background = gedit_prefs_manager_get_background_color ();
			text = gedit_prefs_manager_get_text_color ();
			selection = gedit_prefs_manager_get_selection_color ();
			sel_text = gedit_prefs_manager_get_selected_text_color ();

			set_colors (FALSE,
				    &background, 
				    &text, 
				    &selection, 
				    &sel_text);
		}

		return;
	}
	
	if (strcmp (entry->key, GPM_BACKGROUND_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_BACKGROUND_COLOR);
				
		def = gedit_prefs_manager_get_use_default_colors ();

		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (def, &color, NULL, NULL, NULL);
	
		return;
	}

	if (strcmp (entry->key, GPM_TEXT_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_TEXT_COLOR);
				
		def = gedit_prefs_manager_get_use_default_colors ();

		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (def, NULL, &color, NULL, NULL);
	
		return;
	}

	if (strcmp (entry->key, GPM_SELECTION_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_SELECTION_COLOR);
				
		def = gedit_prefs_manager_get_use_default_colors ();

		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (def, NULL, NULL, &color, NULL);
	
		return;
	}

	if (strcmp (entry->key, GPM_SELECTED_TEXT_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_SELECTED_TEXT_COLOR);

		def = gedit_prefs_manager_get_use_default_colors ();

		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (def, NULL, NULL, NULL, &color);
	
		return;
	}
}

static void 
gedit_prefs_manager_tabs_size_changed (GConfClient *client,
	guint cnxn_id, GConfEntry *entry, gpointer user_data)
{

	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_TABS_SIZE) == 0)
	{
		gint tabs_size;
		GList *children;
		
		if (entry->value->type == GCONF_VALUE_INT)
			tabs_size = gconf_value_get_int (entry->value);
		else
			tabs_size = GPM_DEFAULT_TABS_SIZE;
	
		tabs_size = CLAMP (tabs_size, 1, 24);

		children = bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi));

		while (children != NULL)
		{
			GList *views = bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (children->data));

			while (views != NULL)
			{
				GeditView *v =	GEDIT_VIEW (views->data);
			
				gedit_view_set_tab_size (v, tabs_size);
			
				views = views->next;
			}
		
			children = children->next;
		}
	}
}

static void 
gedit_prefs_manager_wrap_mode_changed (GConfClient *client,
	guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_WRAP_MODE) == 0)
	{
		GtkWrapMode wrap_mode;
			
		GList *children;
		
		if (entry->value->type == GCONF_VALUE_STRING)
			wrap_mode = 
				get_wrap_mode_from_string (gconf_value_get_string (entry->value));	
		else
			wrap_mode = get_wrap_mode_from_string (GPM_DEFAULT_WRAP_MODE);
	
		children = bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi));

		while (children != NULL)
		{
			GList *views = bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (children->data));

			while (views != NULL)
			{
				GeditView *v =	GEDIT_VIEW (views->data);
			
				gedit_view_set_wrap_mode (v, wrap_mode);
			
				views = views->next;
			}
		
			children = children->next;
		}
	}

}

static void 
gedit_prefs_manager_line_numbers_changed (GConfClient *client,
	guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_DISPLAY_LINE_NUMBERS) == 0)
	{
		gboolean dln;
			
		GList *children;
		
		if (entry->value->type == GCONF_VALUE_BOOL)
			dln = gconf_value_get_bool (entry->value);	
		else
			dln = GPM_DEFAULT_DISPLAY_LINE_NUMBERS;
	
		children = bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi));

		while (children != NULL)
		{
			GList *views = bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (children->data));

			while (views != NULL)
			{
				GeditView *v =	GEDIT_VIEW (views->data);
			
				gedit_view_show_line_numbers (v, dln);
			
				views = views->next;
			}
		
			children = children->next;
		}
	}
}

static gint window_state = -1;
static gint window_height = -1;
static gint window_width = -1;

/* Window state */
gint gedit_prefs_manager_get_window_state (void)
{
	if (window_state == -1)
		window_state = gnome_config_get_int (GPM_WINDOW_STATE "=" GPM_DEFAULT_WINDOW_STATE_STR);

	return window_state;
}
			
void
gedit_prefs_manager_set_window_state (gint ws)
{
	g_return_if_fail (ws != -1);
	
	window_state = ws;
	gnome_config_set_int (GPM_WINDOW_STATE, ws);
}

gboolean
gedit_prefs_manager_window_state_can_set (void)
{
	return TRUE;
}

/* Window height */
gint
gedit_prefs_manager_get_window_height (void)
{
	if (window_height == -1)
		window_height = gnome_config_get_int (GPM_WINDOW_HEIGHT "=" GPM_DEFAULT_WINDOW_HEIGHT_STR);

	return window_height;
}


gint
gedit_prefs_manager_get_default_window_height (void)
{
	return GPM_DEFAULT_WINDOW_HEIGHT;
}

void gedit_prefs_manager_set_window_height (gint wh)
{
	g_return_if_fail (wh != -1);

	window_height = wh;
	gnome_config_set_int (GPM_WINDOW_HEIGHT, wh);
}

gboolean 
gedit_prefs_manager_window_height_can_set (void)
{
	return TRUE;
}

/* Window width */
gint
gedit_prefs_manager_get_window_width (void)
{
	if (window_width == -1)
		window_width = gnome_config_get_int (GPM_WINDOW_WIDTH "=" GPM_DEFAULT_WINDOW_WIDTH_STR);

	return window_width;
}

gint 
gedit_prefs_manager_get_default_window_width (void)
{
	return GPM_DEFAULT_WINDOW_WIDTH;
}

void 
gedit_prefs_manager_set_window_width (gint ww)
{
	g_return_if_fail (ww != -1);
	
	window_width = ww;
	gnome_config_set_int (GPM_WINDOW_WIDTH, ww);
}

gboolean 
gedit_prefs_manager_window_width_can_set (void)
{
	return TRUE;
}

void
gedit_prefs_manager_save_window_size_and_state (BonoboWindow *window)
{
	const BonoboMDIWindowInfo *window_info;

	gedit_debug (DEBUG_PREFS, "");
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (BONOBO_IS_WINDOW (window));

	window_info = bonobo_mdi_get_window_info (window);
	g_return_if_fail (window_info != NULL);
	
	if (gedit_prefs_manager_window_height_can_set ())
		gedit_prefs_manager_set_window_height (window_info->height);

	if (gedit_prefs_manager_window_width_can_set ())
		gedit_prefs_manager_set_window_width (window_info->width);

	if (gedit_prefs_manager_window_state_can_set ())
		gedit_prefs_manager_set_window_state (window_info->state);
}


/* Encodings */
GSList *
gedit_prefs_manager_get_encodings (void)
{
	GSList *strings;
	GSList *res = NULL;
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, NULL);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, NULL);

	strings = gconf_client_get_list (gedit_prefs_manager->gconf_client,
				GPM_ENCODINGS,
				GCONF_VALUE_STRING, 
				NULL);

	if (strings != NULL)
	{	
		GSList *tmp;
		const GeditEncoding *enc;

		tmp = strings;
		
		while (tmp)
		{
		      const char *charset = tmp->data;

		      if (strcmp (charset, "current") == 0)
			      g_get_charset (&charset);
      
		      g_return_val_if_fail (charset != NULL, NULL);
		      enc = gedit_encoding_get_from_charset (charset);
		      
		      if (enc != NULL)
				res = g_slist_prepend (res, (gpointer)enc);

		      tmp = g_slist_next (tmp);
		}

		g_slist_foreach (strings, (GFunc) g_free, NULL);
		g_slist_free (strings);    

	 	res = g_slist_reverse (res);
	}
	return res;
}

void
gedit_prefs_manager_set_encodings (const GSList *encs)
{	
	GSList *list = NULL;
	
	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gedit_prefs_manager_encodings_can_set ());

	while (encs != NULL)
	{
		const GeditEncoding *enc;
		const gchar *charset;
		
		enc = (const GeditEncoding *)encs->data;

		charset = gedit_encoding_get_charset (enc);
		g_return_if_fail (charset != NULL);

		list = g_slist_prepend (list, (gpointer)charset);

		encs = g_slist_next (encs);
	}

	list = g_slist_reverse (list);
		
	gconf_client_set_list (gedit_prefs_manager->gconf_client,
			GPM_ENCODINGS,
			GCONF_VALUE_STRING,
		       	list,
			NULL);	
	
	g_slist_free (list);
}

gboolean
gedit_prefs_manager_encodings_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_ENCODINGS);

}


/* The following functions are taken from gconf-client.c 
 * and partially modified. 
 * The licensing terms on these is: 
 *
 * 
 * GConf
 * Copyright (C) 1999, 2000, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


static const gchar* 
gconf_value_type_to_string(GConfValueType type)
{
  switch (type)
    {
    case GCONF_VALUE_INT:
      return "int";
      break;
    case GCONF_VALUE_STRING:
      return "string";
      break;
    case GCONF_VALUE_FLOAT:
      return "float";
      break;
    case GCONF_VALUE_BOOL:
      return "bool";
      break;
    case GCONF_VALUE_SCHEMA:
      return "schema";
      break;
    case GCONF_VALUE_LIST:
      return "list";
      break;
    case GCONF_VALUE_PAIR:
      return "pair";
      break;
    case GCONF_VALUE_INVALID:
      return "*invalid*";
      break;
    default:
      g_assert_not_reached();
      return NULL; /* for warnings */
      break;
    }
}

/* Emit the proper signals for the error, and fill in err */
static gboolean
handle_error (GConfClient* client, GError* error, GError** err)
{
  if (error != NULL)
    {
      gconf_client_error(client, error);
      
      if (err == NULL)
        {
          gconf_client_unreturned_error(client, error);

          g_error_free(error);
        }
      else
        *err = error;

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
check_type(const gchar* key, GConfValue* val, GConfValueType t, GError** err)
{
  if (val->type != t)
    {
      /*
      gconf_set_error(err, GCONF_ERROR_TYPE_MISMATCH,
                      _("Expected `%s' got `%s' for key %s"),
                      gconf_value_type_to_string(t),
                      gconf_value_type_to_string(val->type),
                      key);
      */
      g_set_error (err, GCONF_ERROR, GCONF_ERROR_TYPE_MISMATCH,
	  	   _("Expected `%s' got `%s' for key %s"),
                   gconf_value_type_to_string(t),
                   gconf_value_type_to_string(val->type),
                   key);
	      
      return FALSE;
    }
  else
    return TRUE;
}

static gboolean
gconf_client_get_bool_with_default (GConfClient* client, const gchar* key,
                        	    gboolean def, GError** err)
{
  GError* error = NULL;
  GConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  val = gconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gboolean retval = def;

      g_assert (error == NULL);
      
      if (check_type (key, val, GCONF_VALUE_BOOL, &error))
        retval = gconf_value_get_bool (val);
      else
        handle_error (client, error, err);

      gconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}

static gchar*
gconf_client_get_string_with_default (GConfClient* client, const gchar* key,
                        	      const gchar* def, GError** err)
{
  GError* error = NULL;
  gchar* val;

  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  val = gconf_client_get_string (client, key, &error);

  if (val != NULL)
    {
      g_assert(error == NULL);
      
      return val;
    }
  else
    {
      if (error != NULL)
        *err = error;
      return def ? g_strdup (def) : NULL;
    }
}

static gint
gconf_client_get_int_with_default (GConfClient* client, const gchar* key,
                        	   gint def, GError** err)
{
  GError* error = NULL;
  GConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, 0);

  val = gconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gint retval = def;

      g_assert(error == NULL);
      
      if (check_type (key, val, GCONF_VALUE_INT, &error))
        retval = gconf_value_get_int(val);
      else
        handle_error (client, error, err);

      gconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}



