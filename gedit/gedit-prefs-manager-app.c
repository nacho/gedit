/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005  Paolo Maggi 
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
 * Modified by the gedit Team, 2002-2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libgnome/gnome-config.h>

#include "gedit-prefs-manager.h"
#include "gedit-prefs-manager-private.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-app.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-window.h"
#include "gedit-window-private.h"
#include "gedit-source-style-manager.h"

static void gedit_prefs_manager_editor_font_changed	(GConfClient *client,
							 guint        cnxn_id,
							 GConfEntry  *entry,
							 gpointer     user_data);

static void gedit_prefs_manager_system_font_changed	(GConfClient *client,
							 guint        cnxn_id,
							 GConfEntry  *entry,
							 gpointer     user_data);

static void gedit_prefs_manager_editor_colors_changed	(GConfClient *client,
							 guint        cnxn_id,
							 GConfEntry  *entry,
							 gpointer     user_data);

static void gedit_prefs_manager_tabs_size_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_wrap_mode_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_line_numbers_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_auto_indent_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_undo_changed		(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_right_margin_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_hl_current_line_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);
							 
static void gedit_prefs_manager_bracket_matching_changed(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_syntax_hl_enable_changed(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_search_hl_enable_changed(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_source_style_scheme_changed
							(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_max_recents_changed	(GConfClient *client,
							 guint        cnxn_id, 
							 GConfEntry  *entry, 
							 gpointer     user_data);

static void gedit_prefs_manager_auto_save_changed	(GConfClient *client,
							 guint        cnxn_id,
							 GConfEntry  *entry,
							 gpointer     user_data);

static void gedit_prefs_manager_lockdown_changed	(GConfClient *client,
							 guint        cnxn_id,
							 GConfEntry  *entry,
							 gpointer     user_data);


static gint window_state = -1;
static gint window_height = -1;
static gint window_width = -1;
static gint side_panel_size = -1;
static gint bottom_panel_size = -1;

static gint side_panel_active_page = 0;
static gint bottom_panel_active_page = 0;

static gint active_file_filter = -1;

gboolean
gedit_prefs_manager_app_init (void)
{
	gedit_debug (DEBUG_PREFS);

	g_return_val_if_fail (gedit_prefs_manager == NULL, FALSE);
	
	gedit_prefs_manager_init ();
	
	if (gedit_prefs_manager != NULL)
	{
		/* TODO: notify, add dirs */
		gconf_client_add_dir (gedit_prefs_manager->gconf_client,
				GPM_PREFS_DIR,
				GCONF_CLIENT_PRELOAD_RECURSIVE,
				NULL);
		
		gconf_client_add_dir (gedit_prefs_manager->gconf_client,
				GPM_LOCKDOWN_DIR,
				GCONF_CLIENT_PRELOAD_RECURSIVE,
				NULL);
		
		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_FONT_DIR,
				gedit_prefs_manager_editor_font_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_SYSTEM_FONT,
				gedit_prefs_manager_system_font_changed,
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

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_AUTO_INDENT_DIR,
				gedit_prefs_manager_auto_indent_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_UNDO_DIR,
				gedit_prefs_manager_undo_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_RIGHT_MARGIN_DIR,
				gedit_prefs_manager_right_margin_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_CURRENT_LINE_DIR,
				gedit_prefs_manager_hl_current_line_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_BRACKET_MATCHING_DIR,
				gedit_prefs_manager_bracket_matching_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_SYNTAX_HL_ENABLE,
				gedit_prefs_manager_syntax_hl_enable_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_SEARCH_HIGHLIGHTING_ENABLE,
				gedit_prefs_manager_search_hl_enable_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_SOURCE_STYLE_DIR,
				gedit_prefs_manager_source_style_scheme_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_MAX_RECENTS,
				gedit_prefs_manager_max_recents_changed,
				NULL, NULL, NULL);

		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_SAVE_DIR,
				gedit_prefs_manager_auto_save_changed,
				NULL, NULL, NULL);
		
		gconf_client_notify_add (gedit_prefs_manager->gconf_client,
				GPM_LOCKDOWN_DIR,
				gedit_prefs_manager_lockdown_changed,
				NULL, NULL, NULL);
	}

	return gedit_prefs_manager != NULL;	
}

/* This function must be called before exiting gedit */
void
gedit_prefs_manager_app_shutdown ()
{
	gedit_debug (DEBUG_PREFS);

	gedit_prefs_manager_shutdown ();

	gnome_config_sync ();
}

/* Window state */
gint
gedit_prefs_manager_get_window_state (void)
{
	if (window_state == -1)
		window_state = gnome_config_get_int (GPM_WINDOW_STATE "=" GPM_DEFAULT_WINDOW_STATE_STR);

	return window_state;
}
			
void
gedit_prefs_manager_set_window_state (gint ws)
{
	g_return_if_fail (ws > -1);
	
	window_state = ws;
	gnome_config_set_int (GPM_WINDOW_STATE, ws);
}

gboolean
gedit_prefs_manager_window_state_can_set (void)
{
	return TRUE;
}

/* Window size */
void
gedit_prefs_manager_get_window_size (gint *width, gint *height)
{
	g_return_if_fail (width != NULL && height != NULL);

	if (window_width == -1)
		window_width = gnome_config_get_int (GPM_WINDOW_WIDTH "=" GPM_DEFAULT_WINDOW_WIDTH_STR);

	if (window_height == -1)
		window_height = gnome_config_get_int (GPM_WINDOW_HEIGHT "=" GPM_DEFAULT_WINDOW_HEIGHT_STR);

	*width = window_width;
	*height = window_height;
}

void
gedit_prefs_manager_get_default_window_size (gint *width, gint *height)
{
	g_return_if_fail (width != NULL && height != NULL);

	*width = GPM_DEFAULT_WINDOW_WIDTH;
	*height = GPM_DEFAULT_WINDOW_HEIGHT;
}

void
gedit_prefs_manager_set_window_size (gint width, gint height)
{
	g_return_if_fail (width > -1 && height > -1);

	window_width = width;
	window_height = height;

	gnome_config_set_int (GPM_WINDOW_WIDTH, width);
	gnome_config_set_int (GPM_WINDOW_HEIGHT, height);
}

gboolean 
gedit_prefs_manager_window_size_can_set (void)
{
	return TRUE;
}

/* Side panel */
gint
gedit_prefs_manager_get_side_panel_size (void)
{
	if (side_panel_size == -1)
		side_panel_size = gnome_config_get_int (GPM_SIDE_PANEL_SIZE "=" GPM_DEFAULT_SIDE_PANEL_SIZE_STR);

	return side_panel_size;
}

gint 
gedit_prefs_manager_get_default_side_panel_size (void)
{
	return GPM_DEFAULT_SIDE_PANEL_SIZE;
}

void 
gedit_prefs_manager_set_side_panel_size (gint ps)
{
	g_return_if_fail (ps > -1);
	
	if (side_panel_size == ps)
		return;
		
	side_panel_size = ps;
	gnome_config_set_int (GPM_SIDE_PANEL_SIZE, ps);
}

gboolean 
gedit_prefs_manager_side_panel_size_can_set (void)
{
	return TRUE;
}

gint
gedit_prefs_manager_get_side_panel_active_page (void)
{
	if (side_panel_active_page == 0)
		side_panel_active_page = gnome_config_get_int (
				GPM_SIDE_PANEL_ACTIVE_PAGE);

	return side_panel_active_page;
}

void
gedit_prefs_manager_set_side_panel_active_page (gint id)
{
	if (side_panel_active_page == id)
		return;

	side_panel_active_page = id;
	gnome_config_set_int (GPM_SIDE_PANEL_ACTIVE_PAGE, id);
}

gboolean 
gedit_prefs_manager_side_panel_active_page_can_set (void)
{
	return TRUE;
}

/* Bottom panel */
gint
gedit_prefs_manager_get_bottom_panel_size (void)
{
	if (bottom_panel_size == -1)
		bottom_panel_size = gnome_config_get_int (GPM_BOTTOM_PANEL_SIZE "=" GPM_DEFAULT_BOTTOM_PANEL_SIZE_STR);

	return bottom_panel_size;
}

gint 
gedit_prefs_manager_get_default_bottom_panel_size (void)
{
	return GPM_DEFAULT_BOTTOM_PANEL_SIZE;
}

void 
gedit_prefs_manager_set_bottom_panel_size (gint ps)
{
	g_return_if_fail (ps > -1);

	if (bottom_panel_size == ps)
		return;
	
	bottom_panel_size = ps;
	gnome_config_set_int (GPM_BOTTOM_PANEL_SIZE, ps);
}

gboolean 
gedit_prefs_manager_bottom_panel_size_can_set (void)
{
	return TRUE;
}

gint
gedit_prefs_manager_get_bottom_panel_active_page (void)
{
	if (bottom_panel_active_page == 0)
		bottom_panel_active_page = gnome_config_get_int (
				GPM_BOTTOM_PANEL_ACTIVE_PAGE);

	return bottom_panel_active_page;
}

void
gedit_prefs_manager_set_bottom_panel_active_page (gint id)
{
	if (bottom_panel_active_page == id)
		return;

	bottom_panel_active_page = id;
	gnome_config_set_int (GPM_BOTTOM_PANEL_ACTIVE_PAGE, id);
}

gboolean 
gedit_prefs_manager_bottom_panel_active_page_can_set (void)
{
	return TRUE;
}

/* File filter */
gint
gedit_prefs_manager_get_active_file_filter (void)
{
	if (active_file_filter == -1)
		active_file_filter = gnome_config_get_int (GPM_ACTIVE_FILE_FILTER "=0");

	return active_file_filter;
}

void
gedit_prefs_manager_set_active_file_filter (gint id)
{
	g_return_if_fail (id >= 0);
	
	if (active_file_filter == id)
		return;

	active_file_filter = id;
	gnome_config_set_int (GPM_ACTIVE_FILE_FILTER, id);
}

gboolean 
gedit_prefs_manager_active_file_filter_can_set (void)
{
	return TRUE;
}
static void 
gedit_prefs_manager_editor_font_changed (GConfClient *client,
					 guint        cnxn_id, 
					 GConfEntry  *entry, 
					 gpointer     user_data)
{
	GList *views;
	GList *l;
	gchar *font = NULL;
	gboolean def = TRUE;
	gint ts;

	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_USE_DEFAULT_FONT) == 0)
	{
		if (entry->value->type == GCONF_VALUE_BOOL)
			def = gconf_value_get_bool (entry->value);
		else
			def = GPM_DEFAULT_USE_DEFAULT_FONT;
		
		if (def)
			font = gedit_prefs_manager_get_system_font ();
		else
			font = gedit_prefs_manager_get_editor_font ();
	}
	else if (strcmp (entry->key, GPM_EDITOR_FONT) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			font = g_strdup (gconf_value_get_string (entry->value));
		else
			font = g_strdup (GPM_DEFAULT_EDITOR_FONT);
				
		def = gedit_prefs_manager_get_use_default_font ();
	}
	else
		return;

	g_return_if_fail (font != NULL);
	
	ts = gedit_prefs_manager_get_tabs_size ();

	views = gedit_app_get_views (gedit_app_get_default ());
	l = views;

	while (l != NULL)
	{
		/* Note: we use def=FALSE to avoid GeditView to query gconf */
		gedit_view_set_font (GEDIT_VIEW (l->data), FALSE,  font);
		gtk_source_view_set_tabs_width (GTK_SOURCE_VIEW (l->data), ts);

		l = l->next;
	}

	g_list_free (views);
	g_free (font);
}

