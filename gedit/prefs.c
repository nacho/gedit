/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


Preferences *settings = NULL;


void 
gedit_save_settings (void)
{
	gedit_debug_mess ("F:gedit_save_settings\n", DEBUG_PREFS);
	
	gnome_config_push_prefix ("/gedit/Global/");

	gnome_config_set_int ("tab_pos", (gint) settings->tab_pos);
	gnome_config_set_int ("mdi_mode", settings->mdi_mode);

	gnome_config_set_bool ("auto_indent", settings->auto_indent);
	gnome_config_set_bool ("word_wrap", settings->word_wrap);
	gnome_config_set_bool ("show_statusbar", settings->show_status);
	gnome_config_set_int ("toolbar", (gint) settings->have_toolbar);
	gnome_config_set_int ("tb_text", (gint) settings->have_tb_text);
	gnome_config_set_int ("tb_pix", (gint) settings->have_tb_pix);
	gnome_config_set_int ("tb_relief", (gint) settings->use_relief_toolbar);
	gnome_config_set_int ("splitscreen", (gint) settings->splitscreen);
	gnome_config_set_int ("close_doc", (gint) settings->close_doc);

	gnome_config_set_int ("bgr", settings->bg[0]);
	gnome_config_set_int ("bgg", settings->bg[1]);
	gnome_config_set_int ("bgb", settings->bg[2]);
	gnome_config_set_int ("fgr", settings->fg[0]);
	gnome_config_set_int ("fgg", settings->fg[1]);
	gnome_config_set_int ("fgb", settings->fg[2]);

	if (GTK_IS_WIDGET (mdi->active_window))
		gdk_window_get_size (GTK_WIDGET (mdi->active_window)->window,
				     &settings->width, &settings->height);
	gnome_config_set_int ("width", (gint) settings->width);
	gnome_config_set_int ("height", (gint) settings->height);


	gnome_config_set_bool ("printwrap", settings->printwrap);
	gnome_config_set_bool ("printheader", settings->printheader);
	gnome_config_set_int  ("printlines", settings->printlines);
	gnome_config_set_string ("papersize", settings->papersize);
	gnome_config_set_string ("font", settings->font);

	if (!settings->run)
		settings->run = TRUE;
	
	gnome_config_set_int ("run", (gint) settings->run);
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}

/* Determine we use fonts or fontsets. If a fontset is supplied for
 * text widgets, we use fontsets for drawing texts. Otherwise we use
 * normal fonts instead.
 */
static gboolean
prefs_determine_use_fontset (void)
{
	GtkWidget *dummy_widget;
	gboolean retval;

	dummy_widget = gtk_text_new (NULL, NULL);
	gtk_widget_ensure_style (dummy_widget);
	if (dummy_widget->style->font->type == GDK_FONT_FONTSET)
		retval = TRUE;
	else
		retval = FALSE;

	gtk_widget_destroy (dummy_widget);

	return retval;
}

void
gedit_load_settings (void)
{
	gedit_debug_mess ("F:gedit_load_settings\n", DEBUG_PREFS);

	if (!settings)
	{
		settings = g_malloc (sizeof (Preferences));
		settings->num_recent = 0;
	}

	gnome_config_push_prefix ("/gedit/Global/");

	settings->tab_pos = gnome_config_get_int ("tab_pos=2");
	settings->mdi_mode = gnome_config_get_int ("mdi_mode=42");

	settings->auto_indent = gnome_config_get_bool ("auto_indent");
	settings->word_wrap = gnome_config_get_bool ("word_wrap");
	settings->run = gnome_config_get_int ("run");
	settings->show_status = gnome_config_get_bool ("show_statusbar");
	settings->have_toolbar = gnome_config_get_int ("toolbar");
	settings->have_tb_text = gnome_config_get_int ("tb text");
	settings->have_tb_pix = gnome_config_get_int ("tb_pix");
	settings->use_relief_toolbar = gnome_config_get_int("tb_relief");
	settings->splitscreen = gnome_config_get_int("splitscreen");
	settings->close_doc = gnome_config_get_int ("close_doc");

	settings->bg[0] = gnome_config_get_int ("bgr=65535");
	settings->bg[1] = gnome_config_get_int ("bgg=65535");
	settings->bg[2] = gnome_config_get_int ("bgb=65535");
	
	settings->fg[0] = gnome_config_get_int ("fgr=0");
	settings->fg[1] = gnome_config_get_int ("fgg=0");
	settings->fg[2] = gnome_config_get_int ("fgb=0");
	
	settings->width = gnome_config_get_int ("width=600");
	settings->height = gnome_config_get_int ("height=400");
	 
	settings->printwrap = gnome_config_get_bool ("printwrap=TRUE");
	settings->printheader = gnome_config_get_bool ("printheader=TRUE");
	settings->printlines = gnome_config_get_int ("printlines=0");
	settings->papersize = gnome_config_get_string ("papersize");
	if (settings->papersize==NULL)
		settings->papersize = g_strdup (gnome_paper_name_default());
	if (strlen (settings->papersize)<1)
	{
		g_free (settings->papersize);
		strcpy (settings->papersize, gnome_paper_name_default());
	}

	settings->use_fontset = prefs_determine_use_fontset ();
	settings->font = gnome_config_get_string ("font");
	if (settings->font == NULL)
	{
		if (settings->use_fontset)
			settings->font = DEFAULT_FONTSET;
		else
			settings->font = DEFAULT_FONT;
	}
	
	if (mdi)
		tab_pos (settings->tab_pos);

	if (settings->run && mdi)
	{
		if (mdi->active_window && !settings->show_status)
			gtk_widget_hide (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
	}
	else
	{
		settings->show_status = TRUE;
		gnome_config_set_int ("show_statusbar",
				      (gboolean) settings->show_status);
	}
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}
