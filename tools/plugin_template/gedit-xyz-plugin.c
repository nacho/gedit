/*
 * gedit-xyz-plugin.c
 * 
 * Copyright (C) %YEAR% - %AUTHOR%
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-xyz-plugin.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <gedit/gedit-debug.h>

#define GEDIT_XYZ_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_XYZ_PLUGIN, GeditXyzPluginPrivate))

struct _GeditXyzPluginPrivate
{
	gpointer dummy;
};

GEDIT_PLUGIN_REGISTER_TYPE(GeditXyzPlugin, gedit_xyz_plugin)

static void
gedit_xyz_plugin_init (GeditXyzPlugin *plugin)
{
	plugin->priv = GEDIT_XYZ_PLUGIN_GET_PRIVATE (plugin);

	gedit_debug_message (DEBUG_PLUGINS, "GeditXyzPlugin initializing");
}

static void
gedit_xyz_plugin_finalize (GObject *object)
{
/*
	GeditXyzPlugin *plugin = GEDIT_XYZ_PLUGIN (object);
*/
	gedit_debug_message (DEBUG_PLUGINS, "GeditXyzPlugin finalizing");

	G_OBJECT_CLASS (gedit_xyz_plugin_parent_class)->finalize (object);
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	gedit_debug (DEBUG_PLUGINS);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	gedit_debug (DEBUG_PLUGINS);
}

static void
impl_update_ui	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	gedit_debug (DEBUG_PLUGINS);
}

/*
static GtkWidget *
impl_create_configure_dialog (GeditPlugin *plugin)
{
	* Implements this function only and only if the plugin
	* is configurable. Otherwise you can safely remove it. *
}
*/

static void
gedit_xyz_plugin_class_init (GeditXyzPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_xyz_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;

	/* Only if the plugin is configurable */
	/* plugin_class->create_configure_dialog = impl_create_configure_dialog; */

	g_type_class_add_private (object_class, sizeof (GeditXyzPluginPrivate));
}
