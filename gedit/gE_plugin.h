/*
 * gEdit
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

#ifndef __GE_PLUGIN_H__
#define __GE_PLUGIN_H__

typedef struct _gE_Plugin_Data gE_Plugin_Data;

#include <gmodule.h>

enum {
	PLUGIN_OK,
	PLUGIN_ERROR,
	PLUGIN_DEAD
};

struct _gE_Plugin_Data {
	
	gchar	*file;
	GModule	*handle;
	
	gint	(*init_plugin) 		(gE_Plugin_Data *);
	gint	(*can_unload)		(gE_Plugin_Data *);
	void	(*destroy_plugin)	(gE_Plugin_Data *);
	
	gchar	*name;
	gchar	*desc;
	gchar	*author;
	
	/* filled in by plugin */
	void	*private_data;
	
};

extern GSList	*plugin_list;

/* Plugin MUST have this function */
extern gint init_plugin (gE_Plugin_Data *pd);

void		 gE_plugins_init 	();
gE_Plugin_Data	*plugin_load 		(const gchar *file);
void		 plugin_unload		(gE_Plugin_Data *pd);

void		 gE_plugins_window_add (GnomeApp *app);

#endif /* __GE_PLUGIN_H__ */