/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs.c
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


/* TODO: 
 * [X] It should be rewritten to use GConf 
 * [X] check if all the preferences are really used in the code 
 */

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include <bonobo-mdi.h>
#include <gconf/gconf-client.h>
#include "gedit-prefs.h"
#include "gedit-debug.h"
#include "gedit2.h"


#define GEDIT_BASE_KEY "/apps/gedit2"

#define GEDIT_PREF_USE_DEFAULT_FONT	"/use-default-font"
#define GEDIT_PREF_EDITOR_FONT		"/editor-font"

#define GEDIT_PREF_USE_DEFAULT_COLORS	"/use-default-colors"
#define GEDIT_PREF_BACKGROUND_COLOR	"/background-color"
#define GEDIT_PREF_TEXT_COLOR		"/text-color"
#define GEDIT_PREF_SELECTED_TEXT_COLOR	"/selected-text-color"
#define GEDIT_PREF_SELECTION_COLOR	"/selection-color"

#define GEDIT_PREFS_CREATE_BACKUP_COPY  "/create-backup-copy"
#define GEDIT_PREFS_BACKUP_EXTENSION  	"/backup-copy-extension"

#define GEDIT_PREFS_AUTO_SAVE		"/auto-save"
#define GEDIT_PREFS_AUTO_SAVE_INTERVAL	"/auto-save-interval"

#define GEDIT_PREFS_SAVE_ENCODING	"/save-encoding"

#define GEDIT_PREF_UNDO_LEVELS		"/undo-levels"

#define GEDIT_PREF_WRAP_MODE		"/wrap-mode"

#define GEDIT_PREF_TAB_SIZE		"/tab-size"

#define GEDIT_PREF_TOOLBAR_VISIBLE	 "/toolbar-visible"
#define GEDIT_PREF_TOOLBAR_BUTTONS_STYLE "/toolbar-buttons-style"
#define GEDIT_PREF_TOOLBAR_VIEW_TOOLTIPS "/toolbar-view-tooltips"

#define GEDIT_PREF_STATUSBAR_VISIBLE	"/statusbar-visible"

#define GEDIT_PREF_MDI_MODE		"/mdi-mode"
#define GEDIT_PREF_TABS_POSITION	"/mdi-tabs-position"

#define GEDIT_PREF_WINDOW_WIDTH		"/window_width"
#define GEDIT_PREF_WINDOW_HEIGHT	"/window_height"

#if 0
#define GEDIT_PREF_PRINTWRAP		"/printwrap"
#define GEDIT_PREF_PRINTHEADER		"/printheader"
#define GEDIT_PREF_PRINTLINES		"/printlines"
#define GEDIT_PREF_PRINTORIENTATION	"/print-orientation"
#define GEDIT_PREF_PAPERSIZE		"/papersize"
#endif

GeditPreferences 	*gedit_settings 	= NULL;
static GConfClient 	*gedit_gconf_client 	= NULL;

#define DEFAULT_EDITOR_FONT 		(const gchar*) "Courier Medium 12"

#define DEFAULT_PRINT_FONT_BODY 	(const gchar*) "Courier 10"
#define DEFAULT_PRINT_FONT_H_AND_F	(const gchar*) "Helvetica 12"
#define DEFAULT_PRINT_FONT_NUMBERS	(const gchar*) "Courier 8"

static gchar* 
gedit_prefs_gdk_color_to_string (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");

	return g_strdup_printf ("#%04x%04x%04x",
				color.red, 
				color.green,
				color.blue);
}

