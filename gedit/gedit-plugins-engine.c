/*
 * gedit-plugins-engine.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi 
 * Copyright (C) 2010 Steve Frécinaux
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <girepository.h>

#include "gedit-plugins-engine.h"
#include "gedit-debug.h"
#include "gedit-app.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"
#include "gedit-utils.h"

#define GEDIT_PLUGINS_ENGINE_BASE_KEY "/apps/gedit-2/plugins"
#define GEDIT_PLUGINS_ENGINE_KEY GEDIT_PLUGINS_ENGINE_BASE_KEY "/active-plugins"

G_DEFINE_TYPE(GeditPluginsEngine, gedit_plugins_engine, PEAS_TYPE_ENGINE)

struct _GeditPluginsEnginePrivate
{
	GSettings *plugin_settings;
	gboolean loading_plugin_list : 1;
};

GeditPluginsEngine *default_engine = NULL;

static void
gedit_plugins_engine_init (GeditPluginsEngine *engine)
{
	gedit_debug (DEBUG_PLUGINS);

	engine->priv = G_TYPE_INSTANCE_GET_PRIVATE (engine,
						    GEDIT_TYPE_PLUGINS_ENGINE,
						    GeditPluginsEnginePrivate);

	engine->priv->plugin_settings = g_settings_new ("org.gnome.gedit.plugins");

	engine->priv->loading_plugin_list = FALSE;
}

static void
save_plugin_list (GeditPluginsEngine *engine)
{
	gchar **loaded_plugins;

	loaded_plugins = peas_engine_get_loaded_plugins (PEAS_ENGINE (engine));

	g_settings_set_strv (engine->priv->plugin_settings,
			     GEDIT_SETTINGS_ACTIVE_PLUGINS,
			     (const gchar * const *) loaded_plugins);

	g_strfreev (loaded_plugins);
}

static void
gedit_plugins_engine_load_plugin (PeasEngine     *engine,
				  PeasPluginInfo *info)
{
	GeditPluginsEngine *gengine = GEDIT_PLUGINS_ENGINE (engine);

	PEAS_ENGINE_CLASS (gedit_plugins_engine_parent_class)->load_plugin (engine, info);

	/* We won't save the plugin list if we are currently loading the
	 * plugins from the saved list */
	if (!gengine->priv->loading_plugin_list && peas_plugin_info_is_loaded (info))
	{
		save_plugin_list (gengine);
	}
}

static void
gedit_plugins_engine_unload_plugin (PeasEngine     *engine,
				    PeasPluginInfo *info)
{
	GeditPluginsEngine *gengine = GEDIT_PLUGINS_ENGINE (engine);

	PEAS_ENGINE_CLASS (gedit_plugins_engine_parent_class)->unload_plugin (engine, info);

	/* We won't save the plugin list if we are currently unloading the
	 * plugins from the saved list */
	if (!gengine->priv->loading_plugin_list && !peas_plugin_info_is_loaded (info))
	{
		save_plugin_list (gengine);
	}
}

static void
gedit_plugins_engine_class_init (GeditPluginsEngineClass *klass)
{
	PeasEngineClass *engine_class = PEAS_ENGINE_CLASS (klass);

	engine_class->load_plugin = gedit_plugins_engine_load_plugin;
	engine_class->unload_plugin = gedit_plugins_engine_unload_plugin;

	g_type_class_add_private (klass, sizeof (GeditPluginsEnginePrivate));
}

GeditPluginsEngine *
gedit_plugins_engine_get_default (void)
{
	gchar *modules_dir;
	gchar **search_paths;

	if (default_engine != NULL)
	{
		return default_engine;
	}

	/* This should be moved to libpeas */
	g_irepository_require (g_irepository_get_default (),
			       "Peas", "1.0", 0, NULL);
	g_irepository_require (g_irepository_get_default (),
			       "PeasUI", "1.0", 0, NULL);

	modules_dir = gedit_dirs_get_binding_modules_dir ();
	search_paths = g_new (gchar *, 5);
	/* Add the user plugins dir in ~ */
	search_paths[0] = gedit_dirs_get_user_plugins_dir ();
	search_paths[1] = gedit_dirs_get_user_plugins_dir ();
	/* Add the system plugins dir */
	search_paths[2] = gedit_dirs_get_gedit_plugins_dir ();
	search_paths[3] = gedit_dirs_get_gedit_plugins_data_dir ();
	/* Add the trailing NULL */
	search_paths[4] = NULL;

	default_engine = GEDIT_PLUGINS_ENGINE (g_object_new (GEDIT_TYPE_PLUGINS_ENGINE,
							     "app-name", "Gedit",
							     "base-module-dir", modules_dir,
							     "search-paths", search_paths,
							     NULL));

	g_strfreev (search_paths);
	g_free (modules_dir);

	g_object_add_weak_pointer (G_OBJECT (default_engine),
				   (gpointer) &default_engine);

	gedit_plugins_engine_active_plugins_changed (default_engine);

	return default_engine;
}

void
gedit_plugins_engine_active_plugins_changed (GeditPluginsEngine *engine)
{
	gchar **loaded_plugins;

	loaded_plugins = g_settings_get_strv (engine->priv->plugin_settings,
					      GEDIT_SETTINGS_ACTIVE_PLUGINS);

	engine->priv->loading_plugin_list = TRUE;
	peas_engine_set_loaded_plugins (PEAS_ENGINE (engine),
					(const gchar **) loaded_plugins);
	engine->priv->loading_plugin_list = FALSE;
	g_strfreev (loaded_plugins);
}

/* ex:set ts=8 noet: */
