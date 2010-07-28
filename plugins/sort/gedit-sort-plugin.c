/*
 * gedit-sort-plugin.c
 * 
 * Original author: Carlo Borreo <borreo@softhome.net>
 * Ported to Gedit2 by Lee Mallabone <gnome@fonicmonkey.net>
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
#include <config.h>
#endif

#include "gedit-sort-plugin.h"

#include <string.h>
#include <glib/gi18n.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>

#define GEDIT_SORT_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SORT_PLUGIN, GeditSortPluginPrivate))

#define MENU_PATH "/MenuBar/EditMenu/EditOps_6"

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditSortPlugin,
				gedit_sort_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init))

struct _GeditSortPluginPrivate
{
	GeditWindow *window;

	GtkActionGroup *ui_action_group;
	guint ui_id;

	GtkWidget *dialog;
	GtkWidget *col_num_spinbutton;
	GtkWidget *reverse_order_checkbutton;
	GtkWidget *ignore_case_checkbutton;
	GtkWidget *remove_dups_checkbutton;

	GtkTextIter start, end; /* selection */
};

typedef struct
{
	gint starting_column;

	guint ignore_case : 1;
	guint reverse_order : 1;
	guint remove_duplicates : 1;
} SortInfo;

enum
{
	PROP_0,
	PROP_WINDOW
};

static void sort_cb (GtkAction *action, GeditSortPlugin *plugin);
static void sort_real (GeditSortPlugin *plugin);

static const GtkActionEntry action_entries[] =
{
	{ "Sort",
	  GTK_STOCK_SORT_ASCENDING,
	  N_("S_ort..."),
	  NULL,
	  N_("Sort the current document or selection"),
	  G_CALLBACK (sort_cb) }
};

static void
sort_dialog_response_handler (GtkDialog       *dlg,
			      gint             res_id,
			      GeditSortPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS);

	switch (res_id)
	{
		case GTK_RESPONSE_OK:
			sort_real (plugin);
			gtk_widget_destroy (GTK_WIDGET (dlg));
			break;

		case GTK_RESPONSE_HELP:
			gedit_app_show_help (gedit_app_get_default (),
					     GTK_WINDOW (dlg),
					     NULL,
					     "gedit-sort-plugin");
			break;

		case GTK_RESPONSE_CANCEL:
			gtk_widget_destroy (GTK_WIDGET (dlg));
			break;
	}
}

/* NOTE: we store the current selection in the dialog since focusing
 * the text field (like the combo box) looses the documnent selection.
 * Storing the selection ONLY works because the dialog is modal */
static void
get_current_selection (GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GeditDocument *doc;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	doc = gedit_window_get_active_document (priv->window);

	if (!gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						   &priv->start,
						   &priv->end))
	{
		/* No selection, get the whole document. */
		gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
					    &priv->start,
					    &priv->end);
	}
}