static void 
gedit_prefs_manager_system_font_changed (GConfClient *client,
					 guint        cnxn_id, 
					 GConfEntry  *entry, 
					 gpointer     user_data)
{
	GList *views;
	GList *l;
	gchar *font;
	gint ts;

	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_SYSTEM_FONT) != 0)
		return;

	if (!gedit_prefs_manager_get_use_default_font ())
		return;

	if (entry->value->type == GCONF_VALUE_STRING)
		font = g_strdup (gconf_value_get_string (entry->value));
	else
		font = g_strdup (GPM_DEFAULT_SYSTEM_FONT);

	ts = gedit_prefs_manager_get_tabs_size ();

	views = gedit_app_get_views (gedit_app_get_default ());
	l = views;

	while (l != NULL)
	{
		/* Note: we use def=FALSE to avoid GeditView to query gconf */
		gedit_view_set_font (GEDIT_VIEW (l->data), FALSE, font);

		gtk_source_view_set_tabs_width (GTK_SOURCE_VIEW (l->data), ts);
		l = l->next;
	}

	g_list_free (views);
	g_free (font);
}

static void 
set_colors (gboolean  def, 
	    GdkColor *backgroud, 
	    GdkColor *text,
	    GdkColor *selection, 
	    GdkColor *sel_text)
{
	GList *views;
	GList *l;

	views = gedit_app_get_views (gedit_app_get_default ());
	l = views;

	while (l != NULL)
	{
		gedit_view_set_colors (GEDIT_VIEW (l->data),
				       def,
				       backgroud,
				       text,
				       selection,
				       sel_text);

		l = l->next;
	}

	g_list_free (views);
}

