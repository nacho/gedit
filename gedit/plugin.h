/*
 * gedit
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, 
 * and Chris Lahey
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

#ifndef __GEDIT_PLUGIN_H__
#define __GEDIT_PLUGIN_H__

typedef struct _PluginData PluginData;

#include <gmodule.h>

enum {
	PLUGIN_OK,
	PLUGIN_ERROR,
	PLUGIN_DEAD
};

struct _PluginData {
	gchar	*file;
	GModule	*handle;
	
	gint	(*init_plugin) 		(PluginData *);
	gint	(*can_unload)		(PluginData *);
	void	(*destroy_plugin)	(PluginData *);
	
	gchar	*name;
	gchar	*desc;
	gchar	*author;
	
	/* filled in by plugin */
	void	*private_data;
};

extern GSList	*plugin_list;

/* Plugin MUST have this function */
/*extern gint init_plugin (PluginData *pd); */

PluginData* plugin_load              (const gchar *file);
void        gedit_plugins_init       (void);
void        plugin_unload            (PluginData *pd);

void        gedit_plugins_window_add (GnomeApp *app);

#endif /* __GEDIT_PLUGIN_H__ */
