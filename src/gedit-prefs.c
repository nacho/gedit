/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
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


#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include <bonobo-mdi.h>
#include <gconf/gconf-client.h>
#include "gedit-prefs.h"
#include "gedit-debug.h"
#include "gedit2.h"


#define GEDIT_BASE_KEY "/apps/gedit-2"

#define GEDIT_PREF_USE_DEFAULT_FONT	"/use-default-font"
#define GEDIT_PREF_EDITOR_FONT		"/editor-font"

#define GEDIT_PREF_USE_DEFAULT_COLORS	"/use-default-colors"
#define GEDIT_PREF_BACKGROUND_COLOR	"/background-color"
#define GEDIT_PREF_TEXT_COLOR		"/text-color"
#define GEDIT_PREF_SELECTED_TEXT_COLOR	"/selected-text-color"
#define GEDIT_PREF_SELECTION_COLOR	"/selection-color"

#define GEDIT_PREF_CREATE_BACKUP_COPY   "/create-backup-copy"
#define GEDIT_PREF_BACKUP_EXTENSION  	"/backup-copy-extension"

#define GEDIT_PREF_AUTO_SAVE		"/auto-save"
#define GEDIT_PREF_AUTO_SAVE_INTERVAL	"/auto-save-interval"

#define GEDIT_PREF_SAVE_ENCODING	"/save-encoding"

#define GEDIT_PREF_UNDO_LEVELS		"/undo-levels"

#define GEDIT_PREF_WRAP_MODE		"/wrap-mode"

#define GEDIT_PREF_TAB_SIZE		"/tab-size"

#define GEDIT_PREF_SHOW_LINE_NUMBERS	"/show-line-numbers"

#define GEDIT_PREF_TOOLBAR_VISIBLE	 "/toolbar-visible"
#define GEDIT_PREF_TOOLBAR_BUTTONS_STYLE "/toolbar-buttons-style"

#define GEDIT_PREF_STATUSBAR_VISIBLE	"/statusbar-visible"
#define GEDIT_PREF_STATUSBAR_VIEW_CURSOR_POSITION "/statusbar-view-cursor-position"
#define GEDIT_PREF_STATUSBAR_VIEW_OVERWRITE_MODE  "/statusbar-view-overwrite-mode"

#define GEDIT_PREF_MDI_MODE		"/mdi-mode"
#define GEDIT_PREF_TABS_POSITION	"/mdi-tabs-position"

#define GEDIT_PREF_WINDOW_WIDTH		"/window-width"
#define GEDIT_PREF_WINDOW_HEIGHT	"/window-height"

#define GEDIT_PREF_PRINT_HEADER		"/print-header"
#define GEDIT_PREF_PRINT_WRAP_LINES	"/print-wrap-lines"
#define GEDIT_PREF_PRINT_LINE_NUMBERS	"/print-line-numbers"

#define GEDIT_PREF_PRINT_FONT_BODY	"/print-font-body"
#define GEDIT_PREF_PRINT_FONT_HEADER	"/print-font-header"
#define GEDIT_PREF_PRINT_FONT_NUMBERS	"/print-font-numbers"

#define GEDIT_PREF_MAX_RECENTS		"/max-recents"


GeditPreferences 	*gedit_settings 	= NULL;
static GConfClient 	*gedit_gconf_client 	= NULL;

#define DEFAULT_EDITOR_FONT 		(const gchar*) "Courier Medium 12"

#define DEFAULT_PRINT_FONT_BODY 	(const gchar*) "Courier 9"
#define DEFAULT_PRINT_FONT_HEADER	(const gchar*) "Helvetica 11"
#define DEFAULT_PRINT_FONT_NUMBERS	(const gchar*) "Helvetica 8"


static gchar	*gedit_prefs_gdk_color_to_string 	(GdkColor color);
static void	 gedit_prefs_save_color 		(GdkColor color, const gchar *key);
static GdkColor  gedit_prefs_load_color 		(const gchar *key);


static gchar* 
gedit_prefs_gdk_color_to_string (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");

	return g_strdup_printf ("#%04x%04x%04x",
				color.red, 
				color.green,
				color.blue);
}

/* Converts GdkColor to string format and save. */
static void
gedit_prefs_save_color (GdkColor color, const gchar *key)
{
	gchar *str_color = NULL;

	str_color = gedit_prefs_gdk_color_to_string (color);
	g_return_if_fail (str_color != NULL);

	gconf_client_set_string (gedit_gconf_client,
				 key,
				 str_color,
				 NULL);

	g_free (str_color);
}

/* Parses color from string format and loads GdkColor. */
static GdkColor
gedit_prefs_load_color (const gchar *key)
{
	gchar *str_color = NULL;
	GdkColor color;
	
	str_color = gconf_client_get_string (gedit_gconf_client, key, NULL);

	if (str_color != NULL)
	{
		gdk_color_parse (str_color, &color);
		g_free (str_color);
	}

	return color;
}