void 
gedit_prefs_save_settings (void)
{
#if 0
	BonoboWindow* active_window;
	GdkWindow *toplevel;
	gint root_x;
	gint root_y;
#endif	
	gchar *str_color = NULL;

	gedit_debug (DEBUG_PREFS, "START");
	
	g_return_if_fail (gedit_gconf_client != NULL);
	g_return_if_fail (gedit_settings != NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_FONT,
				gedit_settings->use_default_font,
				NULL);

	g_return_if_fail (gedit_settings->editor_font != NULL);
	gconf_client_set_string (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_EDITOR_FONT,
			      	gedit_settings->editor_font,
			      	NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_COLORS,
				gedit_settings->use_default_colors,
				NULL);

	/* Backgroung color */
	str_color = gedit_prefs_gdk_color_to_string (
				gedit_settings->background_color);
	g_return_if_fail (str_color != NULL);

	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_BACKGROUND_COLOR,
			      	str_color,
			      	NULL);

	g_free (str_color);
	
	/* Text color */
	str_color = gedit_prefs_gdk_color_to_string (
				gedit_settings->text_color);
	g_return_if_fail (str_color != NULL);

	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_TEXT_COLOR,
			      	str_color,
			      	NULL);

	g_free (str_color);

	/* Selection color */
	str_color = gedit_prefs_gdk_color_to_string (
				gedit_settings->selection_color);
	g_return_if_fail (str_color != NULL);

	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_SELECTION_COLOR,
			      	str_color,
			      NULL);

	g_free (str_color);

	/* Selected text color */
	str_color = gedit_prefs_gdk_color_to_string (
				gedit_settings->selected_text_color);
	g_return_if_fail (str_color != NULL);

	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_SELECTED_TEXT_COLOR,
			      	str_color,
			      NULL);

	g_free (str_color);

	gconf_client_set_bool (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREFS_CREATE_BACKUP_COPY,
			      	gedit_settings->create_backup_copy,
			      	NULL);

	g_return_if_fail (gedit_settings->backup_extension != NULL);
	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREFS_BACKUP_EXTENSION,
			      	gedit_settings->backup_extension,
			      	NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREFS_AUTO_SAVE,
			      	gedit_settings->auto_save,
			      	NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREFS_AUTO_SAVE_INTERVAL,
			      	gedit_settings->auto_save_interval,
			      	NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREFS_SAVE_ENCODING,
			      	gedit_settings->save_encoding,
			      	NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_UNDO_LEVELS,
				gedit_settings->undo_levels,
			      	NULL);

	gconf_client_set_int  (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_WRAP_MODE,
				gedit_settings->wrap_mode,
				NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TAB_SIZE,
				gedit_settings->tab_size,
			      	NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_VISIBLE,
				gedit_settings->toolbar_visible,
				NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_BUTTONS_STYLE,
				gedit_settings->toolbar_buttons_style,
			      	NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_VIEW_TOOLTIPS,
				gedit_settings->toolbar_view_tooltips,
				NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VISIBLE,
				gedit_settings->statusbar_visible,
				NULL);
	
	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_MDI_MODE,
				gedit_settings->mdi_mode,
				NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TABS_POSITION,
				gedit_settings->mdi_tabs_position,
				NULL);

	
#if 0 /* FIXME */
	
	active_window = bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi));
	if (active_window) 
	{
		toplevel = gdk_window_get_toplevel (GTK_WIDGET (active_window)->window);
		gdk_window_get_root_origin (toplevel, &root_x, &root_y);
		/* We don't want to save the size of a maximized window,
		   so chek the left margin. This will not work if there is
		   a pannel in left, but there is no other way that I know
		   of knowing if a window is maximzed or not */
		if (root_x != 0)
			if (active_window)
				gdk_window_get_size (GTK_WIDGET (active_window)->window,
						     &settings->width, &settings->height);
		gconf_client_set_int (gedit_gconf_client,
				      GEDIT_BASE_KEY GEDIT_PREF_WIDTH,
				      settings->width,
				      NULL);

		gconf_client_set_int (gedit_gconf_client,
				      GEDIT_BASE_KEY GEDIT_PREF_HEIGHT,
				      settings->height,
				      NULL);
	}
#endif	
	gconf_client_suggest_sync (gedit_gconf_client, NULL);
	
#if 0 /* FIXME */
	gedit_plugin_save_settings ();
#endif	
	gedit_debug (DEBUG_PREFS, "END");
}

