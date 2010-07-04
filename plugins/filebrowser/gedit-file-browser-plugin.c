/*
 * gedit-file-browser-plugin.c - Gedit plugin providing easy file access
 * from the sidepanel
 *
 * Copyright (C) 2006 - Jesse van den Kieboom <jesse@icecrew.nl>
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

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-commands.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-utils.h>

#include "gedit-file-browser-enum-types.h"
#include "gedit-file-browser-plugin.h"
#include "gedit-file-browser-utils.h"
#include "gedit-file-browser-error.h"
#include "gedit-file-browser-widget.h"
#include "gedit-file-browser-messages.h"

#define FILEBROWSER_BASE_SETTINGS	"org.gnome.gedit.plugins.filebrowser"
#define FILEBROWSER_TREE_VIEW		"tree-view"
#define FILEBROWSER_ROOT		"root"
#define FILEBROWSER_VIRTUAL_ROOT	"virtual-root"
#define FILEBROWSER_ENABLE_REMOTE	"enable-remote"
#define FILEBROWSER_OPEN_AT_FIRST_DOC	"open-at-first-doc"
#define FILEBROWSER_FILTER_MODE		"filter-mode"
#define FILEBROWSER_FILTER_PATTERN	"filter-pattern"

#define NAUTILUS_BASE_SETTINGS		"org.gnome.Nautilus.preferences"
#define NAUTILUS_CLICK_POLICY_KEY	"click-policy"
#define NAUTILUS_ENABLE_DELETE_KEY	"enable-delete"
#define NAUTILUS_CONFIRM_TRASH_KEY	"confirm-trash"

#define TERMINAL_BASE_SETTINGS		"org.gnome.Desktop.Applications.Terminal"
#define TERMINAL_EXEC_KEY		"exec"

#define GEDIT_FILE_BROWSER_PLUGIN_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_FILE_BROWSER_PLUGIN, GeditFileBrowserPluginPrivate))

struct _GeditFileBrowserPluginPrivate
{
	GSettings              *settings;
	GSettings              *nautilus_settings;
	GSettings              *terminal_settings;

	GeditWindow            *window;

	GeditFileBrowserWidget *tree_widget;
	gulong                  merge_id;
	GtkActionGroup         *action_group;
	GtkActionGroup	       *single_selection_action_group;
	gboolean	        auto_root;
	gulong                  end_loading_handle;
	gboolean		confirm_trash;

	guint			click_policy_handle;
	guint			enable_delete_handle;
	guint			confirm_trash_handle;
};

static void gedit_window_activatable_iface_init	(GeditWindowActivatableInterface *iface);

static void on_location_activated_cb     (GeditFileBrowserWidget        *widget,
                                          GFile                         *location,
                                          GeditWindow                   *window);
static void on_error_cb                  (GeditFileBrowserWidget        *widget,
                                          guint                          code,
                                          gchar const                   *message,
                                          GeditFileBrowserPlugin        *plugin);
static void on_model_set_cb              (GeditFileBrowserView          *widget,
                                          GParamSpec                    *param,
                                          GeditFileBrowserPlugin        *plugin);
static void on_virtual_root_changed_cb   (GeditFileBrowserStore         *model,
                                          GParamSpec                    *param,
                                          GeditFileBrowserPlugin        *plugin);
static void on_rename_cb		 (GeditFileBrowserStore         *model,
					  GFile                         *oldfile,
					  GFile                         *newfile,
					  GeditWindow                   *window);
static void on_tab_added_cb              (GeditWindow                   *window,
                                          GeditTab                      *tab,
                                          GeditFileBrowserPlugin        *plugin);
static gboolean on_confirm_delete_cb     (GeditFileBrowserWidget        *widget,
                                          GeditFileBrowserStore         *store,
                                          GList                         *rows,
                                          GeditFileBrowserPlugin        *plugin);
static gboolean on_confirm_no_trash_cb   (GeditFileBrowserWidget        *widget,
                                          GList                         *files,
                                          GeditWindow                   *window);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditFileBrowserPlugin,
				gedit_file_browser_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)	\
													\
				gedit_file_browser_enum_and_flag_register_type	(type_module);		\
				_gedit_file_bookmarks_store_register_type	(type_module);		\
				_gedit_file_browser_store_register_type		(type_module);		\
				_gedit_file_browser_view_register_type		(type_module);		\
				_gedit_file_browser_widget_register_type	(type_module);		\
)

static void
gedit_file_browser_plugin_init (GeditFileBrowserPlugin *plugin)
{
	plugin->priv = GEDIT_FILE_BROWSER_PLUGIN_GET_PRIVATE (plugin);

	plugin->priv->settings = g_settings_new (FILEBROWSER_BASE_SETTINGS);
	plugin->priv->nautilus_settings = g_settings_new (NAUTILUS_BASE_SETTINGS);
	plugin->priv->terminal_settings = g_settings_new (TERMINAL_BASE_SETTINGS);
}

static void
gedit_file_browser_plugin_dispose (GObject *object)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (object);

	if (plugin->priv->settings != NULL)
	{
		g_object_unref (plugin->priv->settings);
		plugin->priv->settings = NULL;
	}

	if (plugin->priv->nautilus_settings != NULL)
	{
		g_object_unref (plugin->priv->nautilus_settings);
		plugin->priv->nautilus_settings = NULL;
	}

	if (plugin->priv->terminal_settings != NULL)
	{
		g_object_unref (plugin->priv->terminal_settings);
		plugin->priv->terminal_settings = NULL;
	}

	G_OBJECT_CLASS (gedit_file_browser_plugin_parent_class)->dispose (object);
}

static void
on_end_loading_cb (GeditFileBrowserStore  *store,
                   GtkTreeIter            *iter,
                   GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;

	/* Disconnect the signal */
	g_signal_handler_disconnect (store, priv->end_loading_handle);
	priv->end_loading_handle = 0;
	priv->auto_root = FALSE;
}

