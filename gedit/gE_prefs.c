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
#include "toolbar.h"


static char *rc;

void 
gE_rc_parse(void)
{
	/*if ((rc = gE_prefs_open_file ("gtkrc", "r")) == NULL)
	{
		printf ("gE_rc_parse: Couldn't open gtk rc file for parsing.\n");
		return;
	}*/
	rc = gE_prefs_open_file ("gtkrc", "r");
	gtk_rc_parse(rc);
}

void 
gE_save_settings()
{
/*	window = (gE_window *) cbwindow;*/

/*	gE_prefs_set_int("tab pos", (gint) settings->tab_pos);*/
	gE_prefs_set_int("auto indent", (gboolean) settings->auto_indent);
	gE_prefs_set_int("show statusbar", (gboolean) settings->show_status);
	gE_prefs_set_int("toolbar", (gint) settings->have_toolbar);
	gE_prefs_set_int("tb text", (gint) settings->have_tb_text);
	gE_prefs_set_int("tb pix", (gint) settings->have_tb_pix);
	gE_prefs_set_int("tb relief", (gint) settings->use_relief_toolbar);
	gE_prefs_set_int("splitscreen", (gint) settings->splitscreen);

	gE_prefs_set_char("font", settings->font);
	if (settings->print_cmd == "")
		gE_prefs_set_char ("print command", "lpr -rs %s");
	else
		gE_prefs_set_char ("print command", settings->print_cmd);

}

void gE_get_settings()
{
	
/*	 settings->tab_pos = gE_prefs_get_int("tab pos");*/
	 settings->show_status = gE_prefs_get_int("show statusbar");
	 settings->have_toolbar = gE_prefs_get_int("toolbar");
	 settings->have_tb_text = gE_prefs_get_int("tb text");
	 settings->have_tb_pix = gE_prefs_get_int("tb pix");
	 settings->use_relief_toolbar = gE_prefs_get_int("tb relief");
	 settings->splitscreen = gE_prefs_get_int("splitscreen");
	 settings->font = gE_prefs_get_char("font");
	 if (settings->font == NULL)
	   settings->font = "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1";
	 settings->print_cmd = gE_prefs_get_char("print command"); 
	 if (settings->print_cmd == NULL)
	   settings->print_cmd = "lpr %s";


/*bOrK	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), w->tab_pos);*/

	if (settings->show_status == FALSE)
	  gtk_widget_hide (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
}
