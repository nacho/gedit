/*
 * gedit-sample-plugin.h
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
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include "gedit-sample-plugin.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <gtk/gtk.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>

#define WINDOW_DATA_KEY "GeditSamplePluginWindowData"
#define MENU_PATH "/MenuBar/EditMenu/EditOps_4"

#define GEDIT_SAMPLE_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SAMPLE_PLUGIN, GeditSamplePluginPrivate))

struct _GeditSamplePluginPrivate
{
	gpointer dummy;
};

GEDIT_PLUGIN_REGISTER_TYPE(GeditSamplePlugin, gedit_sample_plugin)

typedef struct
{
	GtkActionGroup *action_group;
	guint           ui_id;
} WindowData;

static void sample_cb (GtkAction *action, GeditWindow *window);

static const GtkActionEntry action_entries[] =
{
	{ "UserName",
	  NULL,
	  N_("Insert User Na_me"),
	  NULL,
	  N_("Insert the user name at the cursor position"),
	  G_CALLBACK (sample_cb) },
};

static void
gedit_sample_plugin_init (GeditSamplePlugin *plugin)
{
	plugin->priv = GEDIT_SAMPLE_PLUGIN_GET_PRIVATE (plugin);

	gedit_debug_message (DEBUG_PLUGINS, "GeditSamplePlugin initialising");
}

static void
gedit_sample_plugin_finalize (GObject *object)
{
/*
	GeditSamplePlugin *plugin = GEDIT_SAMPLE_PLUGIN (object);
*/
	gedit_debug_message (DEBUG_PLUGINS, "GeditSamplePlugin finalising");

	G_OBJECT_CLASS (gedit_sample_plugin_parent_class)->finalize (object);
}

static void 
sample_cb (GtkAction   *action,
	   GeditWindow *window)
{
	GeditDocument *doc;
	gchar *user_name;
	gchar *user_name_utf8;
	const gchar *temp;

	gedit_debug (DEBUG_PLUGINS);

	doc = gedit_window_get_active_document (window);
	g_return_if_fail (doc != NULL);

	temp = g_get_real_name ();
	g_return_if_fail (temp != NULL);

	if (strlen (temp) <= 0)
	{
		temp = g_get_user_name ();
		g_return_if_fail (temp != NULL);
	}

	user_name = g_strdup_printf ("%s ", temp);
	g_return_if_fail (user_name != NULL);

	if (g_utf8_validate (user_name, -1, NULL))
		user_name_utf8 = user_name;
	else
	{
		user_name_utf8 = g_locale_to_utf8 (user_name,
						   -1,
						   NULL,
						   NULL,
						   NULL);
		g_free (user_name);

		if (user_name_utf8 == NULL)
			user_name_utf8 = g_strdup (" ");
	}

	gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (doc));

	gtk_text_buffer_insert_at_cursor (GTK_TEXT_BUFFER (doc),
					  user_name_utf8,
					  -1);

	gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (doc));

	g_free (user_name_utf8);
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->action_group);
	g_free (data);
}

static void
update_ui_real (GeditWindow  *window,
		WindowData   *data)
{
	GeditView *view;
	GtkAction *action;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (window);

	gedit_debug_message (DEBUG_PLUGINS, "View: %p", view);
	
	action = gtk_action_group_get_action (data->action_group,
					      "UserName");
	gtk_action_set_sensitive (action,
				  (view != NULL) &&
				  gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = g_new (WindowData, 1);

	manager = gedit_window_get_ui_manager (window);

	data->action_group = gtk_action_group_new ("GeditSamplePluginActions");
	gtk_action_group_set_translation_domain (data->action_group, 
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (data->action_group, 
				      action_entries,
				      G_N_ELEMENTS (action_entries), 
				      window);

	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);

	data->ui_id = gtk_ui_manager_new_merge_id (manager);

	g_object_set_data_full (G_OBJECT (window), 
				WINDOW_DATA_KEY, 
				data,
				(GDestroyNotify) free_window_data);

	gtk_ui_manager_add_ui (manager, 
			       data->ui_id, 
			       MENU_PATH,
			       "UserName", 
			       "UserName",
			       GTK_UI_MANAGER_MENUITEM, 
			       TRUE);

	update_ui_real (window, data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);	
}
		 
static void
impl_update_ui	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	WindowData   *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	update_ui_real (window, data);
}

static void
gedit_sample_plugin_class_init (GeditSamplePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_sample_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
	
	g_type_class_add_private (object_class, sizeof (GeditSamplePluginPrivate));
}