static void
prepare_auto_root (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GeditFileBrowserStore *store;

	priv->auto_root = TRUE;

	store = gedit_file_browser_widget_get_browser_store (priv->tree_widget);

	if (priv->end_loading_handle != 0)
	{
		g_signal_handler_disconnect (store, priv->end_loading_handle);
		priv->end_loading_handle = 0;
	}

	priv->end_loading_handle = g_signal_connect (store,
	                                               "end-loading",
	                                               G_CALLBACK (on_end_loading_cb),
	                                               plugin);
}

static void
restore_default_location (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *root;
	gchar *virtual_root;
	gboolean bookmarks;
	gboolean remote;

	bookmarks = !g_settings_get_boolean (priv->settings,
					     FILEBROWSER_TREE_VIEW);

	if (bookmarks)
	{
		gedit_file_browser_widget_show_bookmarks (priv->tree_widget);
		return;
	}

	root = g_settings_get_string (priv->settings,
				      FILEBROWSER_ROOT);
	virtual_root = g_settings_get_string (priv->settings,
					      FILEBROWSER_VIRTUAL_ROOT);

	remote = g_settings_get_boolean (priv->settings,
					 FILEBROWSER_ENABLE_REMOTE);

	if (root != NULL && *root != '\0')
	{
		GFile *rootfile;
		GFile *vrootfile;

		rootfile = g_file_new_for_uri (root);
		vrootfile = g_file_new_for_uri (virtual_root);

		if (remote || g_file_is_native (rootfile))
		{
			if (virtual_root != NULL && *virtual_root != '\0')
			{
				prepare_auto_root (plugin);
				gedit_file_browser_widget_set_root_and_virtual_root (priv->tree_widget,
					                                             rootfile,
					                                             vrootfile);
			}
			else
			{
				prepare_auto_root (plugin);
				gedit_file_browser_widget_set_root (priv->tree_widget,
					                            rootfile,
					                            TRUE);
			}
		}

		g_object_unref (rootfile);
		g_object_unref (vrootfile);
	}

	g_free (root);
	g_free (virtual_root);
}

static GeditFileBrowserViewClickPolicy
click_policy_from_string (gchar const *click_policy)
{
	if (click_policy && strcmp (click_policy, "single") == 0)
		return GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE;
	else
		return GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_DOUBLE;
}

