/*
 * gedit-time-plugin.c
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
 */

/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <time.h>

#include "gedit-time-plugin.h"
#include <gedit/gedit-app.h>

#include <glib/gi18n-lib.h>
#include <glib.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <libpeas-gtk/peas-gtk-configurable.h>

#define GEDIT_TIME_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
					      GEDIT_TYPE_TIME_PLUGIN, \
					      GeditTimePluginPrivate))

#define MENU_PATH "/MenuBar/EditMenu/EditOps_4"

/* gsettings keys */
#define TIME_BASE_SETTINGS	"org.gnome.gedit.plugins.time"
#define PROMPT_TYPE_KEY		"prompt-type"
#define SELECTED_FORMAT_KEY	"selected-format"
#define CUSTOM_FORMAT_KEY	"custom-format"

#define DEFAULT_CUSTOM_FORMAT "%d/%m/%Y %H:%M:%S"

static const gchar *formats[] =
{
	"%c",
	"%x",
	"%X",
	"%x %X",
	"%Y-%m-%d %H:%M:%S",
	"%a %b %d %H:%M:%S %Z %Y",
	"%a %b %d %H:%M:%S %Y",
	"%a %d %b %Y %H:%M:%S %Z",
	"%a %d %b %Y %H:%M:%S",
	"%d/%m/%Y",
	"%d/%m/%y",
#ifndef G_OS_WIN32
	"%D", /* This one is not supported on win32 */
#endif
	"%A %d %B %Y",
	"%A %B %d %Y",
	"%Y-%m-%d",
	"%d %B %Y",
	"%B %d, %Y",
	"%A %b %d",
	"%H:%M:%S",
	"%H:%M",
	"%I:%M:%S %p",
	"%I:%M %p",
	"%H.%M.%S",
	"%H.%M",
	"%I.%M.%S %p",
	"%I.%M %p",
	"%d/%m/%Y %H:%M:%S",
	"%d/%m/%y %H:%M:%S",
#if __GLIBC__ >= 2
	"%a, %d %b %Y %H:%M:%S %z",
#endif
	NULL
};

enum
{
	COLUMN_FORMATS = 0,
	COLUMN_INDEX,
	NUM_COLUMNS
};

typedef enum
{
	PROMPT_SELECTED_FORMAT = 0,	/* Popup dialog with list preselected */
	PROMPT_CUSTOM_FORMAT,		/* Popup dialog with entry preselected */
	USE_SELECTED_FORMAT,		/* Use selected format directly */
	USE_CUSTOM_FORMAT		/* Use custom format directly */
} GeditTimePluginPromptType;

typedef struct _TimeConfigureWidget TimeConfigureWidget;

struct _TimeConfigureWidget
{
	GtkWidget *content;

	GtkWidget *list;

	/* Radio buttons to indicate what should be done */
	GtkWidget *prompt;
	GtkWidget *use_list;
	GtkWidget *custom;

	GtkWidget *custom_entry;
	GtkWidget *custom_format_example;

	GSettings *settings;
};

typedef struct _ChooseFormatDialog ChooseFormatDialog;

struct _ChooseFormatDialog
{
	GtkWidget *dialog;

	GtkWidget *list;

	/* Radio buttons to indicate what should be done */
	GtkWidget *use_list;
	GtkWidget *custom;

	GtkWidget *custom_entry;
	GtkWidget *custom_format_example;

	/* Info needed for the response handler */
	GtkTextBuffer   *buffer;

	GSettings *settings;
};

struct _GeditTimePluginPrivate
{
	GSettings      *settings;

	GeditWindow    *window;

	GtkActionGroup *action_group;
	guint           ui_id;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);
static void peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditTimePlugin,
				gedit_time_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_GTK_TYPE_CONFIGURABLE,
							       peas_gtk_configurable_iface_init))

static void time_cb (GtkAction *action, GeditTimePlugin *plugin);

static const GtkActionEntry action_entries[] =
{
	{
		"InsertDateAndTime",
		NULL,
		N_("In_sert Date and Time..."),
		NULL,
		N_("Insert current date and time at the cursor position"),
		G_CALLBACK (time_cb)
	},
};

static void
gedit_time_plugin_init (GeditTimePlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditTimePlugin initializing");

	plugin->priv = GEDIT_TIME_PLUGIN_GET_PRIVATE (plugin);

	plugin->priv->settings = g_settings_new (TIME_BASE_SETTINGS);
}