static void 
gedit_prefs_manager_editor_colors_changed (GConfClient *client,
					   guint        cnxn_id, 
					   GConfEntry  *entry, 
					   gpointer     user_data)
{	
	gchar *str_color;
	GdkColor color;

	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_USE_DEFAULT_COLORS) == 0)
	{
		gboolean def = TRUE;
		
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
	
	if (gedit_prefs_manager_get_use_default_colors ())
	{
		set_colors (TRUE, NULL, NULL, NULL, NULL);
		return;
	}
		
	if (strcmp (entry->key, GPM_BACKGROUND_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_BACKGROUND_COLOR);
				
		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (FALSE, &color, NULL, NULL, NULL);
	
		return;
	}

	if (strcmp (entry->key, GPM_TEXT_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_TEXT_COLOR);
				
		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (FALSE, NULL, &color, NULL, NULL);
	
		return;
	}

	if (strcmp (entry->key, GPM_SELECTION_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_SELECTION_COLOR);
				
		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (FALSE, NULL, NULL, &color, NULL);
	
		return;
	}

	if (strcmp (entry->key, GPM_SELECTED_TEXT_COLOR) == 0)
	{
		if (entry->value->type == GCONF_VALUE_STRING)
			str_color = g_strdup (gconf_value_get_string (entry->value));
		else
			str_color = g_strdup (GPM_DEFAULT_SELECTED_TEXT_COLOR);

		gdk_color_parse (str_color, &color);	
		g_free (str_color);

		set_colors (FALSE, NULL, NULL, NULL, &color);
	
		return;
	}
}