static void
on_click_policy_changed (GSettings              *settings,
			 const gchar            *key,
			 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *click_policy;
	GeditFileBrowserViewClickPolicy policy = GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_DOUBLE;
	GeditFileBrowserView *view;

	click_policy = g_settings_get_string (settings, key);
	policy = click_policy_from_string (click_policy);
	g_free (click_policy);

	view = gedit_file_browser_widget_get_browser_view (priv->tree_widget);
	gedit_file_browser_view_set_click_policy (view, policy);
}

static void
on_confirm_trash_changed (GSettings              *settings,
		 	  const gchar            *key,
			  GeditFileBrowserPlugin *plugin)
{
	plugin->priv->confirm_trash = g_settings_get_boolean (settings, key);
}

static void
install_nautilus_prefs (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *pref;
	gboolean prefb;
	GeditFileBrowserViewClickPolicy policy;
	GeditFileBrowserView *view;

	/* Get click_policy */
	pref = g_settings_get_string (priv->nautilus_settings,
				      NAUTILUS_CLICK_POLICY_KEY);

	policy = click_policy_from_string (pref);

	view = gedit_file_browser_widget_get_browser_view (priv->tree_widget);
	gedit_file_browser_view_set_click_policy (view, policy);

	if (pref)
	{
		priv->click_policy_handle =
			g_signal_connect (priv->nautilus_settings,
					  "changed::" NAUTILUS_CLICK_POLICY_KEY,
					  G_CALLBACK (on_click_policy_changed),
					  plugin);
		g_free (pref);
	}

	/* Bind enable-delete */
	g_settings_bind (priv->nautilus_settings,
			 NAUTILUS_ENABLE_DELETE_KEY,
			 priv->tree_widget,
			 "enable-delete",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	/* Get confirm_trash */
	prefb = g_settings_get_boolean (priv->nautilus_settings,
					NAUTILUS_CONFIRM_TRASH_KEY);

	priv->confirm_trash = prefb;

	priv->confirm_trash_handle =
		g_signal_connect (priv->nautilus_settings,
				  "changed::" NAUTILUS_CONFIRM_TRASH_KEY,
				  G_CALLBACK (on_confirm_trash_changed),
				  plugin);
}

static void
set_root_from_doc (GeditFileBrowserPlugin *plugin,
                   GeditDocument          *doc)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GFile *file;
	GFile *parent;

	if (doc == NULL)
		return;

	file = gedit_document_get_location (doc);
	if (file == NULL)
		return;

	parent = g_file_get_parent (file);

	if (parent != NULL)
	{
		gedit_file_browser_widget_set_root (priv->tree_widget,
				                    parent,
				                    TRUE);

		g_object_unref (parent);
	}

	g_object_unref (file);
}

static void
on_action_set_active_root (GtkAction              *action,
                           GeditFileBrowserPlugin *plugin)
{
	set_root_from_doc (plugin,
	                   gedit_window_get_active_document (plugin->priv->window));
}

static gchar *
get_terminal (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *terminal;

	terminal = g_settings_get_string (priv->terminal_settings,
					  TERMINAL_EXEC_KEY);

	if (terminal == NULL)
	{
		const gchar *term = g_getenv ("TERM");

		if (term != NULL)
			terminal = g_strdup (term);
		else
			terminal = g_strdup ("xterm");
	}

	return terminal;
}

static void
on_action_open_terminal (GtkAction              *action,
                         GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *terminal;
	gchar *local;
	gchar *argv[2];
	GFile *file;

	GtkTreeIter iter;
	GeditFileBrowserStore *store;

	/* Get the current directory */
	if (!gedit_file_browser_widget_get_selected_directory (priv->tree_widget, &iter))
		return;

	store = gedit_file_browser_widget_get_browser_store (priv->tree_widget);
	gtk_tree_model_get (GTK_TREE_MODEL (store),
	                    &iter,
	                    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION,
	                    &file,
	                    -1);

	if (file == NULL)
		return;

	terminal = get_terminal (plugin);

	local = g_file_get_path (file);

	argv[0] = terminal;
	argv[1] = NULL;

	g_spawn_async (local,
	               argv,
	               NULL,
	               G_SPAWN_SEARCH_PATH,
	               NULL,
	               NULL,
	               NULL,
	               NULL);

	g_free (terminal);
	g_free (local);
}