static void
gedit_time_plugin_dispose (GObject *object)
{
	GeditTimePlugin *plugin = GEDIT_TIME_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditTimePlugin disposing");

	if (plugin->priv->settings != NULL)
	{
		g_object_unref (plugin->priv->settings);
		plugin->priv->settings = NULL;
	}

	if (plugin->priv->action_group != NULL)
	{
		g_object_unref (plugin->priv->action_group);
		plugin->priv->action_group = NULL;
	}

	if (plugin->priv->window != NULL)
	{
		g_object_unref (plugin->priv->window);
		plugin->priv->window = NULL;
	}

	G_OBJECT_CLASS (gedit_time_plugin_parent_class)->dispose (object);
}

static void
gedit_time_plugin_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	GeditTimePlugin *plugin = GEDIT_TIME_PLUGIN (object);

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
gedit_time_plugin_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	GeditTimePlugin *plugin = GEDIT_TIME_PLUGIN (object);

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
update_ui (GeditTimePlugin *plugin)
{
	GeditView *view;
	GtkAction *action;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (plugin->priv->window);

	gedit_debug_message (DEBUG_PLUGINS, "View: %p", view);

	action = gtk_action_group_get_action (plugin->priv->action_group,
					      "InsertDateAndTime");
	gtk_action_set_sensitive (action,
				  (view != NULL) &&
				  gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
gedit_time_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditTimePluginPrivate *priv;
	GtkUIManager *manager;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TIME_PLUGIN (activatable)->priv;

	manager = gedit_window_get_ui_manager (priv->window);

	priv->action_group = gtk_action_group_new ("GeditTimePluginActions");
	gtk_action_group_set_translation_domain (priv->action_group,
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      activatable);

	gtk_ui_manager_insert_action_group (manager, priv->action_group, -1);

	priv->ui_id = gtk_ui_manager_new_merge_id (manager);

	gtk_ui_manager_add_ui (manager,
			       priv->ui_id,
			       MENU_PATH,
			       "InsertDateAndTime",
			       "InsertDateAndTime",
			       GTK_UI_MANAGER_MENUITEM,
			       FALSE);

	update_ui (GEDIT_TIME_PLUGIN (activatable));
}

static void
gedit_time_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditTimePluginPrivate *priv;
	GtkUIManager *manager;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TIME_PLUGIN (activatable)->priv;

	manager = gedit_window_get_ui_manager (priv->window);

	gtk_ui_manager_remove_ui (manager, priv->ui_id);
	gtk_ui_manager_remove_action_group (manager, priv->action_group);
}

static void
gedit_time_plugin_update_state (GeditWindowActivatable *activatable)
{
	gedit_debug (DEBUG_PLUGINS);

	update_ui (GEDIT_TIME_PLUGIN (activatable));
}

/* The selected format in the list */
static gchar *
get_selected_format (GeditTimePlugin *plugin)
{
	gchar *sel_format;

	sel_format = g_settings_get_string (plugin->priv->settings,
					    SELECTED_FORMAT_KEY);

	return sel_format ? sel_format : g_strdup (formats [0]);
}

/* the custom format in the entry */
static gchar *
get_custom_format (GeditTimePlugin *plugin)
{
	gchar *format;

	format = g_settings_get_string (plugin->priv->settings,
					CUSTOM_FORMAT_KEY);

	return format ? format : g_strdup (DEFAULT_CUSTOM_FORMAT);
}

static gchar *
get_time (const gchar* format)
{
	gchar *out = NULL;
	gchar *out_utf8 = NULL;
	time_t clock;
	struct tm *now;
	size_t out_length = 0;
	gchar *locale_format;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (format != NULL, NULL);

	if (strlen (format) == 0)
		return g_strdup (" ");

	locale_format = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
	if (locale_format == NULL)
		return g_strdup (" ");

	clock = time (NULL);
	now = localtime (&clock);

	do
	{
		out_length += 255;
		out = g_realloc (out, out_length);
	}
	while (strftime (out, out_length, locale_format, now) == 0);

	g_free (locale_format);

	if (g_utf8_validate (out, -1, NULL))
	{
		out_utf8 = out;
	}
	else
	{
		out_utf8 = g_locale_to_utf8 (out, -1, NULL, NULL, NULL);
		g_free (out);

		if (out_utf8 == NULL)
			out_utf8 = g_strdup (" ");
	}

	return out_utf8;
}

