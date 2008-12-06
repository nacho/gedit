/*
 * gedit-plugin.h
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi 
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

#include "gedit-plugin.h"

/* properties */
enum {
	PROP_0,
	PROP_INSTALL_PATH
};

typedef struct _GeditPluginPrivate GeditPluginPrivate;

struct _GeditPluginPrivate
{
	gchar *install_path;
};

#define GEDIT_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PLUGIN, GeditPluginPrivate))

G_DEFINE_TYPE(GeditPlugin, gedit_plugin, G_TYPE_OBJECT)

static void
dummy (GeditPlugin *plugin, GeditWindow *window)
{
	/* Empty */
}

static GtkWidget *
create_configure_dialog	(GeditPlugin *plugin)
{
	return NULL;
}

static gboolean
is_configurable (GeditPlugin *plugin)
{
	return (GEDIT_PLUGIN_GET_CLASS (plugin)->create_configure_dialog !=
		create_configure_dialog);
}

static void
gedit_plugin_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GeditPluginPrivate *priv = GEDIT_PLUGIN_GET_PRIVATE (object);

	switch (prop_id)
	{
		case PROP_INSTALL_PATH:
			g_value_set_string (value, priv->install_path);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
gedit_plugin_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GeditPluginPrivate *priv = GEDIT_PLUGIN_GET_PRIVATE (object);

	switch (prop_id)
	{
		case PROP_INSTALL_PATH:
			priv->install_path = g_value_dup_string (value);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
gedit_plugin_finalize (GObject *object)
{
	GeditPluginPrivate *priv = GEDIT_PLUGIN_GET_PRIVATE (object);
	
	g_free (priv->install_path);
}

static void 
gedit_plugin_class_init (GeditPluginClass *klass)
{
    	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	klass->activate = dummy;
	klass->deactivate = dummy;
	klass->update_ui = dummy;
	
	klass->create_configure_dialog = create_configure_dialog;
	klass->is_configurable = is_configurable;

	object_class->get_property = gedit_plugin_get_property;
	object_class->set_property = gedit_plugin_set_property;
	object_class->finalize = gedit_plugin_finalize;

	g_object_class_install_property (object_class,
					 PROP_INSTALL_PATH,
					 g_param_spec_string ("install-path",
							      "Install Path",
							      "The path where the plugin is installed",
							      NULL,
							      G_PARAM_READWRITE | 
								  G_PARAM_CONSTRUCT_ONLY));
	
	g_type_class_add_private (klass, sizeof (GeditPluginPrivate));
}

static void
gedit_plugin_init (GeditPlugin *plugin)
{
	/* Empty */
}

/**
 * gedit_plugin_get_install_path:
 * @plugin: a #GeditPlugin
 *
 * Returns the path where the plugin is installed
 */
const gchar *
gedit_plugin_get_install_path (GeditPlugin *plugin)
{
	g_return_val_if_fail (GEDIT_IS_PLUGIN (plugin), NULL);
	
	return GEDIT_PLUGIN_GET_PRIVATE (plugin)->install_path;
}

/**
 * gedit_plugin_activate:
 * @plugin: a #GeditPlugin
 * @window: a #GeditWindow
 * 
 * Activates the plugin.
 */
void
gedit_plugin_activate (GeditPlugin *plugin,
		       GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	
	GEDIT_PLUGIN_GET_CLASS (plugin)->activate (plugin, window);
}

/**
 * gedit_plugin_deactivate:
 * @plugin: a #GeditPlugin
 * @window: a #GeditWindow
 * 
 * Deactivates the plugin.
 */
void
gedit_plugin_deactivate	(GeditPlugin *plugin,
			 GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	GEDIT_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, window);
}

/**
 * gedit_plugin_update_ui:
 * @plugin: a #GeditPlugin
 * @window: a #GeditWindow
 *
 * Triggers an update of the user interface to take into account state changes
 * caused by the plugin.
 */		 
void
gedit_plugin_update_ui	(GeditPlugin *plugin,
			 GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	GEDIT_PLUGIN_GET_CLASS (plugin)->update_ui (plugin, window);
}

/**
 * gedit_plugin_is_configurable:
 * @plugin: a #GeditPlugin
 *
 * Whether the plugin is configurable.
 *
 * Returns: TRUE if the plugin is configurable:
 */
gboolean
gedit_plugin_is_configurable (GeditPlugin *plugin)
{
	g_return_val_if_fail (GEDIT_IS_PLUGIN (plugin), FALSE);

	return GEDIT_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

/**
 * gedit_plugin_create_configure_dialog:
 * @plugin: a #GeditPlugin
 *
 * Creates the configure dialog widget for the plugin.
 *
 * Returns: the configure dialog widget for the plugin.
 */
GtkWidget *
gedit_plugin_create_configure_dialog (GeditPlugin *plugin)
{
	g_return_val_if_fail (GEDIT_IS_PLUGIN (plugin), NULL);
	
	return GEDIT_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
}