static void
on_selection_changed_cb (GtkTreeSelection       *selection,
			 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GtkTreeView *tree_view;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean sensitive;
	GFile *location;

	tree_view = GTK_TREE_VIEW (gedit_file_browser_widget_get_browser_view (priv->tree_widget));
	model = gtk_tree_view_get_model (tree_view);

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	sensitive = gedit_file_browser_widget_get_selected_directory (priv->tree_widget, &iter);

	if (sensitive)
	{
		gtk_tree_model_get (model, &iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION,
				    &location, -1);

		sensitive = gedit_utils_location_has_file_scheme (location);
	}

	gtk_action_set_sensitive (
		gtk_action_group_get_action (priv->single_selection_action_group,
					     "OpenTerminal"),
		sensitive);
}

#define POPUP_UI ""                             \
"<ui>"                                          \
"  <popup name=\"FilePopup\">"                  \
"    <placeholder name=\"FilePopup_Opt1\">"     \
"      <menuitem action=\"SetActiveRoot\"/>"    \
"    </placeholder>"                            \
"    <placeholder name=\"FilePopup_Opt4\">"     \
"      <menuitem action=\"OpenTerminal\"/>"     \
"    </placeholder>"                            \
"  </popup>"                                    \
"  <popup name=\"BookmarkPopup\">"              \
"    <placeholder name=\"BookmarkPopup_Opt1\">" \
"      <menuitem action=\"SetActiveRoot\"/>"    \
"    </placeholder>"                            \
"  </popup>"                                    \
"</ui>"

static GtkActionEntry extra_actions[] =
{
	{"SetActiveRoot", GTK_STOCK_JUMP_TO, N_("_Set root to active document"),
	 NULL,
	 N_("Set the root to the active document location"),
	 G_CALLBACK (on_action_set_active_root)}
};

static GtkActionEntry extra_single_selection_actions[] = {
	{"OpenTerminal", "utilities-terminal", N_("_Open terminal here"),
	 NULL,
	 N_("Open a terminal at the currently opened directory"),
	 G_CALLBACK (on_action_open_terminal)}
};

static void
add_popup_ui (GeditWindow            *window,
	      GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GtkUIManager *manager;
	GtkActionGroup *action_group;
	GError *error = NULL;

	manager = gedit_file_browser_widget_get_ui_manager (priv->tree_widget);

	action_group = gtk_action_group_new ("FileBrowserPluginExtra");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      extra_actions,
				      G_N_ELEMENTS (extra_actions),
				      plugin);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	priv->action_group = action_group;

	action_group = gtk_action_group_new ("FileBrowserPluginSingleSelectionExtra");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      extra_single_selection_actions,
				      G_N_ELEMENTS (extra_single_selection_actions),
				      window);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	priv->single_selection_action_group = action_group;

	priv->merge_id = gtk_ui_manager_add_ui_from_string (manager,
							      POPUP_UI,
							      -1,
							      &error);

	if (priv->merge_id == 0)
	{
		g_warning("Unable to merge UI: %s", error->message);
		g_error_free(error);
	}
}

static void
remove_popup_ui (GeditWindow            *window,
		 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GtkUIManager *manager;

	manager = gedit_file_browser_widget_get_ui_manager (priv->tree_widget);
	gtk_ui_manager_remove_ui (manager, priv->merge_id);

	gtk_ui_manager_remove_action_group (manager, priv->action_group);
	g_object_unref (priv->action_group);

	gtk_ui_manager_remove_action_group (manager, priv->single_selection_action_group);
	g_object_unref (priv->single_selection_action_group);
}

static void
gedit_file_browser_plugin_update_state (GeditWindowActivatable *activatable,
					GeditWindow            *window)
{
	GeditFileBrowserPluginPrivate *priv = GEDIT_FILE_BROWSER_PLUGIN (activatable)->priv;
	GeditDocument *doc;

	doc = gedit_window_get_active_document (window);

	gtk_action_set_sensitive (gtk_action_group_get_action (priv->action_group,
	                                                       "SetActiveRoot"),
	                          doc != NULL && !gedit_document_is_untitled (doc));
}

