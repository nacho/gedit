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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <gmodule.h>


enum
{
	PLUGIN_OK,
	PLUGIN_ERROR,
	PLUGIN_DEAD
};

typedef struct _PluginData PluginData;

struct _PluginData
{
	gchar	*file;
	GModule	*handle;
	
	gint	(*init_plugin) 		(PluginData *);
	gint	(*can_unload)		(PluginData *);
	void	(*destroy_plugin)	(PluginData *);
	
	gchar	*name;
	gchar	*desc;
	gchar	*long_desc;
	gchar	*author;

	/* if the plugin needs an open document to
	   work, and there aren't any open documents.
	   set sentivity off in the plugins menu */
	gint needs_a_document;

	gint modifies_an_existing_doc;

#if 0 /* It is not needed*/
	GtkWidget * menu_item;
#endif 	

	/* filled in by plugin */
	void	*private_data;

	/* Is this plugin installed ? */
	gint installed;
	gint installed_by_default;
};

extern GList	*plugins_list;
extern GList	*plugins_installed_list;

/* Plugin MUST have this function */
/*extern gint init_plugin (PluginData *pd); */

gint init_plugin (PluginData *pd);

void			gedit_plugins_init (void);
void			gedit_plugins_menu_add (GnomeApp *app);

void			gedit_plugin_save_settings (void);

gchar *			gedit_plugin_program_location_get (gchar *program_name, gchar *plugin_name, gint dont_guess);
gchar *			gedit_plugin_program_location_change (gchar * program_name, gchar * plugin_name);

#endif /* __PLUGIN_H__ */