static void 
gedit_prefs_manager_tabs_size_changed (GConfClient *client,
				       guint        cnxn_id,
				       GConfEntry  *entry, 
				       gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_TABS_SIZE) == 0)
	{
		gint tabs_size;
		GList *views;
		GList *l;
		
		if (entry->value->type == GCONF_VALUE_INT)
			tabs_size = gconf_value_get_int (entry->value);
		else
			tabs_size = GPM_DEFAULT_TABS_SIZE;
	
		tabs_size = CLAMP (tabs_size, 1, 24);

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_source_view_set_tabs_width (GTK_SOURCE_VIEW (l->data), 
							tabs_size);

			l = l->next;
		}

		g_list_free (views);
	}
	else if (strcmp (entry->key, GPM_INSERT_SPACES) == 0)
	{
		gboolean enable;
		GList *views;
		GList *l;
		
		if (entry->value->type == GCONF_VALUE_BOOL)
			enable = gconf_value_get_bool (entry->value);	
		else
			enable = GPM_DEFAULT_INSERT_SPACES;

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_source_view_set_insert_spaces_instead_of_tabs (
					GTK_SOURCE_VIEW (l->data), 
					enable);

			l = l->next;
		}

		g_list_free (views);
	}
}

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

static void 
gedit_prefs_manager_wrap_mode_changed (GConfClient *client,
	                               guint        cnxn_id, 
	                               GConfEntry  *entry, 
	                               gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_WRAP_MODE) == 0)
	{
		GtkWrapMode wrap_mode;
		GList *views;
		GList *l;

		if (entry->value->type == GCONF_VALUE_STRING)
			wrap_mode = 
				get_wrap_mode_from_string (gconf_value_get_string (entry->value));	
		else
			wrap_mode = get_wrap_mode_from_string (GPM_DEFAULT_WRAP_MODE);

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (l->data),
						     wrap_mode);

			l = l->next;
		}

		g_list_free (views);
	}
}

