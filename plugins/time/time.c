/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-document.h
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <libgnome/gnome-i18n.h>
#include <time.h>
#include "gedit-plugin.h"
#include "gedit-debug.h"

/* Gratiously ripped out of GIMP (app/general.c), with a fiew minor changes */
static char *
get_time (void)
{
	static char static_buf[21];
  	gchar *tmp, *out = NULL;
  	time_t clock;
  	struct tm *now;
  	const char *format = NULL;
  	size_t out_length = 0;
  	

  	clock = time (NULL);
  	/*now = gmtime (&clock);*/
  	now = localtime (&clock);
	  	
  	tmp = static_buf;
	
  	/* date format derived from ISO 8601:1988 */
  	/*sprintf(tmp, "%04d-%02d-%02d%c%02d:%02d:%02d%c",
	  now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
	  ' ',
	  now->tm_hour, now->tm_min, now->tm_sec,
	  '\000'); 
	
	  return tmp;
	*/
	format = "%a %b %e %H:%M:%S %Z %Y";

	do
	{
		out_length += 200;
		out = (char *) realloc (out, out_length);
	}
  	while (strftime (out, out_length, format, now) == 0);

  	
  	return out;
}


static void
time_world_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
	GeditView *view;
	gchar *the_time;
	
	gedit_debug (DEBUG_PLUGINS, "");

	view = gedit_get_active_view ();
	g_return_if_fail (view != NULL);
	
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	the_time = get_time ();

	gedit_document_begin_user_action (doc);
	
	gedit_document_insert_text_at_cursor (doc, the_time, -1);

	gedit_document_end_user_action (doc);
}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIEngine *ui_engine;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	ui_engine = bonobo_window_get_ui_engine (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || (gedit_document_is_readonly (doc)))		
		gedit_menus_set_verb_sensitive (ui_engine, "/commands/Time", FALSE);
	else
		gedit_menus_set_verb_sensitive (ui_engine, "/commands/Time", TRUE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");
}
	
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList* top_windows;
	
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);
       
	while (top_windows)
	{
		BonoboUIComponent* ui_component;
		gchar *item_path;
		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
		
		item_path = g_strdup ("/menu/Tools/ToolsOps/Time");

		if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			gchar *xml;
					
			xml = g_strdup ("<menuitem name=\"Time\" verb=\"\""
					" _label=\"_Time\""
				        " _tip=\"Print current time\" hidden=\"0\" />");

			bonobo_ui_component_set_translate (ui_component, 
					"/menu/Tools/ToolsOps/", xml, NULL);

			bonobo_ui_component_set_translate (ui_component, 
					"/commands/", "<cmd name = \"Time\" />", NULL);

			bonobo_ui_component_add_verb (ui_component, 
					"Time", time_world_cb, 
					      NULL); 

			g_free (xml);
		}
		
		g_free (item_path);
		
		top_windows = g_list_next (top_windows);
	}

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	GList* top_windows;
	
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);
       
	while (top_windows)
	{
		BonoboUIComponent* ui_component;
		gchar *item_path;
		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
		
		item_path = g_strdup ("/menu/Tools/ToolsOps/Time");

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			bonobo_ui_component_rm (ui_component, item_path, NULL);
			bonobo_ui_component_rm (ui_component, "/commands/Time", NULL);
		}
		
		g_free (item_path);
		
		top_windows = g_list_next (top_windows);
	}
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->update_ui = update_ui;
	pd->destroy = destroy;
	
	pd->name = _("Time");
	pd->desc = _("Inserts the current time into the current document.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->copyright = _("Copyright (C) 2002 - Alex Roberts");
	
	pd->private_data = NULL;
		
	return PLUGIN_OK;
}