static void
configure_widget_destroyed (GtkWidget *widget,
			    gpointer   data)
{
	TimeConfigureWidget *widget = (TimeConfigureWidget *)data;

	gedit_debug (DEBUG_PLUGINS);

	g_object_unref (widget->settings);
	g_slice_free (TimeConfigureWidget, data);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static void
choose_format_dialog_destroyed (GtkWidget *widget,
				gpointer   dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS);

	g_slice_free (ChooseFormatDialog, dialog_pointer);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static GtkTreeModel *
create_model (GtkWidget       *listview,
	      const gchar     *sel_format,
	      GeditTimePlugin *plugin)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	/* Set tree view model*/
	gtk_tree_view_set_model (GTK_TREE_VIEW (listview),
				 GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (selection != NULL, GTK_TREE_MODEL (store));

	/* there should always be one line selected */
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	/* add data to the list store */
	while (formats[i] != NULL)
	{
		gchar *str;

		str = get_time (formats[i]);

		gedit_debug_message (DEBUG_PLUGINS, "%d : %s", i, str);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_FORMATS, str,
				    COLUMN_INDEX, i,
				    -1);
		g_free (str);

		if (sel_format && strcmp (formats[i], sel_format) == 0)
			gtk_tree_selection_select_iter (selection, &iter);

		++i;
	}

	/* fall back to select the first iter */
	if (!gtk_tree_selection_get_selected (selection, NULL, NULL))
	{
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
		gtk_tree_selection_select_iter (selection, &iter);
	}

	return GTK_TREE_MODEL (store);
}

static void
scroll_to_selected (GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_tree_view_get_model (tree_view);
	g_return_if_fail (model != NULL);

	/* Scroll to selected */
	selection = gtk_tree_view_get_selection (tree_view);
	g_return_if_fail (selection != NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GtkTreePath* path;

		path = gtk_tree_model_get_path (model, &iter);
		g_return_if_fail (path != NULL);

		gtk_tree_view_scroll_to_cell (tree_view,
					      path, NULL, TRUE, 1.0, 0.0);
		gtk_tree_path_free (path);
	}
}

static void
create_formats_list (GtkWidget       *listview,
		     const gchar     *sel_format,
		     GeditTimePlugin *plugin)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (listview != NULL);
	g_return_if_fail (sel_format != NULL);

	/* the Available formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (
			_("Available formats"),
			cell,
			"text", COLUMN_FORMATS,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (listview), column);

	/* Create model, it also add model to the tree view */
	create_model (listview, sel_format, plugin);

	g_signal_connect (listview,
			  "realize",
			  G_CALLBACK (scroll_to_selected),
			  NULL);

	gtk_widget_show (listview);
}

static void
updated_custom_format_example (GtkEntry *format_entry,
			       GtkLabel *format_example)
{
	const gchar *format;
	gchar *time;
	gchar *str;
	gchar *escaped_time;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (GTK_IS_ENTRY (format_entry));
	g_return_if_fail (GTK_IS_LABEL (format_example));

	format = gtk_entry_get_text (format_entry);

	time = get_time (format);
	escaped_time = g_markup_escape_text (time, -1);

	str = g_strdup_printf ("<span size=\"small\">%s</span>", escaped_time);

	gtk_label_set_markup (format_example, str);

	g_free (escaped_time);
	g_free (time);
	g_free (str);
}

static void
choose_format_dialog_button_toggled (GtkToggleButton    *button,
				     ChooseFormatDialog *dialog)
{
	gedit_debug (DEBUG_PLUGINS);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->custom)))
	{
		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
	{
		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
	}
	else
	{
		g_return_if_reached ();
	}
}

static void
configure_widget_button_toggled (GtkToggleButton     *button,
				 TimeConfigureWidget *conf_widget)
{
	GeditTimePluginPromptType prompt_type;

	gedit_debug (DEBUG_PLUGINS);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (conf_widget->custom)))
	{
		gtk_widget_set_sensitive (conf_widget->list, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_entry, TRUE);
		gtk_widget_set_sensitive (conf_widget->custom_format_example, TRUE);

		prompt_type = USE_CUSTOM_FORMAT;
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (conf_widget->use_list)))
	{
		gtk_widget_set_sensitive (conf_widget->list, TRUE);
		gtk_widget_set_sensitive (conf_widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_format_example, FALSE);

		prompt_type = USE_SELECTED_FORMAT;
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (conf_widget->prompt)))
	{
		gtk_widget_set_sensitive (conf_widget->list, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_format_example, FALSE);

		prompt_type = PROMPT_SELECTED_FORMAT;
	}
	else
	{
		g_return_if_reached ();
	}

	g_settings_set_enum (conf_widget->settings,
			     PROMPT_TYPE_KEY,
			     prompt_type);
}

