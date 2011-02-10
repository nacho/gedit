/*
 * gedit-zeitgeist-plugin.h
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Michal Hruby
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_ZEITGEIST_PLUGIN_H__
#define __GEDIT_ZEITGEIST_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_ZEITGEIST_PLUGIN		(gedit_zeitgeist_plugin_get_type ())
#define GEDIT_ZEITGEIST_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_ZEITGEIST_PLUGIN, GeditZeitgeistPlugin))
#define GEDIT_ZEITGEIST_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_ZEITGEIST_PLUGIN, GeditZeitgeistPluginClass))
#define GEDIT_IS_ZEITGEIST_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_ZEITGEIST_PLUGIN))
#define GEDIT_IS_ZEITGEIST_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_ZEITGEIST_PLUGIN))
#define GEDIT_ZEITGEIST_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_ZEITGEIST_PLUGIN, GeditZeitgeistPluginClass))

typedef struct _GeditZeitgeistPlugin		GeditZeitgeistPlugin;
typedef struct _GeditZeitgeistPluginPrivate	GeditZeitgeistPluginPrivate;
typedef struct _GeditZeitgeistPluginClass		GeditZeitgeistPluginClass;

struct _GeditZeitgeistPlugin
{
	PeasExtensionBase parent;

	/*< private >*/
	GeditZeitgeistPluginPrivate *priv;
};

struct _GeditZeitgeistPluginClass
{
	PeasExtensionBaseClass parent_class;
};

GType	gedit_zeitgeist_plugin_get_type		(void) G_GNUC_CONST;

G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __GEDIT_ZEITGEIST_PLUGIN_H__ */

/* ex:ts=8:noet: */
