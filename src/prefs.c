/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/* TODO:
 * [ ] stop saving everything in /gedit/Global/, split config file
 * into sections
 *
 */

#include <config.h>
#include <gnome.h>

#include "prefs.h"
#include "view.h"
#include "commands.h"
#include "document.h"
#include "utils.h"
#include "window.h"
#include "plugin.h"


Preferences *settings = NULL;

#define DEFAULT_FONT (const gchar*) "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1"
#define DEFAULT_FONTSET "-*-*-medium-r-normal-*-14-*-*-*-*-*-*-*,*"

void 
gedit_prefs_save_settings (void)
{
	GdkWindow *toplevel;
	gint root_x;
	gint root_y;

	gedit_debug (DEBUG_PREFS, "start");
	
	gnome_config_push_prefix ("/gedit/Global/");

	gnome_config_set_int ("tab_pos", (gint) settings->tab_pos);
	gnome_config_set_int ("mdi_mode", settings->mdi_mode);

	gnome_config_set_bool ("auto_indent", settings->auto_indent);
	gnome_config_set_bool ("word_wrap", settings->word_wrap);
	gnome_config_set_bool ("show_statusbar", settings->show_status);

	gnome_config_set_int ("toolbar_labels", settings->toolbar_labels);
	gnome_config_set_int ("toolbar", (gint) settings->have_toolbar);
	gnome_config_set_int ("tb_text", (gint) settings->have_tb_text);
	gnome_config_set_int ("tb_pix", (gint) settings->have_tb_pix);
	gnome_config_set_int ("tb_relief", (gint) settings->use_relief_toolbar);
	gnome_config_set_int ("splitscreen", (gint) settings->splitscreen);
	gnome_config_set_int ("close_doc", (gint) settings->close_doc);
	gnome_config_set_int ("undo_levels", (gint) settings->undo_levels);

	gnome_config_set_int ("bgr", settings->bg[0]);
	gnome_config_set_int ("bgg", settings->bg[1]);
	gnome_config_set_int ("bgb", settings->bg[2]);
	gnome_config_set_int ("fgr", settings->fg[0]);
	gnome_config_set_int ("fgg", settings->fg[1]);
	gnome_config_set_int ("fgb", settings->fg[2]);

	if (gedit_window_active ()) {
		toplevel = gdk_window_get_toplevel (GTK_WIDGET (gedit_window_active())->window);
		gdk_window_get_root_origin (toplevel, &root_x, &root_y);
		/* We don't want to save the size of a maximized window,
		   so chek the left margin. This will not work if there is
		   a pannel in left, but there is no other way that I know
		   of knowing if a window is maximzed or not */
		if (root_x != 0)
			if (gedit_window_active())
				gdk_window_get_size (GTK_WIDGET (gedit_window_active())->window,
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

	if (!settings->run)
		settings->run = TRUE;
	
	gnome_config_set_int ("run", (gint) settings->run);
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();

	gedit_plugin_save_settings ();
	
	gedit_debug (DEBUG_PREFS, "end");
}

void
gedit_prefs_load_settings (void)
{
	gchar * mdi_mode_string;
	gedit_debug (DEBUG_PREFS, "");

	if (!settings)
	{
		settings = g_malloc (sizeof (Preferences));
	}

	gnome_config_push_prefix ("/gedit/Global/");

	settings->tab_pos = gnome_config_get_int ("tab_pos=2");
	mdi_mode_string = g_strdup_printf ("mdi_mode=%i", GNOME_MDI_NOTEBOOK);
	settings->mdi_mode = gnome_config_get_int (mdi_mode_string);
	g_free (mdi_mode_string);

	settings->auto_indent = gnome_config_get_bool ("auto_indent");
	settings->word_wrap = gnome_config_get_bool ("word_wrap");
	settings->run = gnome_config_get_int ("run");
	
	settings->show_status = gnome_config_get_bool ("show_statusbar=TRUE");
	settings->toolbar_labels = gnome_config_get_int ("toolbar_labels");
	settings->have_toolbar = gnome_config_get_int ("toolbar");
	settings->have_tb_text = gnome_config_get_int ("tb text");
	settings->have_tb_pix = gnome_config_get_int ("tb_pix");
	settings->use_relief_toolbar = gnome_config_get_int("tb_relief");
	settings->splitscreen = gnome_config_get_int("splitscreen");
	settings->close_doc = gnome_config_get_int ("close_doc");
	settings->undo_levels = gnome_config_get_int ("undo_levels=25");

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
	if (settings->papersize == NULL)
		settings->papersize = g_strdup (gnome_paper_name_default());
#if 0 	/* This is ugly !. Chema */
	if (strlen (settings->papersize)<1)
	{
		g_free (settings->papersize);
		strcpy (settings->papersize, gnome_paper_name_default());
	}
#endif	

	settings->use_fontset = FALSE;
	settings->font = gnome_config_get_string ("font");
	if (settings->font == NULL)
		settings->font = g_strdup (DEFAULT_FONT);
	
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

	gnome_config_pop_prefix ();
	gnome_config_sync ();
}