static gint
get_format_from_list (GtkWidget *listview)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (model != NULL, 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (selection != NULL, 0);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gint selected_value;

		gtk_tree_model_get (model, &iter, COLUMN_INDEX, &selected_value, -1);

		gedit_debug_message (DEBUG_PLUGINS, "Sel value: %d", selected_value);

		return selected_value;
	}

	g_return_val_if_reached (0);
}

static void
on_configure_widget_selection_changed (GtkTreeSelection *selection,
				       TimeConfigureWidget *conf_widget)
{
	gint sel_format;

	sel_format = get_format_from_list (conf_widget->list);
	g_settings_set_string (conf_widget->settings,
			       SELECTED_FORMAT_KEY,
			       formats[sel_format]);
}

static TimeConfigureWidget *
get_configure_widget (GeditTimePlugin *plugin)
{
	TimeConfigureWidget *widget;
	GtkTreeSelection *selection;
	gchar *data_dir;
	gchar *ui_file;
	GtkWidget *viewport;
	GeditTimePluginPromptType prompt_type;
	gchar *sf;
	GtkWidget *error_widget;
	gboolean ret;
	gchar *root_objects[] = {
		"time_dialog_content",
		NULL
	};
	
	gedit_debug (DEBUG_PLUGINS);

	widget = g_slice_new (TimeConfigureWidget);
	widget->settings = g_object_ref (plugin->priv->settings);

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (plugin));
	ui_file = g_build_filename (data_dir, "gedit-time-setup-dialog.ui", NULL);
	ret = gedit_utils_get_ui_objects (ui_file,
					  root_objects,
					  &error_widget,
					  "time_dialog_content", &widget->content,
					  "formats_viewport", &viewport,
					  "formats_tree", &widget->list,
					  "always_prompt", &widget->prompt,
					  "never_prompt", &widget->use_list,
					  "use_custom", &widget->custom,
					  "custom_entry", &widget->custom_entry,
					  "custom_format_example", &widget->custom_format_example,
					  NULL);

	g_free (data_dir);
	g_free (ui_file);

	if (!ret)
	{
		return NULL;
	}

	sf = get_selected_format (plugin);
	create_formats_list (widget->list, sf, plugin);
	g_free (sf);

	prompt_type = g_settings_get_enum (plugin->priv->settings,
					   PROMPT_TYPE_KEY);

	g_settings_bind (widget->settings,
			 CUSTOM_FORMAT_KEY,
			 widget->custom_entry,
			 "text",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	if (prompt_type == USE_CUSTOM_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->custom), TRUE);

		gtk_widget_set_sensitive (widget->list, FALSE);
		gtk_widget_set_sensitive (widget->custom_entry, TRUE);
		gtk_widget_set_sensitive (widget->custom_format_example, TRUE);
	}
	else if (prompt_type == USE_SELECTED_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->use_list), TRUE);

		gtk_widget_set_sensitive (widget->list, TRUE);
		gtk_widget_set_sensitive (widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (widget->custom_format_example, FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->prompt), TRUE);

		gtk_widget_set_sensitive (widget->list, FALSE);
		gtk_widget_set_sensitive (widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (widget->custom_format_example, FALSE);
	}

	updated_custom_format_example (GTK_ENTRY (widget->custom_entry),
				       GTK_LABEL (widget->custom_format_example));

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 10, 200);

	g_signal_connect (widget->custom,
			  "toggled",
			  G_CALLBACK (configure_widget_button_toggled),
			  widget);
	g_signal_connect (widget->prompt,
			  "toggled",
			  G_CALLBACK (configure_widget_button_toggled),
			  widget);
	g_signal_connect (widget->use_list,
			  "toggled",
			  G_CALLBACK (configure_widget_button_toggled),
			  widget);
	g_signal_connect (widget->content,
			  "destroy",
			  G_CALLBACK (configure_widget_destroyed),
			  widget);
	g_signal_connect (widget->custom_entry,
			  "changed",
			  G_CALLBACK (updated_custom_format_example),
			  widget->custom_format_example);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->list));

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (on_configure_widget_selection_changed),
			  widget);

	return widget;
}