static void
gedit_file_browser_plugin_activate (GeditWindowActivatable *activatable,
				    GeditWindow            *window)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (activatable);
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GeditPanel *panel;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GeditFileBrowserStore *store;
	gchar *data_dir;

	priv->window = window;

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (activatable));
	priv->tree_widget = GEDIT_FILE_BROWSER_WIDGET (gedit_file_browser_widget_new (data_dir));
	g_free (data_dir);

	g_signal_connect (priv->tree_widget,
			  "location-activated",
			  G_CALLBACK (on_location_activated_cb), window);

	g_signal_connect (priv->tree_widget,
			  "error", G_CALLBACK (on_error_cb), plugin);

	g_signal_connect (priv->tree_widget,
	                  "confirm-delete",
	                  G_CALLBACK (on_confirm_delete_cb),
	                  plugin);

	g_signal_connect (priv->tree_widget,
	                  "confirm-no-trash",
	                  G_CALLBACK (on_confirm_no_trash_cb),
	                  window);

	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW
			  (gedit_file_browser_widget_get_browser_view
			  (priv->tree_widget))),
			  "changed",
			  G_CALLBACK (on_selection_changed_cb),
			  plugin);

	g_settings_bind (priv->settings,
	                 FILEBROWSER_FILTER_PATTERN,
	                 priv->tree_widget,
	                 FILEBROWSER_FILTER_PATTERN,
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	panel = gedit_window_get_side_panel (window);
	pixbuf = gedit_file_browser_utils_pixbuf_from_theme ("system-file-manager",
	                                                     GTK_ICON_SIZE_MENU);

	if (pixbuf)
	{
		image = gtk_image_new_from_pixbuf (pixbuf);
		g_object_unref(pixbuf);
	}
	else
	{
		image = gtk_image_new_from_stock (GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU);
	}

	gtk_widget_show (image);
	gedit_panel_add_item (panel,
	                      GTK_WIDGET (priv->tree_widget),
	                      "GeditFileBrowserPanel",
	                      _("File Browser"),
	                      image);
	gtk_widget_show (GTK_WIDGET (priv->tree_widget));

	add_popup_ui (window, plugin);

	/* Install nautilus preferences */
	install_nautilus_prefs (plugin);

	/* Connect signals to store the last visited location */
	g_signal_connect (gedit_file_browser_widget_get_browser_view (priv->tree_widget),
	                  "notify::model",
	                  G_CALLBACK (on_model_set_cb),
	                  plugin);

	store = gedit_file_browser_widget_get_browser_store (priv->tree_widget);

	g_settings_bind (priv->settings,
	                 "filter-mode",
	                 store,
	                 "filter-mode",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	g_signal_connect (store,
	                  "notify::virtual-root",
	                  G_CALLBACK (on_virtual_root_changed_cb),
	                  plugin);

	g_signal_connect (store,
			  "rename",
			  G_CALLBACK (on_rename_cb),
			  window);

	g_signal_connect (window,
	                  "tab-added",
	                  G_CALLBACK (on_tab_added_cb),
	                  plugin);

	/* Register messages on the bus */
	gedit_file_browser_messages_register (window, priv->tree_widget);

	gedit_file_browser_plugin_update_state (activatable, window);
}

static void
gedit_file_browser_plugin_deactivate (GeditWindowActivatable *activatable,
				      GeditWindow            *window)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (activatable);
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GeditPanel *panel;


	/* Unregister messages from the bus */
	gedit_file_browser_messages_unregister (window);

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (window,
	                                      G_CALLBACK (on_tab_added_cb),
	                                      plugin);

	if (priv->click_policy_handle)
	{
		g_signal_handler_disconnect (priv->nautilus_settings,
					     priv->click_policy_handle);
	}

	if (priv->enable_delete_handle)
	{
		g_signal_handler_disconnect (priv->nautilus_settings,
					     priv->enable_delete_handle);
	}

	if (priv->confirm_trash_handle)
	{
		g_signal_handler_disconnect (priv->nautilus_settings,
					     priv->confirm_trash_handle);
	}

	remove_popup_ui (window, plugin);

	panel = gedit_window_get_side_panel (window);
	gedit_panel_remove_item (panel, GTK_WIDGET (priv->tree_widget));
}

