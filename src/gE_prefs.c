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
#include <stdio.h>
#include <string.h>
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include "main.h"
#include "gE_prefs.h"
/*#include "toolbar.h"*/
#include "gE_mdi.h"


void 
gE_save_settings()
{
	gnome_config_push_prefix ("/gEdit/Global/");

/*	gE_prefs_set_int ("tab pos", (gint) settings->tab_pos);*/
	gnome_config_set_int ("auto indent", (gboolean) settings->auto_indent);
	gnome_config_set_int ("show statusbar", (gboolean) settings->show_status);
	gnome_config_set_int ("toolbar", (gint) settings->have_toolbar);
	gnome_config_set_int ("tb text", (gint) settings->have_tb_text);
	gnome_config_set_int ("tb pix", (gint) settings->have_tb_pix);
	gnome_config_set_int ("tb relief", (gint) settings->use_relief_toolbar);
	gnome_config_set_int ("splitscreen", (gint) settings->splitscreen);
	gnome_config_set_int ("close doc", (gint) settings->close_doc);
	gnome_config_set_int ("mdi mode", mdiMode);
	gnome_config_set_int ("scrollbar", settings->scrollbar);
	gnome_config_set_string ("font", settings->font);
	gnome_config_set_int ("width", (gint) settings->width);
	gnome_config_set_int ("height", (gint) settings->height);
	
	if (settings->print_cmd == "")
	  gnome_config_set_string ("print command", "lpr -rs %s");
	else
	  gnome_config_set_string ("print command", settings->print_cmd);
	
	if (!settings->run)
	  settings->run = TRUE;
	
	gnome_config_set_int ("run", (gint) settings->run);
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}

void gE_get_settings()
{
	 
	gnome_config_push_prefix ("/gEdit/Global/");
	 
/*	settings->tab_pos = gE_prefs_get_int("tab pos");*/
	settings->run = gnome_config_get_int ("run");
	settings->show_status = gnome_config_get_int ("show statusbar");
	settings->have_toolbar = gnome_config_get_int ("toolbar");
	settings->have_tb_text = gnome_config_get_int ("tb text");
	settings->have_tb_pix = gnome_config_get_int ("tb pix");
	settings->use_relief_toolbar = gnome_config_get_int("tb relief");
	settings->splitscreen = gnome_config_get_int("splitscreen");
	settings->close_doc = gnome_config_get_int ("close doc");
	mdiMode = gnome_config_get_int ("mdi mode");
	if (!mdiMode)
	  mdiMode = mdi_type[GNOME_MDI_NOTEBOOK];
	   
	settings->scrollbar = gnome_config_get_int ("scrollbar");
	if (!settings->scrollbar)
	  settings->scrollbar = GTK_POLICY_AUTOMATIC;
	 
	settings->width = gnome_config_get_int ("width");
	if (!settings->width)
	  settings->width = 630;

	settings->height = gnome_config_get_int ("height");
	if (!settings->height)
	  settings->height = 390;
	 
	settings->font = gnome_config_get_string ("font");
	if (settings->font == NULL) {
	
	  if (use_fontset)
	     settings->font = DEFAULT_FONTSET;
	   else
	     settings->font = DEFAULT_FONT;
	
	}
	
	settings->print_cmd = gnome_config_get_string ("print command"); 
	if (settings->print_cmd == NULL)
	  settings->print_cmd = "lpr %s";


	if (settings->run) {
	
	  if (settings->show_status == FALSE)
	    gtk_widget_hide (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
	
	} else {
	
	 settings->show_status = TRUE;
	 gnome_config_set_int ("show statusbar", (gboolean) settings->show_status);
	
	}
	
	gnome_config_pop_prefix ();
	gnome_config_sync ();
	
}
