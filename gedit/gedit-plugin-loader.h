/*
 * gedit-plugin-loader.h
 * This file is part of gedit
 *
 * Copyright (C) 2008 - Jesse van den Kieboom
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

#ifndef __GEDIT_PLUGIN_LOADER_H__
#define __GEDIT_PLUGIN_LOADER_H__

#include <glib-object.h>
#include <gedit/gedit-plugin.h>
#include <gedit/gedit-plugin-info.h>

#define GEDIT_TYPE_PLUGIN_LOADER                (gedit_plugin_loader_get_type ())
#define GEDIT_PLUGIN_LOADER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PLUGIN_LOADER, GeditPluginLoader))
#define GEDIT_IS_PLUGIN_LOADER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_PLUGIN_LOADER))
#define GEDIT_PLUGIN_LOADER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GEDIT_TYPE_PLUGIN_LOADER, GeditPluginLoaderInterface))

typedef struct _GeditPluginLoader GeditPluginLoader; /* dummy object */
typedef struct _GeditPluginLoaderInterface GeditPluginLoaderInterface;

struct _GeditPluginLoaderInterface {
	GTypeInterface parent;

	const gchar *(*get_name)		(void);

	GeditPlugin *(*load) 		(GeditPluginLoader 	*loader,
			     		 GeditPluginInfo	*info,
			      		 const gchar       	*path);

	void 	     (*unload)		(GeditPluginLoader 	*loader,
					 GeditPluginInfo       	*info);

	void         (*garbage_collect) 	(GeditPluginLoader	*loader);
};

GType gedit_plugin_loader_get_type (void);

const gchar *gedit_plugin_loader_type_get_name	(GType 			 type);
GeditPlugin *gedit_plugin_loader_load		(GeditPluginLoader 	*loader,
						 GeditPluginInfo 	*info,
						 const gchar		*path);
void gedit_plugin_loader_unload			(GeditPluginLoader 	*loader,
						 GeditPluginInfo	*info);

void gedit_plugin_loader_garbage_collect	(GeditPluginLoader 	*loader);

/**
 * GEDIT_PLUGIN_LOADER_REGISTER_TYPE_WITH_CODE(PluginLoaderName, plugin_loader_name, CODE):
 *
 * Utility macro used to register plugins with additional code.
 */
#define GEDIT_PLUGIN_LOADER_REGISTER_TYPE_WITH_CODE(PluginLoaderName, plugin_loader_name, CODE) \
	GEDIT_OBJECT_MODULE_REGISTER_TYPE_WITH_CODE (register_gedit_plugin_loader,	\
						     PluginLoaderName,			\
						     plugin_loader_name,		\
						     G_TYPE_OBJECT,			\
						     GEDIT_OBJECT_MODULE_IMPLEMENT_INTERFACE (cplugin_loader_iface, GEDIT_TYPE_PLUGIN_LOADER, gedit_plugin_loader_iface_init);	\
						     CODE)
/**
 * GEDIT_PLUGIN_LOADER_REGISTER_TYPE(PluginName, plugin_name):
 * 
 * Utility macro used to register plugins.
 */
#define GEDIT_PLUGIN_LOADER_REGISTER_TYPE(PluginName, plugin_name)		\
	GEDIT_PLUGIN_LOADER_REGISTER_TYPE_WITH_CODE(PluginName, plugin_name, ;)

/**
 * GEDIT_PLUGIN_LOADER_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE):
 *
 * Utility macro used to register gobject types in plugins with additional code.
 */
#define GEDIT_PLUGIN_LOADER_DEFINE_TYPE_WITH_CODE GEDIT_OBJECT_MODULE_DEFINE_TYPE_WITH_CODE

/**
 * GEDIT_PLUGIN_LOADER_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE):
 *
 * Utility macro used to register gobject types in plugins.
 */
#define GEDIT_PLUGIN_LOADER_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)	\
	GEDIT_PLUGIN_LOADER_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, ;)

/**
 * GEDIT_PLUGIN_LOADER_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init):
 *
 * Utility macro used to register interfaces for gobject types in plugins.
 */
#define GEDIT_PLUGIN_LOADER_IMPLEMENT_INTERFACE(object_name, TYPE_IFACE, iface_init)	\
	GEDIT_OBJECT_MODULE_IMPLEMENT_INTERFACE(object_name, TYPE_IFACE, iface_init)

#endif /* __GEDIT_PLUGIN_LOADER_H__ */
