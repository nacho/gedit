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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */


/* TODO: 
 * [ ] It should be rewritten to use GConf 
 * [ ] check if all the preferences are really used in the code 
 * [ ] stop saving everything in /gedit2/Global/, split config file
 *     into sections
 */

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include <bonobo-mdi.h>
#include <gconf/gconf-client.h>
#include "gedit-prefs.h"
#include "gedit-debug.h"
#include "gedit2.h"



GeditPreferences *settings = NULL;

#define GEDIT_BASE_KEY "/apps/gedit2"

#define GEDIT_PREF_TAB_POS		"/tab-position"
#define GEDIT_PREF_MDI_MODE		"/mdi-mode"
#define GEDIT_PREF_AUTO_INDENT		"/auto-indent"
#define GEDIT_PREF_WRAP_MODE		"/wrap-mode"
#define GEDIT_PREF_HAS_RUN		"/has-run"
#define GEDIT_PREF_SHOW_STATUSBAR	"/show-statusbar"
#define GEDIT_PREF_TOOLBAR_LABELS	"/toolbar-labels"
#define GEDIT_PREF_TOOLBAR		"/toolbar"
#define GEDIT_PREF_SHOW_TOOLTIPS	"/show-tooltips"
#define GEDIT_PREF_SPLITSCREEN		"/splitscreen"
#define GEDIT_PREF_UNDO_LEVELS		"/undo-levels"
#define GEDIT_PREF_TAB_SIZE		"/tab-size"
#define GEDIT_PREF_USE_DEFAULT_FONT	"/use-default-font"
#define GEDIT_PREF_USE_DEFAULT_COLORS	"/use-default-colors"
#define GEDIT_PREF_BGR			"/background-red"
#define GEDIT_PREF_BGG			"/background-green"
#define GEDIT_PREF_BGB			"/background-blue"
#define GEDIT_PREF_FGR			"/foreground-red"
#define GEDIT_PREF_FGG			"/foreground-green"
#define GEDIT_PREF_FGB			"/foreground-blue"
#define GEDIT_PREF_WIDTH		"/window-width"
#define GEDIT_PREF_HEIGHT		"/window-height"
#define GEDIT_PREF_PRINTWRAP		"/printwrap"
#define GEDIT_PREF_PRINTHEADER		"/printheader"
#define GEDIT_PREF_PRINTLINES		"/printlines"
#define GEDIT_PREF_PRINTORIENTATION	"/print-orientation"
#define GEDIT_PREF_PAPERSIZE		"/papersize"
#define GEDIT_PREF_FONT			"/font"
#define GEDIT_PREF_STR			"/selected-text-red"
#define GEDIT_PREF_STG			"/selected-text-green"
#define GEDIT_PREF_STB			"/selected-text-blue"
#define GEDIT_PREF_SELR			"/selection-red"
#define GEDIT_PREF_SELG			"/selection-green"
#define GEDIT_PREF_SELB			"/selection-blue"


static GConfClient *gconf_client = NULL;
#define DEFAULT_FONT (const gchar*) "Courier Medium 12"

void 
gedit_prefs_save_settings (void)
{
	BonoboWindow* active_window;
	GdkWindow *toplevel;
	gint root_x;
	gint root_y;

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_TAB_POS,
			      settings->tab_pos,
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_MDI_MODE,
			      settings->mdi_mode,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_AUTO_INDENT,
			      settings->auto_indent,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_WRAP_MODE,
			      settings->wrap_mode,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_SHOW_STATUSBAR,
			      settings->show_status,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_LABELS,
			      settings->toolbar_labels,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR,
			      settings->have_toolbar,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_SHOW_TOOLTIPS,
			      settings->show_tooltips,
			      NULL);

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_SPLITSCREEN,
			      settings->splitscreen,
			      NULL);

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_UNDO_LEVELS,
			      settings->undo_levels,
			      NULL);

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_TAB_SIZE,
			      settings->tab_size,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_FONT,
			      settings->use_default_font,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_COLORS,
			      settings->use_default_colors,
			      NULL);

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_BGR,
			      settings->bg[0],
			      NULL);

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_BGG,
			      settings->bg[1],
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_BGB,
			      settings->bg[2],
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_FGR,
			      settings->fg[0],
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_FGG,
			      settings->fg[1],
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_FGB,
			      settings->fg[2],
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_STR,
			      settings->st[0],
			      NULL);
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_STG,
			      settings->st[1],
			      NULL);
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_STB,
			      settings->st[2],
			      NULL);
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_SELR,
			      settings->sel[0],
			      NULL);
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_SELG,
			      settings->sel[1],
			      NULL);
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_SELB,
			      settings->sel[2],
			      NULL);

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
		gconf_client_set_int (gconf_client,
				      GEDIT_BASE_KEY GEDIT_PREF_WIDTH,
				      settings->width,
				      NULL);

		gconf_client_set_int (gconf_client,
				      GEDIT_BASE_KEY GEDIT_PREF_HEIGHT,
				      settings->height,
				      NULL);
	}
	
	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_PRINTWRAP,
			      settings->print_wrap_lines,
			      NULL);

	gconf_client_set_bool (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_PRINTHEADER,
			      settings->print_header,
			      NULL);

	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_PRINTLINES,
			      settings->print_lines,
			      NULL);
	
	gconf_client_set_string (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_PAPERSIZE,
			      settings->papersize,
			      NULL);
	
	gconf_client_set_int (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_PRINTORIENTATION,
			      settings->print_orientation,
			      NULL);
	
	gconf_client_set_string (gconf_client,
			      GEDIT_BASE_KEY GEDIT_PREF_FONT,
			      settings->font,
			      NULL);

	if (!settings->run) {
		settings->run = TRUE;
	
		gconf_client_set_int (gconf_client,
				      GEDIT_BASE_KEY GEDIT_PREF_HAS_RUN,
				      settings->run,
				      NULL);
	}

	gconf_client_suggest_sync (gconf_client, NULL);
	
