/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
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

#include <config.h>
#include <gnome.h>
#include "gedit.h"
#include "gE_prefs.h"
#include "gE_mdi.h"

void 
gedit_save_settings (void)
{
	gnome_config_push_prefix ("/gEdit/Global/");

	gnome_config_set_int ("tab_pos", (gint) mdi->tab_pos);
	gnome_config_set_int ("auto_indent", (gboolean) settings->auto_indent);
	gnome_config_set_int ("word_wrap", (gboolean) settings->word_wrap);
	gnome_config_set_int ("show_statusbar", (gboolean) settings->show_status);
	gnome_config_set_int ("toolbar", (gint) settings->have_toolbar);
	gnome_config_set_int ("tb_text", (gint) settings->have_tb_text);
	gnome_config_set_int ("tb_pix", (gint) settings->have_tb_pix);
	gnome_config_set_int ("tb_relief", (gint) settings->use_relief_toolbar);
	gnome_config_set_int ("splitscreen", (gint) settings->splitscreen);
	gnome_config_set_int ("close_doc", (gint) settings->close_doc);
	gnome_config_set_int ("mdi_mode", mdiMode);

	gnome_config_set_int ("bgr", settings->bg[0]);
	gnome_config_set_int ("bgg", settings->bg[1]);
	gnome_config_set_int ("bgb", settings->bg[2]);
	gnome_config_set_int ("fgr", settings->fg[0]);
	gnome_config_set_int ("fgg", settings->fg[1]);
	gnome_config_set_int ("fgb", settings->fg[2]);

	gnome_config_set_int ("width", (gint) settings->width);
	gnome_config_set_int ("height", (gint) settings->height);
	gnome_config_set_string ("font", settings->font);

	if (settings->print_cmd == "")
		gnome_config_set_string ("print_command", "lpr -rs %s");
	else
		gnome_config_set_string ("print_command", settings->print_cmd);
	
	if (!settings->run)
		settings->run = TRUE;
	
	gnome_config_set_int ("run", (gint) settings->run);
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}

void
gedit_load_settings (void)
{
	gnome_config_push_prefix ("/gEdit/Global/");
	 
	mdi->tab_pos = gnome_config_get_int ("tab_pos");
	settings->auto_indent = gnome_config_get_int ("auto_indent");
	settings->word_wrap = gnome_config_get_int ("word_wrap");
	settings->run = gnome_config_get_int ("run");
	settings->show_status = gnome_config_get_int ("show_statusbar");
	settings->have_toolbar = gnome_config_get_int ("toolbar");
	settings->have_tb_text = gnome_config_get_int ("tb text");
	settings->have_tb_pix = gnome_config_get_int ("tb_pix");
	settings->use_relief_toolbar = gnome_config_get_int("tb_relief");
	settings->splitscreen = gnome_config_get_int("splitscreen");
	settings->close_doc = gnome_config_get_int ("close_doc");
	mdiMode = gnome_config_get_int ("mdi_mode");
	/*if (!mdiMode)
	  mdiMode = mdi_type[GNOME_MDI_NOTEBOOK];*/
	settings->bg[0] = gnome_config_get_int( "bgr=65535" );
	settings->bg[1] = gnome_config_get_int( "bgg=65535" );
	settings->bg[2] = gnome_config_get_int( "bgb=65535" );
	
	settings->fg[0] = gnome_config_get_int( "fgr=0" );
	settings->fg[1] = gnome_config_get_int( "fgg=0" );
	settings->fg[2] = gnome_config_get_int( "fgb=0" );
	
	settings->width = gnome_config_get_int ("width=630");
	settings->height = gnome_config_get_int ("height=390");

	 
	settings->font = gnome_config_get_string ("font");
	if (settings->font == NULL)
	{
		if (use_fontset)
			settings->font = DEFAULT_FONTSET;
		else
			settings->font = DEFAULT_FONT;
	}
	
	settings->print_cmd = gnome_config_get_string ("print_command=lpr %s"); 

	if (settings->run)
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










