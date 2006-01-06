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
gedit_plugin_class_init (GeditPluginClass *klass)
{
	klass->activate = dummy;
	klass->deactivate = dummy;
	klass->update_ui = dummy;
	
	klass->create_configure_dialog = create_configure_dialog;
	klass->is_configurable = is_configurable;
}

static void
gedit_plugin_init (GeditPlugin *plugin)
{
	/* Empty */
}

void
gedit_plugin_activate (GeditPlugin *plugin,
		       GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	
	GEDIT_PLUGIN_GET_CLASS (plugin)->activate (plugin, window);
}

void
gedit_plugin_deactivate	(GeditPlugin *plugin,
			 GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	GEDIT_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, window);
}
				 
void
gedit_plugin_update_ui	(GeditPlugin *plugin,
			 GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	GEDIT_PLUGIN_GET_CLASS (plugin)->update_ui (plugin, window);
}

gboolean
gedit_plugin_is_configurable (GeditPlugin *plugin)
{
	g_return_val_if_fail (GEDIT_IS_PLUGIN (plugin), FALSE);

	return GEDIT_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

GtkWidget *
gedit_plugin_create_configure_dialog (GeditPlugin *plugin)
{
	g_return_val_if_fail (GEDIT_IS_PLUGIN (plugin), NULL);
	
	return GEDIT_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
}