static void
create_sort_dialog (GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GtkWidget *error_widget;
	gboolean ret;
	gchar *data_dir;
	gchar *ui_file;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (plugin));
	ui_file = g_build_filename (data_dir, "gedit-sort-plugin.ui", NULL);

	ret = gedit_utils_get_ui_objects (ui_file,
					  NULL,
					  &error_widget,
					  "sort_dialog", &priv->dialog,
					  "reverse_order_checkbutton", &priv->reverse_order_checkbutton,
					  "col_num_spinbutton", &priv->col_num_spinbutton,
					  "ignore_case_checkbutton", &priv->ignore_case_checkbutton,
					  "remove_dups_checkbutton", &priv->remove_dups_checkbutton,
					  NULL);
	g_free (data_dir);
	g_free (ui_file);

	if (!ret)
	{
		const gchar *err_message;

		err_message = gtk_label_get_label (GTK_LABEL (error_widget));
		gedit_warning (GTK_WINDOW (priv->window),
			       "%s", err_message);

		gtk_widget_destroy (error_widget);

		return;
	}

	gtk_dialog_set_default_response (GTK_DIALOG (priv->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (priv->dialog,
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &priv->dialog);

	g_signal_connect (priv->dialog,
			  "response",
			  G_CALLBACK (sort_dialog_response_handler),
			  plugin);

	get_current_selection (plugin);
}

static void
sort_cb (GtkAction       *action,
	 GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GtkWindowGroup *wg;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	create_sort_dialog (plugin);

	wg = gedit_window_get_group (priv->window);
	gtk_window_group_add_window (wg,
				     GTK_WINDOW (priv->dialog));

	gtk_window_set_transient_for (GTK_WINDOW (priv->dialog),
				      GTK_WINDOW (priv->window));
				      
	gtk_window_set_modal (GTK_WINDOW (priv->dialog),
			      TRUE);

	gtk_widget_show (GTK_WIDGET (priv->dialog));
}

/* Compares two strings for the sorting algorithm. Uses the UTF-8 processing
 * functions in GLib to be as correct as possible.*/
static gint
compare_algorithm (gconstpointer s1,
		   gconstpointer s2,
		   gpointer      data)
{
	gint length1, length2;
	gint ret;
	gchar *string1, *string2;
	gchar *substring1, *substring2;
	gchar *key1, *key2;
	SortInfo *sort_info;

	gedit_debug (DEBUG_PLUGINS);

	sort_info = (SortInfo *) data;
	g_return_val_if_fail (sort_info != NULL, -1);

	if (!sort_info->ignore_case)
	{
		string1 = *((gchar **) s1);
		string2 = *((gchar **) s2);
	}
	else
	{
		string1 = g_utf8_casefold (*((gchar **) s1), -1);
		string2 = g_utf8_casefold (*((gchar **) s2), -1);
	}

	length1 = g_utf8_strlen (string1, -1);
	length2 = g_utf8_strlen (string2, -1);

	if ((length1 < sort_info->starting_column) &&
	    (length2 < sort_info->starting_column))
	{
		ret = 0;
	}
	else if (length1 < sort_info->starting_column)
	{
		ret = -1;
	}
	else if (length2 < sort_info->starting_column)
	{
		ret = 1;
	}
	else if (sort_info->starting_column < 1)
	{
		key1 = g_utf8_collate_key (string1, -1);
		key2 = g_utf8_collate_key (string2, -1);
		ret = strcmp (key1, key2);

		g_free (key1);
		g_free (key2);
	}
	else
	{
		/* A character column offset is required, so figure out
		 * the correct offset into the UTF-8 string. */
		substring1 = g_utf8_offset_to_pointer (string1, sort_info->starting_column);
		substring2 = g_utf8_offset_to_pointer (string2, sort_info->starting_column);

		key1 = g_utf8_collate_key (substring1, -1);
		key2 = g_utf8_collate_key (substring2, -1);
		ret = strcmp (key1, key2);

		g_free (key1);
		g_free (key2);
	}

	/* Do the necessary cleanup. */
	if (sort_info->ignore_case)
	{
		g_free (string1);
		g_free (string2);
	}

	if (sort_info->reverse_order)
	{
		ret = -1 * ret;
	}

	return ret;
}

static gchar *
get_line_slice (GtkTextBuffer *buf,
		gint           line)
{
	GtkTextIter start, end;
	char *ret;

	gtk_text_buffer_get_iter_at_line (buf, &start, line);
	end = start;

	if (!gtk_text_iter_ends_line (&start))
		gtk_text_iter_forward_to_line_end (&end);

	ret= gtk_text_buffer_get_slice (buf,
					&start,
					&end,
					TRUE);

	g_assert (ret != NULL);

	return ret;
}

static void
sort_real (GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GeditDocument *doc;
	GtkTextIter start, end;
	gint start_line, end_line;
	gint i;
	gint num_lines;
	gchar **lines;
	SortInfo *sort_info;
	gchar *last_row = NULL;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	doc = gedit_window_get_active_document (priv->window);
	g_return_if_fail (doc != NULL);

	sort_info = g_slice_new (SortInfo);
	sort_info->ignore_case = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ignore_case_checkbutton));
	sort_info->reverse_order = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->reverse_order_checkbutton));
	sort_info->remove_duplicates = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->remove_dups_checkbutton));
	sort_info->starting_column = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->col_num_spinbutton)) - 1;

	start = priv->start;
	end = priv->end;
	start_line = gtk_text_iter_get_line (&start);
	end_line = gtk_text_iter_get_line (&end);

	/* if we are at line start our last line is the previus one.
	 * Otherwise the last line is the current one but we try to
	 * move the iter after the line terminator */
	if (gtk_text_iter_get_line_offset (&end) == 0)
	{
		end_line = MAX (start_line, end_line - 1);
	}
	else
	{
		gtk_text_iter_forward_line (&end);
	}

	num_lines = end_line - start_line + 1;
	lines = g_new0 (gchar *, num_lines + 1);

	gedit_debug_message (DEBUG_PLUGINS, "Building list...");

	for (i = 0; i < num_lines; i++)
	{
		lines[i] = get_line_slice (GTK_TEXT_BUFFER (doc), start_line + i);
	}

	lines[num_lines] = NULL;

	gedit_debug_message (DEBUG_PLUGINS, "Sort list...");

	g_qsort_with_data (lines,
			   num_lines,
			   sizeof (gpointer),
			   compare_algorithm,
			   sort_info);

	gedit_debug_message (DEBUG_PLUGINS, "Rebuilding document...");

	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (doc));

	gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc),
				&start,
				&end);

	for (i = 0; i < num_lines; i++)
	{
		if (sort_info->remove_duplicates &&
		    last_row != NULL &&
		    (strcmp (last_row, lines[i]) == 0))
			continue;

		gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc),
					&start,
					lines[i],
					-1);
		gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc),
					&start,
					"\n",
					-1);

		last_row = lines[i];
	}

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (doc));

	g_strfreev (lines);
	g_slice_free (SortInfo, sort_info);

	gedit_debug_message (DEBUG_PLUGINS, "Done.");
}