#if 0 /* FIXME */
	gedit_plugin_save_settings ();
#endif	
	gedit_debug (DEBUG_PREFS, "end");
}

void
gedit_prefs_load_settings (void)
{
	gedit_debug (DEBUG_PREFS, "");

	if (!settings)
	{
		settings = g_new0 (GeditPreferences, 1);
	}

	settings->tab_pos = gconf_client_get_int (gconf_client,
						  GEDIT_BASE_KEY GEDIT_PREF_TAB_POS,
						  NULL);

	settings->mdi_mode = gconf_client_get_int (gconf_client,
						  GEDIT_BASE_KEY GEDIT_PREF_MDI_MODE,
						  NULL);

	settings->auto_indent = gconf_client_get_bool (gconf_client,
						  GEDIT_BASE_KEY GEDIT_PREF_AUTO_INDENT,
						  NULL);
	
	settings->wrap_mode = gconf_client_get_bool (gconf_client,
						  GEDIT_BASE_KEY GEDIT_PREF_WRAP_MODE,
						  NULL);

	settings->run = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_HAS_RUN,
					      NULL);
	
	settings->show_status = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_SHOW_STATUSBAR,
					      NULL);

	settings->toolbar_labels = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR_LABELS,
					      NULL);

	settings->have_toolbar = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_TOOLBAR,
					      NULL);

	settings->show_tooltips = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_SHOW_TOOLTIPS,
					      NULL);

	settings->splitscreen = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_SPLITSCREEN,
					      NULL);

	settings->undo_levels = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_UNDO_LEVELS,
					      NULL);

	settings->tab_size = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_TAB_SIZE,
					      NULL);

	settings->use_default_font = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_FONT,
					      NULL);

	settings->use_default_colors = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_USE_DEFAULT_COLORS,
					      NULL);

	settings->bg[0] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_BGR,
					      NULL);
	settings->bg[1] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_BGG,
					      NULL);
	settings->bg[2] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_BGB,
					      NULL);
	
	settings->fg[0] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_FGR,
					      NULL);
	settings->fg[1] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_FGG,
					      NULL);
	settings->fg[2] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_FGB,
					      NULL);
	
	settings->width = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_WIDTH,
					      NULL);
	settings->height = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_HEIGHT,
					      NULL);

	settings->sel[0] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_SELR,
					      NULL);
	settings->sel[1] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_SELG,
					      NULL);
	settings->sel[2] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_SELB,
					      NULL);
	
	settings->st[0] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_STR,
					      NULL);
	settings->st[1] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_STG,
					      NULL);
	settings->st[2] = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_STG,
					      NULL);
	
	settings->print_wrap_lines = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_PRINTWRAP,
					      NULL);
	settings->print_header = gconf_client_get_bool (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_PRINTHEADER,
					      NULL);
	settings->print_lines = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_PRINTLINES,
					      NULL);
	settings->print_orientation = gconf_client_get_int (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_PRINTORIENTATION,
					      NULL);
	settings->papersize = gconf_client_get_string (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_PAPERSIZE,
					      NULL);
#if 0 /* FIXME */
	if (settings->papersize == NULL)
		settings->papersize = g_strdup (gnome_paper_name_default());
#endif
	if (settings->font != NULL)
		g_free (settings->font);
	
	settings->font = gconf_client_get_string (gconf_client,
					      GEDIT_BASE_KEY GEDIT_PREF_FONT,
					      NULL);
	
	if (settings->font == NULL)
		settings->font = g_strdup (DEFAULT_FONT);
	

#if 0 /* FIXME */
	if (mdi)
		tab_pos (settings->tab_pos);

	/*
	if (mdi)
	{
		if (mdi->active_window && !settings->show_status)
			gtk_widget_hide (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
	}
	else
	{
		settings->show_status = TRUE;
		gnome_config_set_int ("show_statusbar",
				      (gboolean) settings->show_status);
				      }*/
#endif
	
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
	gconf_client = gconf_client_get_default ();

	gconf_client_add_dir (gconf_client,
			      GEDIT_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);
	
	gconf_client_notify_add (gconf_client,
				 GEDIT_BASE_KEY,
				 gedit_prefs_notify_cb,
				 NULL, NULL, NULL);
}