static void 
gedit_prefs_manager_line_numbers_changed (GConfClient *client,
					  guint        cnxn_id, 
					  GConfEntry  *entry, 
					  gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_DISPLAY_LINE_NUMBERS) == 0)
	{
		gboolean dln;
		GList *views;
		GList *l;

		if (entry->value->type == GCONF_VALUE_BOOL)
			dln = gconf_value_get_bool (entry->value);	
		else
			dln = GPM_DEFAULT_DISPLAY_LINE_NUMBERS;

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (l->data), 
							       dln);

			l = l->next;
		}

		g_list_free (views);
	}
}

static void 
gedit_prefs_manager_hl_current_line_changed (GConfClient *client,
					     guint        cnxn_id, 
					     GConfEntry  *entry, 
					     gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_HIGHLIGHT_CURRENT_LINE) == 0)
	{
		gboolean hl;
		GList *views;
		GList *l;

		if (entry->value->type == GCONF_VALUE_BOOL)
			hl = gconf_value_get_bool (entry->value);	
		else
			hl = GPM_DEFAULT_HIGHLIGHT_CURRENT_LINE;

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (l->data), 
								    hl);

			l = l->next;
		}

		g_list_free (views);
	}
}

static void 
gedit_prefs_manager_bracket_matching_changed (GConfClient *client,
					      guint        cnxn_id, 
					      GConfEntry  *entry, 
					      gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_BRACKET_MATCHING) == 0)
	{
		gboolean enable;
		GList *docs;
		GList *l;

		if (entry->value->type == GCONF_VALUE_BOOL)
			enable = gconf_value_get_bool (entry->value);
		else
			enable = GPM_DEFAULT_BRACKET_MATCHING;

		docs = gedit_app_get_documents (gedit_app_get_default ());
		l = docs;

		while (l != NULL)
		{
			gtk_source_buffer_set_check_brackets (GTK_SOURCE_BUFFER (l->data),
							      enable);

			l = l->next;
		}

		g_list_free (docs);
	}
}

static void 
gedit_prefs_manager_auto_indent_changed (GConfClient *client,
					 guint        cnxn_id, 
					 GConfEntry  *entry, 
					 gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_AUTO_INDENT) == 0)
	{
		gboolean enable;
		GList *views;
		GList *l;

		if (entry->value->type == GCONF_VALUE_BOOL)
			enable = gconf_value_get_bool (entry->value);	
		else
			enable = GPM_DEFAULT_AUTO_INDENT;
	
		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{		
			gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (l->data), 
							 enable);
			
			l = l->next;
		}

		g_list_free (views);
	}
}

static void 
gedit_prefs_manager_undo_changed (GConfClient *client,
				  guint        cnxn_id, 
				  GConfEntry  *entry, 
				  gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_UNDO_ACTIONS_LIMIT) == 0)
	{
		gint ul;
		GList *docs;
		GList *l;

		if (entry->value->type == GCONF_VALUE_INT)
			ul = gconf_value_get_int (entry->value);
		else
			ul = GPM_DEFAULT_UNDO_ACTIONS_LIMIT;
	
		ul = CLAMP (ul, -1, 250);

		docs = gedit_app_get_documents (gedit_app_get_default ());
		l = docs;
		
		while (l != NULL)
		{
			gtk_source_buffer_set_max_undo_levels (GTK_SOURCE_BUFFER (l->data), 
							       ul);

			l = l->next;
		}

		g_list_free (docs);
	}
}