void 
gedit_prefs_save_settings (void)
{
	BonoboWindow* active_window = NULL;		

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
	gedit_prefs_save_color (gedit_settings->background_color,
				GEDIT_BASE_KEY GEDIT_PREF_BACKGROUND_COLOR);
	
	/* Text color */
	gedit_prefs_save_color (gedit_settings->text_color,
			      	GEDIT_BASE_KEY GEDIT_PREF_TEXT_COLOR);

	/* Selection color */
	gedit_prefs_save_color (gedit_settings->selection_color,
				GEDIT_BASE_KEY GEDIT_PREF_SELECTION_COLOR);

	/* Selected text color */
	gedit_prefs_save_color (gedit_settings->selected_text_color,
				GEDIT_BASE_KEY GEDIT_PREF_SELECTED_TEXT_COLOR);

	gconf_client_set_bool (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_CREATE_BACKUP_COPY,
			      	gedit_settings->create_backup_copy,
			      	NULL);

	g_return_if_fail (gedit_settings->backup_extension != NULL);
	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_BACKUP_EXTENSION,
			      	gedit_settings->backup_extension,
			      	NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_AUTO_SAVE,
			      	gedit_settings->auto_save,
			      	NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_AUTO_SAVE_INTERVAL,
			      	gedit_settings->auto_save_interval,
			      	NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_SAVE_ENCODING,
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
				GEDIT_BASE_KEY GEDIT_PREF_SHOW_LINE_NUMBERS,
			      	gedit_settings->show_line_numbers,
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
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VISIBLE,
				gedit_settings->statusbar_visible,
				NULL);
	
	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VIEW_CURSOR_POSITION,
				gedit_settings->statusbar_view_cursor_position,
				NULL);
	
	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VIEW_OVERWRITE_MODE,
				gedit_settings->statusbar_view_overwrite_mode,
				NULL);

	
	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_MDI_MODE,
				gedit_settings->mdi_mode,
				NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_TABS_POSITION,
				gedit_settings->mdi_tabs_position,
				NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_PRINT_HEADER,
			      	gedit_settings->print_header,
			      	NULL);

	gconf_client_set_bool (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_PRINT_WRAP_LINES,
			      	gedit_settings->print_wrap_lines,
			      	NULL);

	gconf_client_set_int (gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_PRINT_LINE_NUMBERS,
			      	gedit_settings->print_line_numbers,
			      	NULL);

	g_return_if_fail (gedit_settings->print_font_body != NULL);
	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_PRINT_FONT_BODY,
			      	gedit_settings->print_font_body,
			      	NULL);

	g_return_if_fail (gedit_settings->print_font_header != NULL);
	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_PRINT_FONT_HEADER,
			      	gedit_settings->print_font_header,
			      	NULL);

	g_return_if_fail (gedit_settings->print_font_numbers != NULL);
	gconf_client_set_string (gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_PRINT_FONT_NUMBERS,
			      	gedit_settings->print_font_numbers,
			      	NULL);


	/* FIXME: we should not save the size of a maximized or iconified window */
	active_window = bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi));
	if (active_window != NULL) 
	{
		gtk_window_get_size (GTK_WINDOW (active_window),
				     &gedit_settings->window_width, 
				     &gedit_settings->window_height);
	}
	
	gconf_client_set_int (gedit_gconf_client,
		      GEDIT_BASE_KEY GEDIT_PREF_WINDOW_WIDTH,
		      gedit_settings->window_width,
		      NULL);

	gconf_client_set_int (gedit_gconf_client,
		      GEDIT_BASE_KEY GEDIT_PREF_WINDOW_HEIGHT,
		      gedit_settings->window_height,
		      NULL);	
		
	gconf_client_suggest_sync (gedit_gconf_client, NULL);
	
#if 0 /* FIXME */
	gedit_plugin_save_settings ();
#endif	
	gedit_debug (DEBUG_PREFS, "END");
}