void
gedit_prefs_load_settings (void)
{
	gchar *str_color = NULL;

	gedit_debug (DEBUG_PREFS, "START");
	g_return_if_fail (gedit_gconf_client != NULL);

	if (!gedit_settings)
	{
		gedit_settings = g_new0 (GeditPreferences, 1);
	}

	gedit_settings->use_default_font = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_FONT,
				NULL);
	
	if (gedit_settings->editor_font != NULL)
		g_free (gedit_settings->editor_font);

	gedit_settings->editor_font = gconf_client_get_string (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_EDITOR_FONT,
			      	NULL);
	if (gedit_settings->editor_font == NULL)
		gedit_settings->editor_font = g_strdup (DEFAULT_EDITOR_FONT);
	
	gedit_settings->use_default_colors = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_COLORS,
				NULL);

	/* Backgroung color */	
	str_color = gconf_client_get_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_BACKGROUND_COLOR,
			      	NULL);
	
	if (str_color != NULL)
	{
		gdk_color_parse (str_color, &gedit_settings->background_color);
		
		g_free (str_color);
		str_color = NULL;
	}
	
	/* Text color */	
	str_color = gconf_client_get_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_TEXT_COLOR,
			      	NULL);
	
	if (str_color != NULL)
	{
		gdk_color_parse (str_color, &gedit_settings->text_color);
		
		g_free (str_color);
		str_color = NULL;
	}

	/* Selection color */	
	str_color = gconf_client_get_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_SELECTION_COLOR,
			      	NULL);
	
	if (str_color != NULL)
	{
		gdk_color_parse (str_color, &gedit_settings->selection_color);
		
		g_free (str_color);
		str_color = NULL;
	}

	/* Selected text color */	
	str_color = gconf_client_get_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_SELECTED_TEXT_COLOR,
			      	NULL);
	
	if (str_color != NULL)
	{
		gdk_color_parse (str_color, &gedit_settings->selected_text_color);
		
		g_free (str_color);
		str_color = NULL;
	}
	
	/* Editor/Save */
	gedit_settings->create_backup_copy = gconf_client_get_bool (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREFS_CREATE_BACKUP_COPY,
			      	NULL);

	if (gedit_settings->backup_extension != NULL)
		g_free (gedit_settings->backup_extension);
	
	gedit_settings->backup_extension = gconf_client_get_string (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREFS_BACKUP_EXTENSION,
			      	NULL);
	
	if (gedit_settings->backup_extension == NULL)
		gedit_settings->backup_extension = g_strdup (".bak");

	gedit_settings->auto_save = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREFS_AUTO_SAVE,
			      	NULL);

	gedit_settings->auto_save_interval = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREFS_AUTO_SAVE_INTERVAL,
			      	NULL);

	gedit_settings->save_encoding = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREFS_SAVE_ENCODING,
			      	NULL);

	/* Editor/Undo */
	gedit_settings->undo_levels = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_UNDO_LEVELS,
			      	NULL);

	/* Editor/Wrap Mode */
	gedit_settings->wrap_mode = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_WRAP_MODE,
				NULL);

	/* Editor/Tab */
	gedit_settings->tab_size = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TAB_SIZE,
			      	NULL);

	/* User Inferface/Toolbar */
	gedit_settings->toolbar_visible = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_VISIBLE,
				NULL);

	gedit_settings->toolbar_buttons_style = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_BUTTONS_STYLE,
			      	NULL);

	gedit_settings->toolbar_view_tooltips = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_VIEW_TOOLTIPS,
				NULL);

	/* User Inferface/Statusbar */
	gedit_settings->statusbar_visible = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VISIBLE,
				NULL);

	/* User Inferface/MDI */
	gedit_settings->mdi_mode = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_MDI_MODE,
				NULL);

	gedit_settings->mdi_tabs_position = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TABS_POSITION,
				NULL);

	/* FIXME */
	gedit_settings->window_width = 600;
	gedit_settings->window_height = 400;

	gedit_settings->print_header = TRUE;
	gedit_settings->print_footer = FALSE;

	if (gedit_settings->print_font_body != NULL)
		g_free (gedit_settings->print_font_body);
	
	gedit_settings->print_font_body = g_strdup (DEFAULT_PRINT_FONT_BODY);
	
	if (gedit_settings->print_font_header_and_footer != NULL)
		g_free (gedit_settings->print_font_header_and_footer);
	
	gedit_settings->print_font_header_and_footer =  g_strdup (DEFAULT_PRINT_FONT_H_AND_F);	
	
	if (gedit_settings->print_font_numbers != NULL)
		g_free (gedit_settings->print_font_numbers);
	
	gedit_settings->print_font_numbers = g_strdup (DEFAULT_PRINT_FONT_NUMBERS);

	gedit_debug (DEBUG_PREFS, "END");
}

static void gedit_prefs_notify_cb (GConfClient *client,
				   guint cnxn_id,
				   GConfEntry *entry,
				   gpointer user_data)
{
	gedit_debug (DEBUG_PREFS, "Key was changed: %s", entry->key);
}

void gedit_prefs_init ()
{
	gedit_debug (DEBUG_PREFS, "");

	gedit_gconf_client = gconf_client_get_default ();
	
	g_return_if_fail (gedit_gconf_client != NULL);

	gconf_client_add_dir (gedit_gconf_client,
			      GEDIT_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);
	
	gconf_client_notify_add (gedit_gconf_client,
				 GEDIT_BASE_KEY,
				 gedit_prefs_notify_cb,
				 NULL, NULL, NULL);
}