static void 
gedit_prefs_manager_right_margin_changed (GConfClient *client,
					  guint cnxn_id,
					  GConfEntry *entry,
					  gpointer user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_RIGHT_MARGIN_POSITION) == 0)
	{
		gint pos;
		GList *views;
		GList *l;

		if (entry->value->type == GCONF_VALUE_INT)
			pos = gconf_value_get_int (entry->value);
		else
			pos = GPM_DEFAULT_RIGHT_MARGIN_POSITION;

		pos = CLAMP (pos, 1, 160);

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_source_view_set_margin (GTK_SOURCE_VIEW (l->data),
						    pos);

			l = l->next;
		}

		g_list_free (views);
	}
	else if (strcmp (entry->key, GPM_DISPLAY_RIGHT_MARGIN) == 0)
	{
		gboolean display;
		GList *views;
		GList *l;

		if (entry->value->type == GCONF_VALUE_BOOL)
			display = gconf_value_get_bool (entry->value);	
		else
			display = GPM_DEFAULT_DISPLAY_RIGHT_MARGIN;

		views = gedit_app_get_views (gedit_app_get_default ());
		l = views;

		while (l != NULL)
		{
			gtk_source_view_set_show_margin (GTK_SOURCE_VIEW (l->data),
							 display);

			l = l->next;
		}

		g_list_free (views);
	}
}

static void
gedit_prefs_manager_syntax_hl_enable_changed (GConfClient *client,
					      guint        cnxn_id,
					      GConfEntry  *entry,
					      gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_SYNTAX_HL_ENABLE) == 0)
	{
		gboolean enable;
		GList *docs;
		GList *l;
		const GList *windows;

		if (entry->value->type == GCONF_VALUE_BOOL)
			enable = gconf_value_get_bool (entry->value);
		else
			enable = GPM_DEFAULT_SYNTAX_HL_ENABLE;

		docs = gedit_app_get_documents (gedit_app_get_default ());
		l = docs;

		while (l != NULL)
		{
			g_return_if_fail (GTK_IS_SOURCE_BUFFER (l->data));

			gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (l->data),
							 enable);

			l = l->next;
		}

		g_list_free (docs);

		/* update the sensitivity of the Higlight Mode menu item */
		windows = gedit_app_get_windows (gedit_app_get_default ());
		while (windows != NULL)
		{
			GtkUIManager *ui;
			GtkAction *a;

			ui = gedit_window_get_ui_manager (GEDIT_WINDOW (windows->data));

			a = gtk_ui_manager_get_action (ui,
						       "/MenuBar/ViewMenu/ViewHighlightModeMenu");

			gtk_action_set_sensitive (a, enable);

			windows = g_list_next (windows);
		}
	}
}

static void
gedit_prefs_manager_search_hl_enable_changed (GConfClient *client,
					      guint        cnxn_id,
					      GConfEntry  *entry,
					      gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_SEARCH_HIGHLIGHTING_ENABLE) == 0)
	{
		gboolean enable;
		GList *docs;
		GList *l;

		if (entry->value->type == GCONF_VALUE_BOOL)
			enable = gconf_value_get_bool (entry->value);
		else
			enable = GPM_DEFAULT_SEARCH_HIGHLIGHTING_ENABLE;

		docs = gedit_app_get_documents (gedit_app_get_default ());
		l = docs;

		while (l != NULL)
		{
			g_return_if_fail (GEDIT_IS_DOCUMENT (l->data));

			gedit_document_set_enable_search_highlighting  (GEDIT_DOCUMENT (l->data),
									enable);

			l = l->next;
		}

		g_list_free (docs);
	}
}

static void
gedit_prefs_manager_source_style_scheme_changed (GConfClient *client,
						 guint        cnxn_id,
						 GConfEntry  *entry,
						 gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_SOURCE_STYLE_SCHEME) == 0)
	{
		const gchar *scheme;
		gboolean changed;
		GtkSourceStyleScheme *style;
		GList *docs;
		GList *l;

		if (entry->value->type == GCONF_VALUE_STRING)
			scheme = gconf_value_get_string (entry->value);
		else
			scheme = GPM_DEFAULT_SOURCE_STYLE_SCHEME;

		changed = _gedit_source_style_manager_set_default_scheme
					(gedit_get_source_style_manager (),
					 scheme);
		if (!changed)
			return;

		style = gedit_source_style_manager_get_default_scheme
					(gedit_get_source_style_manager ());

		docs = gedit_app_get_documents (gedit_app_get_default ());
		for (l = docs; l != NULL; l = l->next)
		{
			g_return_if_fail (GTK_IS_SOURCE_BUFFER (l->data));

			gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (l->data),
							    style);
		}

		g_list_free (docs);
	}
}

