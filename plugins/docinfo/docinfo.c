/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * docinfo.c
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

#include <pango/pango-break.h>

#include "gedit-plugin.h"
#include "gedit-debug.h"

#define MENU_ITEM_LABEL		N_("_Word Count")
#define MENU_ITEM_PATH		"/menu/Search/SearchOps_3/"
#define MENU_ITEM_NAME		"PluginWordCount"	
#define MENU_ITEM_TIP		N_("Get info on current document.")

static void	word_count_cb (BonoboUIComponent *uic, gpointer user_data, 
			       const gchar* verbname);
static void	word_count_real ();

static void
word_count_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_PLUGINS, "");

	word_count_real ();
}

static void
word_count_real ()
{
	GeditDocument 	*doc;
	gchar		*text;
	PangoLogAttr 	*attrs;
	gint		 words;
	gint		 chars;
	gint		 chars_non_space;
	gint		 sentences;
	gint		 lines;

	gedit_debug (DEBUG_PLUGINS, "");

	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	text = gedit_document_get_buffer (doc);

	g_return_if_fail (g_utf8_validate (text, -1, NULL));
  
	chars = g_utf8_strlen (text, -1);
 	attrs = g_new0 (PangoLogAttr, chars + 1);

	pango_get_log_attrs (text,
                       -1,
                       0,
                       pango_language_from_string ("C"),
                       attrs,
                       chars + 1);

	g_free (attrs);
	g_free (text);
}

G_MODULE_EXPORT GeditPluginState
word_count_update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIEngine *ui_engine;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	ui_engine = bonobo_window_get_ui_engine (window);

	doc = gedit_get_active_document ();

	if (doc == NULL)		
		gedit_menus_set_verb_sensitive (ui_engine, "/commands/" MENU_ITEM_NAME, FALSE);
	else
		gedit_menus_set_verb_sensitive (ui_engine, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
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
		
		item_path = g_strdup (MENU_ITEM_PATH MENU_ITEM_NAME);

		if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			gchar *xml;
					
			xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"\""
					" _label=\"%s\" _tip=\"%s\" hidden=\"0\" />", 
					MENU_ITEM_NAME, MENU_ITEM_LABEL, MENU_ITEM_TIP);

			bonobo_ui_component_set_translate (ui_component, 
					MENU_ITEM_PATH, xml, NULL);

			bonobo_ui_component_set_translate (ui_component, 
					"/commands/", "<cmd name = \"" MENU_ITEM_NAME "\" />", NULL);

			bonobo_ui_component_add_verb (ui_component, 
					MENU_ITEM_NAME, word_count_cb, 
					      NULL); 

			g_free (xml);
		}
		
		g_free (item_path);
		
		top_windows = g_list_next (top_windows);
	}
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
		
		item_path = g_strdup (MENU_ITEM_PATH MENU_ITEM_NAME);

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			bonobo_ui_component_rm (ui_component, item_path, NULL);
			bonobo_ui_component_rm (ui_component, "/commands/" MENU_ITEM_NAME, NULL);
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
     
	pd->update_ui = word_count_update_ui;
	
	pd->name = _("Word count");
	pd->desc = _("The word count plugin analyzes the current document and determines the number "
		     "of words, sentences, lines, characters and non-space characters in it "
		     "and display the result.");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->copyright = _("Copyright (C) 2002 - Paolo Maggi");
	
	pd->private_data = NULL;
		
	return PLUGIN_OK;
}