void
gedit_prefs_load_settings (void)
{
	gedit_debug (DEBUG_PREFS, "START");
	
	if (gedit_settings == NULL)
		gedit_settings = g_new0 (GeditPreferences, 1);

	if (gedit_gconf_client == NULL)
	{
		/* TODO: in any case set default values */
		g_warning ("Cannot load settings.");
		return;
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
	gedit_settings->background_color = gedit_prefs_load_color (
				GEDIT_BASE_KEY GEDIT_PREF_BACKGROUND_COLOR);
	
	/* Text color */	
	gedit_settings->text_color = gedit_prefs_load_color (
				GEDIT_BASE_KEY GEDIT_PREF_TEXT_COLOR);

	/* Selection color */	
	gedit_settings->selection_color = gedit_prefs_load_color (
				GEDIT_BASE_KEY GEDIT_PREF_SELECTION_COLOR);

	/* Selected text color */	
	gedit_settings->selected_text_color = gedit_prefs_load_color (
				GEDIT_BASE_KEY GEDIT_PREF_SELECTED_TEXT_COLOR);
	
	/* Editor/Save */
	gedit_settings->create_backup_copy = gconf_client_get_bool (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_CREATE_BACKUP_COPY,
			      	NULL);

	if (gedit_settings->backup_extension != NULL)
		g_free (gedit_settings->backup_extension);
	
	gedit_settings->backup_extension = gconf_client_get_string (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_BACKUP_EXTENSION,
			      	NULL);
	
	if (gedit_settings->backup_extension == NULL)
		gedit_settings->backup_extension = g_strdup (".bak");

	gedit_settings->auto_save = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_AUTO_SAVE,
			      	NULL);

	gedit_settings->auto_save_interval = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_AUTO_SAVE_INTERVAL,
			      	NULL);

	gedit_settings->save_encoding = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_SAVE_ENCODING,
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

	/* Editor/Line numbers */
	gedit_settings->show_line_numbers = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_SHOW_LINE_NUMBERS,
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

	/* User Inferface/Statusbar */
	gedit_settings->statusbar_visible = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VISIBLE,
				NULL);

	gedit_settings->statusbar_view_cursor_position = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VIEW_CURSOR_POSITION,
				NULL);

	gedit_settings->statusbar_view_overwrite_mode = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_STATUSBAR_VIEW_OVERWRITE_MODE,
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

	gedit_settings->window_width =  gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_WINDOW_WIDTH,
				NULL);
	
	gedit_settings->window_height =  gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_WINDOW_HEIGHT,
				NULL);

	/* Print */
	gedit_settings->print_header = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_PRINT_HEADER,
				NULL);

	gedit_settings->print_wrap_lines = gconf_client_get_bool (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_PRINT_WRAP_LINES,
				NULL);

	gedit_settings->print_line_numbers = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_PRINT_LINE_NUMBERS,
				NULL);

	/* Print -> font body */
	if (gedit_settings->print_font_body != NULL)
		g_free (gedit_settings->print_font_body);
	
	gedit_settings->print_font_body = gconf_client_get_string (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_PRINT_FONT_BODY,
			      	NULL);

	if (gedit_settings->print_font_body == NULL)
		gedit_settings->print_font_body = g_strdup (DEFAULT_PRINT_FONT_BODY);
	
	/* Print -> font header */
	if (gedit_settings->print_font_header != NULL)
		g_free (gedit_settings->print_font_header);
	
	gedit_settings->print_font_header = gconf_client_get_string (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_PRINT_FONT_HEADER,
			      	NULL);
	
	if (gedit_settings->print_font_header == NULL)
		gedit_settings->print_font_header = g_strdup (DEFAULT_PRINT_FONT_HEADER);	
	
	/* Print -> font numbers */
	if (gedit_settings->print_font_numbers != NULL)
		g_free (gedit_settings->print_font_numbers);
	
	gedit_settings->print_font_numbers = gconf_client_get_string (
				gedit_gconf_client,
			      	GEDIT_BASE_KEY GEDIT_PREF_PRINT_FONT_NUMBERS,
			      	NULL);
	
	if (gedit_settings->print_font_numbers == NULL)
		gedit_settings->print_font_numbers = g_strdup (DEFAULT_PRINT_FONT_NUMBERS);

	
	gedit_settings->max_recents = gconf_client_get_int (
				gedit_gconf_client,
				GEDIT_BASE_KEY GEDIT_PREF_MAX_RECENTS,
				NULL);

	if (gedit_settings->max_recents <= 0)
		gedit_settings->max_recents = 4;

	gedit_debug (DEBUG_PREFS, "END");
}

static void 
gedit_prefs_notify_cb (GConfClient *client,
				   guint cnxn_id,
				   GConfEntry *entry,
				   gpointer user_data)
{
	gedit_debug (DEBUG_PREFS, "Key was changed: %s", entry->key);
}

void 
gedit_prefs_init (void)
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

/* Track windows size */
gboolean gedit_prefs_configure_event_handler (GtkWidget	     *widget,
					      GdkEventConfigure   *event)
{
	gedit_debug (DEBUG_PREFS, "");
	
	g_return_val_if_fail (gedit_settings != NULL, FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	/* FIXME: don't save size if windowe is maximixed or iconified */
	if ((event->width < 300) || (event->height < 200))
		return FALSE;

	gedit_settings->window_width = event->width;
	gedit_settings->window_height = event->height;

	return FALSE;
}

