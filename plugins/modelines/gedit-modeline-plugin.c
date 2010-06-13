/*
 * gedit-modeline-plugin.c
 * Emacs, Kate and Vim-style modelines support for gedit.
 * 
 * Copyright (C) 2005-2010 - Steve Frécinaux <code@istique.net>
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
 */
 
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <libpeas/peas-activatable.h>
#include "gedit-modeline-plugin.h"
#include "modeline-parser.h"

#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-utils.h>

#define DOCUMENT_DATA_KEY "GeditModelinePluginDocumentData"

typedef struct
{
	gulong document_loaded_handler_id;
	gulong document_saved_handler_id;
} DocumentData;

static void	peas_activatable_iface_init (PeasActivatableInterface *iface);
static void	gedit_modeline_plugin_activate (PeasActivatable *activatable, GObject *object);
static void	gedit_modeline_plugin_deactivate (PeasActivatable *activatable, GObject *object);
static GObject	*gedit_modeline_plugin_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_param);
static void	gedit_modeline_plugin_finalize (GObject *object);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditModelinePlugin,
				gedit_modeline_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_TYPE_ACTIVATABLE,
							       peas_activatable_iface_init))

static void
document_data_free (DocumentData *ddata)
{
	g_slice_free (DocumentData, ddata);
}

static void
gedit_modeline_plugin_class_init (GeditModelinePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->constructor = gedit_modeline_plugin_constructor;
	object_class->finalize = gedit_modeline_plugin_finalize;
}

static void
peas_activatable_iface_init (PeasActivatableInterface *iface)
{
	iface->activate = gedit_modeline_plugin_activate;
	iface->deactivate = gedit_modeline_plugin_deactivate;
}

static void
gedit_modeline_plugin_class_finalize (GeditModelinePluginClass *klass)
{
}

static GObject *
gedit_modeline_plugin_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_param)
{
	GObject *object;
	gchar *data_dir;

	object = G_OBJECT_CLASS (gedit_modeline_plugin_parent_class)->constructor (type,
										   n_construct_properties,
										   construct_param);

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (object));

	modeline_parser_init (data_dir);

	g_free (data_dir);

	return object;
}

static void
gedit_modeline_plugin_init (GeditModelinePlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditModelinePlugin initializing");
}

static void
gedit_modeline_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditModelinePlugin finalizing");

	modeline_parser_shutdown ();

	G_OBJECT_CLASS (gedit_modeline_plugin_parent_class)->finalize (object);
}

static void
on_document_loaded_or_saved (GeditDocument *document,
			     const GError  *error,
			     GtkSourceView *view)
{
	modeline_parser_apply_modeline (view);
}

static void
connect_handlers (GeditView *view)
{
	DocumentData *data;
        GtkTextBuffer *doc;

        doc = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

        data = g_slice_new (DocumentData);

	data->document_loaded_handler_id =
		g_signal_connect (doc, "loaded",
				  G_CALLBACK (on_document_loaded_or_saved),
				  view);
	data->document_saved_handler_id =
		g_signal_connect (doc, "saved",
				  G_CALLBACK (on_document_loaded_or_saved),
				  view);

	g_object_set_data_full (G_OBJECT (doc), DOCUMENT_DATA_KEY,
				data, (GDestroyNotify) document_data_free);
}

static void
disconnect_handlers (GeditView *view)
{
	DocumentData *data;
	GtkTextBuffer *doc;

	doc = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	data = g_object_steal_data (G_OBJECT (doc), DOCUMENT_DATA_KEY);

	if (data)
	{
		g_signal_handler_disconnect (doc, data->document_loaded_handler_id);
		g_signal_handler_disconnect (doc, data->document_saved_handler_id);

		document_data_free (data);
	}
	else
	{
		g_warning ("Modeline handlers not found");
	}
}

static void
on_window_tab_added (GeditWindow *window,
		     GeditTab *tab,
		     gpointer user_data)
{
	connect_handlers (gedit_tab_get_view (tab));
}

static void
on_window_tab_removed (GeditWindow *window,
		       GeditTab *tab,
		       gpointer user_data)
{
	disconnect_handlers (gedit_tab_get_view (tab));
}

static void
gedit_modeline_plugin_activate (PeasActivatable *activatable,
				GObject         *object)
{
	GeditModelinePlugin *plugin = GEDIT_MODELINE_PLUGIN (activatable);
	GeditWindow *window = GEDIT_WINDOW (object);

	GList *views;
	GList *l;

	gedit_debug (DEBUG_PLUGINS);

	views = gedit_window_get_views (window);
	for (l = views; l != NULL; l = l->next)
	{
		connect_handlers (GEDIT_VIEW (l->data));
		modeline_parser_apply_modeline (GTK_SOURCE_VIEW (l->data));
	}
	g_list_free (views);

	plugin->tab_added_handler_id =
		g_signal_connect (window, "tab-added",
				  G_CALLBACK (on_window_tab_added), NULL);

	plugin->tab_removed_handler_id =
		g_signal_connect (window, "tab-removed",
				  G_CALLBACK (on_window_tab_removed), NULL);
}

static void
gedit_modeline_plugin_deactivate (PeasActivatable *activatable,
				  GObject         *object)
{
	GeditModelinePlugin *plugin = GEDIT_MODELINE_PLUGIN (activatable);
	GeditWindow *window = GEDIT_WINDOW (object);
	GList *views;
	GList *l;

	gedit_debug (DEBUG_PLUGINS);

	g_signal_handler_disconnect (window, plugin->tab_added_handler_id);
	g_signal_handler_disconnect (window, plugin->tab_removed_handler_id);

	views = gedit_window_get_views (window);

	for (l = views; l != NULL; l = l->next)
	{
		disconnect_handlers (GEDIT_VIEW (l->data));
		
		modeline_parser_deactivate (GTK_SOURCE_VIEW (l->data));
	}
	
	g_list_free (views);
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_modeline_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    PEAS_TYPE_ACTIVATABLE,
						    GEDIT_TYPE_MODELINE_PLUGIN);
}

/* ex:set ts=8 noet: */
