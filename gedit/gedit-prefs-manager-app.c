/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2003  Paolo Maggi 
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libgnome/gnome-config.h>

#include "gedit-prefs-manager.h"
#include "gedit-prefs-manager-private.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-mdi.h"
#include "gedit2.h"

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

static gint window_state = -1;
static gint window_height = -1;
static gint window_width = -1;


gboolean
gedit_prefs_manager_app_init (void)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager == NULL, FALSE);
	
	gedit_prefs_manager_init ();
	
	if (gedit_prefs_manager != NULL)
	{
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

	return gedit_prefs_manager != NULL;	
}

/* This function must be called before exiting gedit */
void
gedit_prefs_manager_app_shutdown ()
{
	gedit_debug (DEBUG_PREFS, "");

	gedit_prefs_manager_shutdown ();

	gnome_config_sync ();
}



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

