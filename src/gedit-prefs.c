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
#include "gedit-prefs.h"
#include "gedit-debug.h"
#include "gedit2.h"

GeditPreferences *settings = NULL;

#define DEFAULT_FONT (const gchar*) "Sans 12"

void 
gedit_prefs_save_settings (void)
{
	BonoboWindow* active_window;
	GdkWindow *toplevel;
	gint root_x;
	gint root_y;

	gedit_debug (DEBUG_PREFS, "start");
	
	gnome_config_push_prefix ("/gedit2/Global/");

	gnome_config_set_int ("tab_pos", (gint) settings->tab_pos);
	gnome_config_set_int ("mdi_mode", settings->mdi_mode);

	gnome_config_set_bool ("auto_indent", settings->auto_indent);
	gnome_config_set_bool ("wrap_mode", settings->wrap_mode);
	gnome_config_set_bool ("show_statusbar", settings->show_status);

	gnome_config_set_int ("toolbar_labels", settings->toolbar_labels);
	gnome_config_set_int ("toolbar", (gint) settings->have_toolbar);
	gnome_config_set_bool ("show_tooltips", settings->show_tooltips);

	gnome_config_set_int ("splitscreen", (gint) settings->splitscreen);
	gnome_config_set_int ("undo_levels", (gint) settings->undo_levels);
	gnome_config_set_int ("tab_size", (gint) settings->tab_size);

	gnome_config_set_bool ("use_default_font", settings->use_default_font);
	gnome_config_set_bool ("use_default_colors", settings->use_default_colors);

	gnome_config_set_int ("bgr", settings->bg[0]);
	gnome_config_set_int ("bgg", settings->bg[1]);
	gnome_config_set_int ("bgb", settings->bg[2]);
	gnome_config_set_int ("fgr", settings->fg[0]);
	gnome_config_set_int ("fgg", settings->fg[1]);
	gnome_config_set_int ("fgb", settings->fg[2]);
	
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
		gnome_config_set_int ("width", (gint) settings->width);
		gnome_config_set_int ("height", (gint) settings->height);
	}
	
	gnome_config_set_bool ("printwrap", settings->print_wrap_lines);
	gnome_config_set_bool ("printheader", settings->print_header);
	gnome_config_set_int  ("printlines", settings->print_lines);
	gnome_config_set_string ("papersize", settings->papersize);
	gnome_config_set_int ("printorientation", settings->print_orientation);
	gnome_config_set_string ("font", settings->font);

	if (!settings->run) {
		settings->run = TRUE;
	
		gnome_config_set_int ("run", (gint) settings->run);
	}
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();

#if 0 /* FIXME */
	gedit_plugin_save_settings ();
#endif	
	gedit_debug (DEBUG_PREFS, "end");
}

void
gedit_prefs_load_settings (void)
{
	gchar * mdi_mode_string;
	gedit_debug (DEBUG_PREFS, "");

	if (!settings)
	{
		settings = g_new0 (GeditPreferences, 1);
	}

	gnome_config_push_prefix ("/gedit2/Global/");

	settings->tab_pos = gnome_config_get_int ("tab_pos=2");
	mdi_mode_string = g_strdup_printf ("mdi_mode=%i", BONOBO_MDI_NOTEBOOK);
	settings->mdi_mode = gnome_config_get_int (mdi_mode_string);
	g_free (mdi_mode_string);

	settings->auto_indent = gnome_config_get_bool ("auto_indent");
	settings->wrap_mode = gnome_config_get_bool ("wrap_mode");
	settings->run = gnome_config_get_int ("run");
	
	settings->show_status = gnome_config_get_bool ("show_statusbar=TRUE"); 
	settings->toolbar_labels = gnome_config_get_int ("toolbar_labels");
	settings->have_toolbar = gnome_config_get_int ("toolbar=1");
	settings->show_tooltips = gnome_config_get_bool ("show_tooltips=TRUE"); 

	settings->splitscreen = gnome_config_get_int("splitscreen");
	settings->undo_levels = gnome_config_get_int ("undo_levels=25");
	settings->tab_size = gnome_config_get_int ("tab_size=8");

	settings->use_default_font = gnome_config_get_bool ("use_default_font=TRUE"); 
	settings->use_default_colors = gnome_config_get_bool ("use_default_colors=TRUE"); 

	settings->bg[0] = gnome_config_get_int ("bgr=65535");
	settings->bg[1] = gnome_config_get_int ("bgg=65535");
	settings->bg[2] = gnome_config_get_int ("bgb=65535");
	
	settings->fg[0] = gnome_config_get_int ("fgr=0");
	settings->fg[1] = gnome_config_get_int ("fgg=0");
	settings->fg[2] = gnome_config_get_int ("fgb=0");
	
	settings->width = gnome_config_get_int ("width=600");
	settings->height = gnome_config_get_int ("height=400");
	 
	settings->print_wrap_lines = gnome_config_get_bool ("printwrap=TRUE");
	settings->print_header = gnome_config_get_bool ("printheader=TRUE");
	settings->print_lines = gnome_config_get_int ("printlines=0");
	settings->print_orientation = gnome_config_get_int ("printorientation=1");
	settings->papersize = gnome_config_get_string ("papersize");
#if 0 /* FIXME */
	if (settings->papersize == NULL)
		settings->papersize = g_strdup (gnome_paper_name_default());
#endif
	if (settings->font != NULL)
		g_free (settings->font);
	
	settings->font = gnome_config_get_string ("font");
	
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
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}
