/*
 * gedit-indent-plugin.h
 * 
 * Copyright (C) 2002-2005 Paolo Maggi 
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

#include "gedit-indent-plugin.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <gtksourceview/gtksourceview.h>
#include <gedit/gedit-debug.h>


#define WINDOW_DATA_KEY "GeditIndentPluginWindowData"
#define MENU_PATH "/MenuBar/EditMenu/EditOps_5"

#define GEDIT_INDENT_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_INDENT_PLUGIN, GeditIndentPluginPrivate))

GEDIT_PLUGIN_REGISTER_TYPE(GeditIndentPlugin, gedit_indent_plugin)

typedef struct
{
	GtkActionGroup *action_group;
	guint           ui_id;
} WindowData;

static void	indent_cb	(GtkAction *action, GeditWindow *window);
static void	unindent_cb	(GtkAction *action, GeditWindow *window);

/* UI actions. */
static const GtkActionEntry action_entries[] =
{
	{ "Indent",
	  GTK_STOCK_INDENT,
	  N_("_Indent"),
	  "<Control>T",
	  N_("Indent selected lines"),
	  G_CALLBACK (indent_cb) },

	{ "Unindent",
	  GTK_STOCK_UNINDENT,
	  N_("U_nindent"),
	  "<Shift><Control>T",
	  N_("Unindent selected lines"),
	  G_CALLBACK (unindent_cb) }
};

static void
gedit_indent_plugin_init (GeditIndentPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditIndentPlugin initializing");
}

static void
gedit_indent_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditIndentPlugin finalizing");

	G_OBJECT_CLASS (gedit_indent_plugin_parent_class)->finalize (object);
}

static void
indent_cb (GtkAction   *action,
	   GeditWindow *window)
{
	GtkSourceView *view;
	GtkTextBuffer *buffer;
	GtkTextIter start, end, iter;
	gint i, start_line, end_line;
	gchar *tab_buffer = NULL;

	gedit_debug (DEBUG_PLUGINS);

	view = GTK_SOURCE_VIEW (gedit_window_get_active_view (window));
	g_return_if_fail (view != NULL);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_text_buffer_begin_user_action (buffer);

	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

	start_line = gtk_text_iter_get_line (&start);
	end_line = gtk_text_iter_get_line (&end);

	if ((gtk_text_iter_get_visible_line_offset (&end) == 0) &&
	    (end_line > start_line))
	{
		end_line--;
	}

	if (gtk_source_view_get_insert_spaces_instead_of_tabs (view))
	{
		gint tab_width;

		tab_width = gtk_source_view_get_tab_width (view);
		tab_buffer = g_strnfill (tab_width, ' ');
	}
	else
	{
		tab_buffer = g_strdup ("\t");
	}

	for (i = start_line; i <= end_line; i++)
	{
		gtk_text_buffer_get_iter_at_line (buffer, &iter, i);

		/* don't add indentation on empty lines */
		if (gtk_text_iter_ends_line (&iter)) continue;

		gtk_text_buffer_insert (buffer, &iter, tab_buffer, -1);
	}

	gtk_text_buffer_end_user_action (buffer);
	
	g_free (tab_buffer);
}

static void
unindent_cb (GtkAction   *action,
	     GeditWindow *window)
{
	GtkSourceView *view;
	GtkTextBuffer *buffer;
	GtkTextIter start, end, iter, iter2;
	gint i, start_line, end_line;

	gedit_debug (DEBUG_PLUGINS);

	view = GTK_SOURCE_VIEW (gedit_window_get_active_view (window));
	g_return_if_fail (view != NULL);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_text_buffer_begin_user_action (buffer);

	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

	start_line = gtk_text_iter_get_line (&start);
	end_line = gtk_text_iter_get_line (&end);

	if ((gtk_text_iter_get_visible_line_offset (&end) == 0) && (end_line > start_line))
		end_line--;

	for (i = start_line; i <= end_line; i++)
	{
		gtk_text_buffer_get_iter_at_line (buffer, &iter, i);

		if (gtk_text_iter_get_char (&iter) == '\t')
		{
			iter2 = iter;
			gtk_text_iter_forward_char (&iter2);
			gtk_text_buffer_delete (buffer, &iter, &iter2);
		}
		else if (gtk_text_iter_get_char (&iter) == ' ')
		{
			gint spaces = 0;

			iter2 = iter;

			while (!gtk_text_iter_ends_line (&iter2))
			{
				if (gtk_text_iter_get_char (&iter2) == ' ')
					spaces++;
				else
					break;

				gtk_text_iter_forward_char (&iter2);
			}

			if (spaces > 0)
			{
				gint tabs = 0;
				gint tab_width = gtk_source_view_get_tab_width (view);

				tabs = spaces / tab_width;
				spaces = spaces - (tabs * tab_width);

				if (spaces == 0)
					spaces = tab_width;

				iter2 = iter;

				gtk_text_iter_forward_chars (&iter2, spaces);
				gtk_text_buffer_delete (buffer, &iter, &iter2);
			}
		}
	}

	gtk_text_buffer_end_user_action (buffer);
}
	
static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->action_group);
	g_free (data);
}

static void
update_ui_real (GeditWindow *window,
		WindowData *data)
{
	GeditView *view;
	
	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (window);

	gtk_action_group_set_sensitive (data->action_group,
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

	data->action_group = gtk_action_group_new ("GeditIndentPluginActions");
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
			       "Indent", 
			       "Indent",
			       GTK_UI_MANAGER_MENUITEM, 
			       FALSE);

	gtk_ui_manager_add_ui (manager, 
			       data->ui_id, 
			       MENU_PATH,
			       "Unindent", 
			       "Unindent",
			       GTK_UI_MANAGER_MENUITEM, 
			       FALSE);

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
impl_update_ui (GeditPlugin *plugin,
		GeditWindow *window)
{
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	update_ui_real (window, data);
}

static void
gedit_indent_plugin_class_init (GeditIndentPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_indent_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}
