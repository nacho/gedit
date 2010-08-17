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

G_DEFINE_TYPE(GeditPluginsEngine, gedit_plugins_engine, PEAS_TYPE_ENGINE)

struct _GeditPluginsEnginePrivate
{
	GSettings *plugin_settings;
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
}

static void
gedit_plugins_engine_dispose (GObject *object)
{
	GeditPluginsEngine *engine = GEDIT_PLUGINS_ENGINE (object);

	if (engine->priv->plugin_settings != NULL)
	{
		g_object_unref (engine->priv->plugin_settings);
		engine->priv->plugin_settings = NULL;
	}

	G_OBJECT_CLASS (gedit_plugins_engine_parent_class)->dispose (object);
}

static void
gedit_plugins_engine_class_init (GeditPluginsEngineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_plugins_engine_dispose;

	g_type_class_add_private (klass, sizeof (GeditPluginsEnginePrivate));
}

GeditPluginsEngine *
gedit_plugins_engine_get_default (void)
{
	gchar *typelib_dir;
	const gchar **search_paths;

	if (default_engine != NULL)
	{
		return default_engine;
	}

	/* Require gedit's typelib. */
	typelib_dir = g_build_filename (gedit_dirs_get_gedit_lib_dir (),
					"girepository-1.0",
					NULL);
	g_irepository_require_private (g_irepository_get_default (),
				       typelib_dir, "Gedit", "3.0", 0, NULL);
	g_free (typelib_dir);

	/* This should be moved to libpeas */
	g_irepository_require (g_irepository_get_default (),
			       "Peas", "1.0", 0, NULL);
	g_irepository_require (g_irepository_get_default (),
			       "PeasUI", "1.0", 0, NULL);

	search_paths = g_new (const gchar *, 5);
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
							     "search-paths", search_paths,
							     NULL));

	g_free (search_paths);

	g_object_add_weak_pointer (G_OBJECT (default_engine),
				   (gpointer) &default_engine);

	g_settings_bind (default_engine->priv->plugin_settings,
	                 GEDIT_SETTINGS_ACTIVE_PLUGINS,
	                 default_engine,
	                 "loaded-plugins",
	                 G_SETTINGS_BIND_DEFAULT);

	return default_engine;
}

/* ex:set ts=8 noet: */