static void
real_insert_time (GtkTextBuffer *buffer,
		  const gchar   *the_time)
{
	gedit_debug_message (DEBUG_PLUGINS, "Insert: %s", the_time);

	gtk_text_buffer_begin_user_action (buffer);

	gtk_text_buffer_insert_at_cursor (buffer, the_time, -1);
	gtk_text_buffer_insert_at_cursor (buffer, " ", -1);

	gtk_text_buffer_end_user_action (buffer);
}

static void
choose_format_dialog_row_activated (GtkTreeView        *list,
				    GtkTreePath        *path,
				    GtkTreeViewColumn  *column,
				    ChooseFormatDialog *dialog)
{
	gint sel_format;
	gchar *the_time;

	sel_format = get_format_from_list (dialog->list);
	the_time = get_time (formats[sel_format]);

	g_settings_set_enum (dialog->settings,
			     PROMPT_TYPE_KEY,
			     PROMPT_SELECTED_FORMAT);
	g_settings_set_string (dialog->settings,
			       SELECTED_FORMAT_KEY,
			       formats[sel_format]);

	g_return_if_fail (the_time != NULL);

	real_insert_time (dialog->buffer, the_time);

	g_free (the_time);
}

static ChooseFormatDialog *
get_choose_format_dialog (GtkWindow                 *parent,
			  GeditTimePluginPromptType  prompt_type,
			  GeditTimePlugin           *plugin)
{
	ChooseFormatDialog *dialog;
	gchar *data_dir;
	gchar *ui_file;
	GtkWidget *error_widget;
	gboolean ret;
	gchar *sf, *cf;
	GtkWindowGroup *wg = NULL;
	
	if (parent != NULL)
		wg = gtk_window_get_group (parent);

	dialog = g_slice_new (ChooseFormatDialog);
	dialog->settings = plugin->priv->settings;

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (plugin));
	ui_file = g_build_filename (data_dir, "gedit-time-dialog.ui", NULL);
	ret = gedit_utils_get_ui_objects (ui_file,
					  NULL,
					  &error_widget,
					  "choose_format_dialog", &dialog->dialog,
					  "choice_list", &dialog->list,
					  "use_sel_format_radiobutton", &dialog->use_list,
					  "use_custom_radiobutton", &dialog->custom,
					  "custom_entry", &dialog->custom_entry,
					  "custom_format_example", &dialog->custom_format_example,
					  NULL);

	g_free (data_dir);
	g_free (ui_file);

	if (!ret)
	{
		GtkWidget *err_dialog;

		err_dialog = gtk_dialog_new_with_buttons (NULL,
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			NULL);

		if (wg != NULL)
			gtk_window_group_add_window (wg, GTK_WINDOW (err_dialog));

		gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);
		gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);

		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (err_dialog))),
				   error_widget);

		g_signal_connect (G_OBJECT (err_dialog),
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_widget_show_all (err_dialog);

		return NULL;
	}

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (dialog->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (dialog->dialog), TRUE);

	sf = get_selected_format (plugin);
	create_formats_list (dialog->list, sf, plugin);
	g_free (sf);

	cf = get_custom_format (plugin);
	gtk_entry_set_text (GTK_ENTRY(dialog->custom_entry), cf);
	g_free (cf);

	updated_custom_format_example (GTK_ENTRY (dialog->custom_entry),
				       GTK_LABEL (dialog->custom_format_example));

	if (prompt_type == PROMPT_CUSTOM_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->custom), TRUE);

		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);
	}
	else if (prompt_type == PROMPT_SELECTED_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->use_list), TRUE);

		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
	}
	else
	{
		g_return_val_if_reached (NULL);
	}

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (dialog->list, 10, 200);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (dialog->custom,
			  "toggled",
			  G_CALLBACK (choose_format_dialog_button_toggled),
			  dialog);
	g_signal_connect (dialog->use_list,
			  "toggled",
			  G_CALLBACK (choose_format_dialog_button_toggled),
			  dialog);
	g_signal_connect (dialog->dialog,
			  "destroy",
			  G_CALLBACK (choose_format_dialog_destroyed),
			  dialog);
	g_signal_connect (dialog->custom_entry,
			  "changed",
			  G_CALLBACK (updated_custom_format_example),
			  dialog->custom_format_example);
	g_signal_connect (dialog->list,
			  "row_activated",
			  G_CALLBACK (choose_format_dialog_row_activated),
			  dialog);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;
}

