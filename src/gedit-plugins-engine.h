/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-plugins-engine.h
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

#ifndef __GEDIT_PLUGINS_ENGINE_H__
#define __GEDIT_PLUGINS_ENGINE_H__

#include "gedit-plugin.h"

typedef struct _GeditPluginInfo GeditPluginInfo;

typedef enum {
	GEDIT_PLUGIN_ACTIVATED,
	GEDIT_PLUGIN_DEACTIVATED
} GeditPluginActivationState;

struct _GeditPluginInfo
{
	GeditPlugin		 	*plugin;
	GeditPluginActivationState 	 state;
};

gboolean	 gedit_plugins_engine_init 		();
void		 gedit_plugins_engine_save_settings 	();

const GList	*gedit_plugins_engine_get_plugins_list 	();

gboolean 	 gedit_plugins_engine_activate_plugin 	(GeditPlugin *plugin);
gboolean 	 gedit_plugins_engine_deactivate_plugin	(GeditPlugin *plugin);

/* 
 * new_window == TRUE if this function is called because a new top window
 * has been created
 */
void		 gedit_plugins_engine_update_plugins_ui	(BonoboWindow *window, gboolean new_window);

gboolean	gedit_plugins_engine_is_a_configurable_plugin (GeditPlugin *plugin);

#endif  /* __GEDIT_PLUGINS_ENGINE_H__ */