static void
update_ui (GeditSortPlugin *plugin)
{
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (plugin->priv->window);

	gtk_action_group_set_sensitive (plugin->priv->ui_action_group,
					(view != NULL) &&
					gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
gedit_sort_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditSortPluginPrivate *priv;
	GtkUIManager *manager;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_SORT_PLUGIN (activatable)->priv;

	manager = gedit_window_get_ui_manager (priv->window);

	priv->ui_action_group = gtk_action_group_new ("GeditSortPluginActions");
	gtk_action_group_set_translation_domain (priv->ui_action_group,
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->ui_action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      activatable);

	gtk_ui_manager_insert_action_group (manager,
					    priv->ui_action_group,
					    -1);

	priv->ui_id = gtk_ui_manager_new_merge_id (manager);

	gtk_ui_manager_add_ui (manager,
			       priv->ui_id,
			       MENU_PATH,
			       "Sort",
			       "Sort",
			       GTK_UI_MANAGER_MENUITEM,
			       FALSE);

	update_ui (GEDIT_SORT_PLUGIN (activatable));
}

static void
gedit_sort_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditSortPluginPrivate *priv;
	GtkUIManager *manager;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_SORT_PLUGIN (activatable)->priv;

	manager = gedit_window_get_ui_manager (priv->window);

	gtk_ui_manager_remove_ui (manager,
				  priv->ui_id);
	gtk_ui_manager_remove_action_group (manager,
					    priv->ui_action_group);
}

static void
gedit_sort_plugin_update_state (GeditWindowActivatable *activatable)
{
	gedit_debug (DEBUG_PLUGINS);

	update_ui (GEDIT_SORT_PLUGIN (activatable));
}

static void
gedit_sort_plugin_init (GeditSortPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin initializing");

	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE (plugin,
						    GEDIT_TYPE_SORT_PLUGIN,
						    GeditSortPluginPrivate);
}

static void
gedit_sort_plugin_dispose (GObject *object)
{
	GeditSortPlugin *plugin = GEDIT_SORT_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin disposing");

	if (plugin->priv->ui_action_group)
	{
		g_object_unref (plugin->priv->ui_action_group);
		plugin->priv->ui_action_group = NULL;
	}

	if (plugin->priv->window != NULL)
	{
		g_object_unref (plugin->priv->window);
		plugin->priv->window = NULL;
	}

	G_OBJECT_CLASS (gedit_sort_plugin_parent_class)->dispose (object);
}


static void
gedit_sort_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin finalizing");

	G_OBJECT_CLASS (gedit_sort_plugin_parent_class)->finalize (object);
}

static void
gedit_sort_plugin_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	GeditSortPlugin *plugin = GEDIT_SORT_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_sort_plugin_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	GeditSortPlugin *plugin = GEDIT_SORT_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_sort_plugin_class_init (GeditSortPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_sort_plugin_dispose;
	object_class->finalize = gedit_sort_plugin_finalize;
	object_class->set_property = gedit_sort_plugin_set_property;
	object_class->get_property = gedit_sort_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");

	g_type_class_add_private (klass, sizeof (GeditSortPluginPrivate));
}

static void
gedit_sort_plugin_class_finalize (GeditSortPluginClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_sort_plugin_activate;
	iface->deactivate = gedit_sort_plugin_deactivate;
	iface->update_state = gedit_sort_plugin_update_state;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_sort_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_SORT_PLUGIN);
}

/* ex:ts=8:noet: */