static void
gedit_file_browser_plugin_class_init (GeditFileBrowserPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_file_browser_plugin_dispose;

	g_type_class_add_private (object_class,
				  sizeof (GeditFileBrowserPluginPrivate));
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_file_browser_plugin_activate;
	iface->deactivate = gedit_file_browser_plugin_deactivate;
	iface->update_state = gedit_file_browser_plugin_update_state;
}

static void
gedit_file_browser_plugin_class_finalize (GeditFileBrowserPluginClass *klass)
{
}

/* Callbacks */
static void
on_location_activated_cb (GeditFileBrowserWidget *tree_widget,
		          GFile                  *location,
		          GeditWindow            *window)
{
	gedit_commands_load_location (window, location, NULL, 0, 0);
}

static void
on_error_cb (GeditFileBrowserWidget *tree_widget,
	     guint                   code,
	     gchar const            *message,
	     GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *title;
	GtkWidget *dlg;

	/* Do not show the error when the root has been set automatically */
	if (priv->auto_root && (code == GEDIT_FILE_BROWSER_ERROR_SET_ROOT ||
	                          code == GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY))
	{
		/* Show bookmarks */
		gedit_file_browser_widget_show_bookmarks (priv->tree_widget);
		return;
	}

	switch (code)
	{
		case GEDIT_FILE_BROWSER_ERROR_NEW_DIRECTORY:
			title = _("An error occurred while creating a new directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_NEW_FILE:
			title = _("An error occurred while creating a new file");
			break;
		case GEDIT_FILE_BROWSER_ERROR_RENAME:
			title = _("An error occurred while renaming a file or directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_DELETE:
			title = _("An error occurred while deleting a file or directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY:
			title = _("An error occurred while opening a directory in the file manager");
			break;
		case GEDIT_FILE_BROWSER_ERROR_SET_ROOT:
			title = _("An error occurred while setting a root directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY:
			title = _("An error occurred while loading a directory");
			break;
		default:
			title = _("An error occurred");
			break;
	}

	dlg = gtk_message_dialog_new (GTK_WINDOW (priv->window),
				      GTK_DIALOG_MODAL |
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				      "%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
						  "%s", message);

	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
}

static void
on_model_set_cb (GeditFileBrowserView   *widget,
                 GParamSpec             *param,
                 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (gedit_file_browser_widget_get_browser_view (priv->tree_widget)));

	if (model == NULL)
		return;

	g_settings_set_boolean (priv->settings,
	                        FILEBROWSER_TREE_VIEW,
	                        GEDIT_IS_FILE_BROWSER_STORE (model));
}

static void
on_rename_cb (GeditFileBrowserStore *store,
	      GFile                 *oldfile,
	      GFile                 *newfile,
	      GeditWindow           *window)
{
	GeditApp *app;
	GList *documents;
	GList *item;
	GeditDocument *doc;
	GFile *docfile;

	/* Find all documents and set its uri to newuri where it matches olduri */
	app = gedit_app_get_default ();
	documents = gedit_app_get_documents (app);

	for (item = documents; item; item = item->next)
	{
		doc = GEDIT_DOCUMENT (item->data);
		docfile = gedit_document_get_location (doc);

		if (!docfile)
			continue;

		if (g_file_equal (docfile, oldfile))
		{
			gedit_document_set_location (doc, newfile);
		}
		else
		{
			gchar *relative;

			relative = g_file_get_relative_path (oldfile, docfile);

			if (relative)
			{
				/* Relative now contains the part in docfile without
				   the prefix oldfile */

				g_object_unref (docfile);
				docfile = g_file_get_child (newfile, relative);

				gedit_document_set_location (doc, docfile);
			}

			g_free (relative);
		}

		g_object_unref (docfile);
	}

	g_list_free (documents);
}

static void
on_virtual_root_changed_cb (GeditFileBrowserStore  *store,
                            GParamSpec             *param,
                            GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GFile *root;
	GFile *virtual_root;
	gchar *uri_root = NULL;

	root = gedit_file_browser_store_get_root (store);

	if (!root)
	{
		return;
	}
	else
	{
		uri_root = g_file_get_uri (root);
		g_object_unref (root);
	}

	g_settings_set_string (priv->settings,
	                       FILEBROWSER_ROOT,
	                       uri_root);
	g_free (uri_root);

	virtual_root = gedit_file_browser_store_get_virtual_root (store);

	if (!virtual_root)
	{
		/* Set virtual to same as root then */
		g_settings_set_string (priv->settings,
		                       FILEBROWSER_VIRTUAL_ROOT,
		                       uri_root);
	}
	else
	{
		gchar *uri_vroot;

		uri_vroot = g_file_get_uri (virtual_root);

		g_settings_set_string (priv->settings,
		                       FILEBROWSER_VIRTUAL_ROOT,
		                       uri_vroot);
		g_free (uri_vroot);
		g_object_unref (virtual_root);
	}

	g_signal_handlers_disconnect_by_func (priv->window,
	                                      G_CALLBACK (on_tab_added_cb),
	                                      plugin);
}

static void
on_tab_added_cb (GeditWindow            *window,
                 GeditTab               *tab,
                 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gboolean open;
	gboolean load_default = TRUE;

	open = g_settings_get_boolean (priv->settings,
	                               FILEBROWSER_OPEN_AT_FIRST_DOC);

	if (open)
	{
		GeditDocument *doc;
		GFile *location;

		doc = gedit_tab_get_document (tab);

		location = gedit_document_get_location (doc);

		if (location != NULL)
		{
			if (gedit_utils_location_has_file_scheme (location))
			{
				prepare_auto_root (plugin);
				set_root_from_doc (plugin, doc);
				load_default = FALSE;
			}
			g_object_unref (location);
		}
	}

	if (load_default)
		restore_default_location (plugin);

	/* Disconnect this signal, it's only called once */
	g_signal_handlers_disconnect_by_func (window,
	                                      G_CALLBACK (on_tab_added_cb),
	                                      plugin);
}

static gchar *
get_filename_from_path (GtkTreeModel *model,
			GtkTreePath  *path)
{
	GtkTreeIter iter;
	GFile *location;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    -1);

	return gedit_file_browser_utils_file_basename (location);
}

static gboolean
on_confirm_no_trash_cb (GeditFileBrowserWidget *widget,
                        GList                  *files,
                        GeditWindow            *window)
{
	gchar *normal;
	gchar *message;
	gchar *secondary;
	gboolean result;

	message = _("Cannot move file to trash, do you\nwant to delete permanently?");

	if (files->next == NULL)
	{
		normal = gedit_file_browser_utils_file_basename (G_FILE (files->data));
	    	secondary = g_strdup_printf (_("The file \"%s\" cannot be moved to the trash."), normal);
		g_free (normal);
	}
	else
	{
		secondary = g_strdup (_("The selected files cannot be moved to the trash."));
	}

	result = gedit_file_browser_utils_confirmation_dialog (window,
	                                                       GTK_MESSAGE_QUESTION,
	                                                       message,
	                                                       secondary,
	                                                       GTK_STOCK_DELETE,
	                                                       NULL);
	g_free (secondary);

	return result;
}

static gboolean
on_confirm_delete_cb (GeditFileBrowserWidget *widget,
                      GeditFileBrowserStore  *store,
                      GList                  *paths,
                      GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *normal;
	gchar *message;
	gchar *secondary;
	gboolean result;

	if (!priv->confirm_trash)
		return TRUE;

	if (paths->next == NULL)
	{
		normal = get_filename_from_path (GTK_TREE_MODEL (store), (GtkTreePath *)(paths->data));
		message = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\"?"), normal);
		g_free (normal);
	}
	else
	{
		message = g_strdup (_("Are you sure you want to permanently delete the selected files?"));
	}

	secondary = _("If you delete an item, it is permanently lost.");

	result = gedit_file_browser_utils_confirmation_dialog (priv->window,
	                                                       GTK_MESSAGE_QUESTION,
	                                                       message,
	                                                       secondary,
	                                                       GTK_STOCK_DELETE,
	                                                       NULL);

	g_free (message);

	return result;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_file_browser_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_FILE_BROWSER_PLUGIN);
}

/* ex:ts=8:noet: */
