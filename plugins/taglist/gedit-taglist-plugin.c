/*
 * gedit-taglist-plugin.h
 * 
 * Copyright (C) 2002-2005 - Paolo Maggi
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

#include "gedit-taglist-plugin.h"
#include "gedit-taglist-plugin-panel.h"
#include "gedit-taglist-plugin-parser.h"

#include <glib/gi18n.h>

#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-debug.h>

#define GEDIT_TAGLIST_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TAGLIST_PLUGIN, GeditTaglistPluginPrivate))

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditTaglistPlugin,
				gedit_taglist_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)	\
													\
				_gedit_taglist_plugin_panel_register_type (type_module);		\
)

struct _GeditTaglistPluginPrivate
{
	GtkWidget *taglist_panel;
};

static void
gedit_taglist_plugin_init (GeditTaglistPlugin *plugin)
{
	plugin->priv = GEDIT_TAGLIST_PLUGIN_GET_PRIVATE (plugin);

	gedit_debug_message (DEBUG_PLUGINS, "GeditTaglistPlugin initializing");
}

static void
gedit_taglist_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditTaglistPlugin finalizing");

	free_taglist ();

	G_OBJECT_CLASS (gedit_taglist_plugin_parent_class)->finalize (object);
}

static void
gedit_taglist_plugin_activate (GeditWindowActivatable *activatable,
			       GeditWindow            *window)
{
	GeditTaglistPluginPrivate *priv;
	GeditPanel *side_panel;
	gchar *data_dir;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TAGLIST_PLUGIN (activatable)->priv;

	side_panel = gedit_window_get_side_panel (window);

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (activatable));
	priv->taglist_panel = gedit_taglist_plugin_panel_new (window, data_dir);
	g_free (data_dir);

	gedit_panel_add_item_with_stock_icon (side_panel,
					      priv->taglist_panel,
					      "GeditTagListPanel",
					      _("Tags"),
					      GTK_STOCK_ADD);
}

static void
gedit_taglist_plugin_deactivate (GeditWindowActivatable *activatable,
				 GeditWindow            *window)
{
	GeditTaglistPluginPrivate *priv;
	GeditPanel *side_panel;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TAGLIST_PLUGIN (activatable)->priv;

	side_panel = gedit_window_get_side_panel (window);

	gedit_panel_remove_item (side_panel,
				 priv->taglist_panel);
}

static void
gedit_taglist_plugin_update_state (GeditWindowActivatable *activatable,
				   GeditWindow            *window)
{
	GeditTaglistPluginPrivate *priv;
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TAGLIST_PLUGIN (activatable)->priv;

	view = gedit_window_get_active_view (window);

	gtk_widget_set_sensitive (priv->taglist_panel,
				  (view != NULL) &&
				  gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
gedit_taglist_plugin_class_init (GeditTaglistPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_taglist_plugin_finalize;

	g_type_class_add_private (object_class, sizeof (GeditTaglistPluginPrivate));
}

static void
gedit_taglist_plugin_class_finalize (GeditTaglistPluginClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_taglist_plugin_activate;
	iface->deactivate = gedit_taglist_plugin_deactivate;
	iface->update_state = gedit_taglist_plugin_update_state;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_taglist_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_TAGLIST_PLUGIN);
}

/* ex:ts=8:noet: */
