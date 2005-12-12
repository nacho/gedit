/*
 * gedit-plugins-engine.h
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __GEDIT_PLUGINS_ENGINE_H__
#define __GEDIT_PLUGINS_ENGINE_H__

#include <glib.h>
#include <gedit/gedit-window.h>

typedef struct _GeditPluginInfo GeditPluginInfo;

gboolean	 gedit_plugins_engine_init 		(void);
void		 gedit_plugins_engine_shutdown 		(void);

const GList	*gedit_plugins_engine_get_plugins_list 	(void);

gboolean 	 gedit_plugins_engine_activate_plugin 	(GeditPluginInfo *info);
gboolean 	 gedit_plugins_engine_deactivate_plugin	(GeditPluginInfo *info);
gboolean 	 gedit_plugins_engine_plugin_is_active 	(GeditPluginInfo *info);

gboolean	 gedit_plugins_engine_plugin_is_configurable 
							(GeditPluginInfo *info);
void	 	 gedit_plugins_engine_configure_plugin	(GeditPluginInfo *info, 
							 GtkWindow       *parent);

/* 
 * new_window == TRUE if this function is called because a new top window
 * has been created
 */
void		 gedit_plugins_engine_update_plugins_ui (GeditWindow     *window, 
							 gboolean         new_window);


const gchar	*gedit_plugins_engine_get_plugin_name	(GeditPluginInfo *info);
const gchar	*gedit_plugins_engine_get_plugin_description
							(GeditPluginInfo *info);

const gchar    **gedit_plugins_engine_get_plugin_authors
							(GeditPluginInfo *info);
const gchar	*gedit_plugins_engine_get_plugin_website
							(GeditPluginInfo *info);
const gchar	*gedit_plugins_engine_get_plugin_copyright
							(GeditPluginInfo *info);

#endif  /* __GEDIT_PLUGINS_ENGINE_H__ */