static void
choose_format_dialog_response_cb (GtkWidget          *widget,
				  gint                response,
				  ChooseFormatDialog *dialog)
{
	switch (response)
	{
		case GTK_RESPONSE_HELP:
		{
			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_HELP");
			gedit_app_show_help (gedit_app_get_default (),
					     GTK_WINDOW (widget),
					     NULL,
					     "gedit-insert-date-time-plugin");
			break;
		}
		case GTK_RESPONSE_OK:
		{
			gchar *the_time;

			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_OK");

			/* Get the user's chosen format */
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
			{
				gint sel_format;

				sel_format = get_format_from_list (dialog->list);
				the_time = get_time (formats[sel_format]);

				g_settings_set_enum (dialog->settings,
						     PROMPT_TYPE_KEY,
						     PROMPT_SELECTED_FORMAT);
				g_settings_set_string (dialog->settings,
						       SELECTED_FORMAT_KEY,
						       formats[sel_format]);
			}
			else
			{
				const gchar *format;

				format = gtk_entry_get_text (GTK_ENTRY (dialog->custom_entry));
				the_time = get_time (format);

				g_settings_set_enum (dialog->settings,
						     PROMPT_TYPE_KEY,
						     PROMPT_CUSTOM_FORMAT);
				g_settings_set_string (dialog->settings,
						       CUSTOM_FORMAT_KEY,
						       format);
			}

			g_return_if_fail (the_time != NULL);

			real_insert_time (dialog->buffer, the_time);
			g_free (the_time);

			gtk_widget_destroy (dialog->dialog);
			break;
		}
		case GTK_RESPONSE_CANCEL:
			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_CANCEL");
			gtk_widget_destroy (dialog->dialog);
	}
}

static void
time_cb (GtkAction       *action,
	 GeditTimePlugin *plugin)
{
	GeditTimePluginPrivate *priv;
	GtkTextBuffer *buffer;
	GeditTimePluginPromptType prompt_type;
	gchar *the_time = NULL;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	buffer = GTK_TEXT_BUFFER (gedit_window_get_active_document (priv->window));
	g_return_if_fail (buffer != NULL);

	prompt_type = g_settings_get_enum (plugin->priv->settings,
					   PROMPT_TYPE_KEY);

	if (prompt_type == USE_CUSTOM_FORMAT)
	{
		gchar *cf = get_custom_format (plugin);
		the_time = get_time (cf);
		g_free (cf);
	}
	else if (prompt_type == USE_SELECTED_FORMAT)
	{
		gchar *sf = get_selected_format (plugin);
		the_time = get_time (sf);
		g_free (sf);
	}
	else
	{
		ChooseFormatDialog *dialog;

		dialog = get_choose_format_dialog (GTK_WINDOW (priv->window),
						   prompt_type,
						   plugin);
		if (dialog != NULL)
		{
			dialog->buffer = buffer;
			dialog->settings = plugin->priv->settings;

			g_signal_connect (dialog->dialog,
					  "response",
					  G_CALLBACK (choose_format_dialog_response_cb),
					  dialog);

			gtk_widget_show (GTK_WIDGET (dialog->dialog));
		}

		return;
	}

	g_return_if_fail (the_time != NULL);

	real_insert_time (buffer, the_time);

	g_free (the_time);
}

static GtkWidget *
gedit_time_plugin_create_configure_widget (PeasGtkConfigurable *configurable)
{
	TimeConfigureWidget *widget;

	widget = get_configure_widget (GEDIT_TIME_PLUGIN (configurable));

	return widget->content;
}

static void
gedit_time_plugin_class_init (GeditTimePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_time_plugin_dispose;
	object_class->set_property = gedit_time_plugin_set_property;
	object_class->get_property = gedit_time_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");

	g_type_class_add_private (object_class, sizeof (GeditTimePluginPrivate));
}

static void
gedit_time_plugin_class_finalize (GeditTimePluginClass *klass)
{
}

static void
peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface)
{
	iface->create_configure_widget = gedit_time_plugin_create_configure_widget;
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_time_plugin_activate;
	iface->deactivate = gedit_time_plugin_deactivate;
	iface->update_state = gedit_time_plugin_update_state;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_time_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_TIME_PLUGIN);
	peas_object_module_register_extension_type (module,
						    PEAS_GTK_TYPE_CONFIGURABLE,
						    GEDIT_TYPE_TIME_PLUGIN);
}

/* ex:ts=8:noet: */