static void
gedit_prefs_manager_max_recents_changed (GConfClient *client,
					 guint        cnxn_id,
					 GConfEntry  *entry,
					 gpointer     user_data)
{
	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_MAX_RECENTS) == 0)
	{
		const GList *windows;
		gint max;

		if (entry->value->type == GCONF_VALUE_INT)
		{
			max = gconf_value_get_int (entry->value);

			if (max < 0)
				max = GPM_DEFAULT_MAX_RECENTS;
		}
		else
			max = GPM_DEFAULT_MAX_RECENTS;

		windows = gedit_app_get_windows (gedit_app_get_default ());
		while (windows != NULL)
		{
			GeditWindow *w = windows->data;

			gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (w->priv->toolbar_recent_menu),
						      max);

			windows = g_list_next (windows);
		}

		/* FIXME: we have no way at the moment to trigger the
		 * update of the inline recents in the File menu */
	}
}

static void
gedit_prefs_manager_auto_save_changed (GConfClient *client,
				       guint        cnxn_id,
				       GConfEntry  *entry,
				       gpointer     user_data)
{
	GList *docs;
	GList *l;

	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (strcmp (entry->key, GPM_AUTO_SAVE) == 0)
	{
		gboolean auto_save;

		if (entry->value->type == GCONF_VALUE_BOOL)
			auto_save = gconf_value_get_bool (entry->value);
		else
			auto_save = GPM_DEFAULT_AUTO_SAVE;

		docs = gedit_app_get_documents (gedit_app_get_default ());
		l = docs;

		while (l != NULL)
		{
			GeditDocument *doc = GEDIT_DOCUMENT (l->data);
			GeditTab *tab = gedit_tab_get_from_document (doc);

			gedit_tab_set_auto_save_enabled (tab, auto_save);

			l = l->next;
		}

		g_list_free (docs);
	}
	else if (strcmp (entry->key,  GPM_AUTO_SAVE_INTERVAL) == 0)
	{
		gint auto_save_interval;

		if (entry->value->type == GCONF_VALUE_INT)
		{
			auto_save_interval = gconf_value_get_int (entry->value);

			if (auto_save_interval <= 0)
				auto_save_interval = GPM_DEFAULT_AUTO_SAVE_INTERVAL;
		}
		else
			auto_save_interval = GPM_DEFAULT_AUTO_SAVE_INTERVAL;

		docs = gedit_app_get_documents (gedit_app_get_default ());
		l = docs;

		while (l != NULL)
		{
			GeditDocument *doc = GEDIT_DOCUMENT (l->data);
			GeditTab *tab = gedit_tab_get_from_document (doc);

			gedit_tab_set_auto_save_interval (tab, auto_save_interval);

			l = l->next;
		}

		g_list_free (docs);
	}
}

static void
gedit_prefs_manager_lockdown_changed (GConfClient *client,
				      guint        cnxn_id,
				      GConfEntry  *entry,
				      gpointer     user_data)
{
	GeditApp *app;
	gboolean locked;

	gedit_debug (DEBUG_PREFS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (entry->value->type == GCONF_VALUE_BOOL)
		locked = gconf_value_get_bool (entry->value);
	else
		locked = FALSE;

	app = gedit_app_get_default ();

	if (strcmp (entry->key, GPM_LOCKDOWN_COMMAND_LINE) == 0)
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_COMMAND_LINE,
					     locked);

	else if (strcmp (entry->key, GPM_LOCKDOWN_PRINTING) == 0)
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_PRINTING,
					     locked);

	else if (strcmp (entry->key, GPM_LOCKDOWN_PRINT_SETUP) == 0)
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_PRINT_SETUP,
					     locked);

	else if (strcmp (entry->key, GPM_LOCKDOWN_SAVE_TO_DISK) == 0)
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_SAVE_TO_DISK,
					     locked);
}
