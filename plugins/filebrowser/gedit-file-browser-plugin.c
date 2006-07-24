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

#include <gedit/gedit-commands.h>
#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gconf/gconf-client.h>
#include <string.h>

#include "gedit-file-browser-enum-types.h"
#include "gedit-file-browser-plugin.h"
#include "gedit-file-browser-utils.h"
#include "gedit-file-browser-error.h"
#include "gedit-file-browser-widget.h"

#define WINDOW_DATA_KEY	        "GeditFileBrowserPluginWindowData"
#define FILE_BROWSER_BASE_KEY 	"/apps/gedit-2/plugins/filebrowser"

#define GEDIT_FILE_BROWSER_PLUGIN_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_FILE_BROWSER_PLUGIN, GeditFileBrowserPluginPrivate))

struct _GeditFileBrowserPluginPrivate 
{
	gpointer dummy;
};

typedef struct _GeditFileBrowserPluginData 
{
	GeditFileBrowserWidget * tree_widget;
	gulong                   merge_id;
	GtkActionGroup         * action_group;	
} GeditFileBrowserPluginData;

static void on_uri_activated_cb          (GeditFileBrowserWidget * widget,
                                          gchar const *uri, 
                                          GeditWindow * window);
static void on_error_cb                  (GeditFileBrowserWidget * widget,
                                          guint code,
                                          gchar const *message, 
                                          GeditWindow * window);
static void on_model_set_cb              (GeditFileBrowserView * widget,
                                          GParamSpec *arg1,
                                          GeditWindow * window);
static void on_virtual_root_changed_cb   (GeditFileBrowserStore * model,
                                          GParamSpec * param,
                                          GeditWindow * window);
static void on_filter_mode_changed_cb    (GeditFileBrowserStore * model,
                                          GParamSpec * param,
                                          GeditWindow * window);
static void on_filter_pattern_changed_cb (GeditFileBrowserWidget * widget,
                                          GParamSpec * param,
                                          GeditWindow * window);
static void on_tab_added_cb              (GeditWindow * window,
                                          GeditTab * tab,
                                          GeditFileBrowserPluginData * data);
static gboolean on_confirm_delete_cb     (GeditFileBrowserWidget * widget,
                                          GeditFileBrowserStore * store,
                                          GtkTreeIter * iter,
                                          GeditWindow * window);
static gboolean on_confirm_no_trash_cb   (GeditFileBrowserWidget * widget,
                                          GeditFileBrowserStore * store,
                                          GtkTreeIter * iter,
                                          GeditWindow * window);

GEDIT_PLUGIN_REGISTER_TYPE_WITH_CODE (GeditFileBrowserPlugin, filetree_plugin, 	\
	gedit_file_browser_enum_and_flag_register_type (module);		\
	gedit_file_browser_store_register_type         (module);		\
	gedit_file_bookmarks_store_register_type       (module);		\
	gedit_file_browser_view_register_type	       (module);		\
	gedit_file_browser_widget_register_type	       (module);		\
)


static void
filetree_plugin_init (GeditFileBrowserPlugin * plugin)
{
	plugin->priv = GEDIT_FILE_BROWSER_PLUGIN_GET_PRIVATE (plugin);
}

static void
filetree_plugin_finalize (GObject * object)
{
	//GeditFileBrowserPlugin * plugin = GEDIT_FILE_BROWSER_PLUGIN (object);
	
	G_OBJECT_CLASS (filetree_plugin_parent_class)->finalize (object);
}

static GeditFileBrowserPluginData *
get_plugin_data (GeditWindow * window)
{
	return (GeditFileBrowserPluginData *) (g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY));
}

static void 
restore_default_location (GeditFileBrowserPlugin * plugin, 
                          GeditFileBrowserPluginData *data)
{
	gchar * root;
	gchar * virtual;
	gboolean bookmarks;
	gboolean remote;
	GConfClient * client;
	GnomeVFSURI * uri = NULL;

	client = gconf_client_get_default ();

	if (!client)
		return;

	bookmarks = !gconf_client_get_bool (client,
	                                    FILE_BROWSER_BASE_KEY "/on_load/tree_view",
	                                    NULL);

	if (bookmarks) {
		g_object_unref (client);
		gedit_file_browser_widget_show_bookmarks (data->tree_widget);
		return;
	}
	
	root = gconf_client_get_string (client, 
	                                FILE_BROWSER_BASE_KEY "/on_load/root", 
	                                NULL);
	virtual = gconf_client_get_string (client, 
	                                   FILE_BROWSER_BASE_KEY "/on_load/virtual_root", 
	                                   NULL);

	remote = gconf_client_get_bool (client,
	                                FILE_BROWSER_BASE_KEY "/on_load/enable_remote",
	                                NULL);

	if (root != NULL && *root != '\0') {
		uri = gnome_vfs_uri_new (root);

		if (uri == NULL || (!remote && !gnome_vfs_uri_is_local (uri))) {
		} else if (virtual != NULL && virtual != '\0') {
			gedit_file_browser_widget_set_root_and_virtual_root (data->tree_widget, 
			                                                     root,
			                                                     virtual);
		} else {
			gedit_file_browser_widget_set_root (data->tree_widget,
			                                    root,
			                                    TRUE);
		}
	}

	if (uri)
		gnome_vfs_uri_unref (uri);

	g_object_unref (client);
	g_free (root);
	g_free (virtual);
}

static void
restore_filter (GeditFileBrowserPluginData * data) 
{
	GConfClient * client;
	gchar *filter_mode;
	GeditFileBrowserStoreFilterMode mode;
	gchar *pattern;

	client = gconf_client_get_default ();
	
	if (!client)
		return;

	/* Get filter_mode */
	filter_mode = gconf_client_get_string (client,
	                                       FILE_BROWSER_BASE_KEY "/filter_mode",
	                                       NULL);
	
	/* Filter mode */
	mode = gedit_file_browser_store_filter_mode_get_default ();
	
	if (filter_mode != NULL) {
		if (strcmp (filter_mode, "hidden") == 0) {
			mode = GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN;
		} else if (strcmp (filter_mode, "binary") == 0) {
			mode = GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY;
		} else if (strcmp (filter_mode, "hidden_and_binary") == 0 ||
		         strcmp (filter_mode, "binary_and_hidden") == 0) {
			mode = GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN |
			       GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY;
		} else if (strcmp (filter_mode, "none") == 0 || 
		           *filter_mode == '\0') {
			mode = GEDIT_FILE_BROWSER_STORE_FILTER_MODE_NONE;
		}
	}
	
	/* Set the filter mode */
	gedit_file_browser_store_set_filter_mode (
	    gedit_file_browser_widget_get_browser_store (data->tree_widget),
	    mode);
	    
	
	pattern = gconf_client_get_string (client,
	                                   FILE_BROWSER_BASE_KEY "/filter_pattern",
	                                   NULL);

	gedit_file_browser_widget_set_filter_pattern (data->tree_widget, 
	                                              pattern);
	g_object_unref (client);
}

static void
set_root_from_doc (GeditFileBrowserPluginData * data,
                   GeditDocument * doc)
{
	gchar * uri;
	gchar * root;
	GnomeVFSURI * guri;
	
	if (doc == NULL)
		return;
	
	uri = gedit_document_get_uri (doc);

	if (uri == NULL)
		return;
	
	guri = gnome_vfs_uri_new (uri);
	
	if (guri == NULL)
		return;
	
	if (gnome_vfs_uri_has_parent(guri))
		guri = gnome_vfs_uri_get_parent (guri);

	root = gnome_vfs_uri_to_string (guri, GNOME_VFS_URI_HIDE_NONE);

	gedit_file_browser_widget_set_root (data->tree_widget,
		                            root,
		                            TRUE);
	g_free (root);
	g_free (uri);
}

static void
on_action_set_active_root (GtkAction * action,
                           GeditWindow * window)
{
	GeditFileBrowserPluginData * data;
	
	data = get_plugin_data (window);
	set_root_from_doc (data, 
	                   gedit_window_get_active_document (window));
}

static gchar *
get_terminal ()
{
	GConfClient * client;
	gchar * terminal;

	client = gconf_client_get_default ();
	terminal = gconf_client_get_string (client, "/desktop/gnome/applications/terminal/exec", NULL);
	
	if (terminal == NULL)
		terminal = g_strdup(getenv("TERM"));
	
	if (terminal == NULL)
		terminal = g_strdup("xterm");
	
	return terminal;
}

static void
on_action_open_terminal (GtkAction * action,
                         GeditWindow * window)
{
	GeditFileBrowserPluginData * data;
	gchar * terminal;
	gchar * wd = NULL;
	gchar * local;
	gchar * argv[2];

	GtkTreeIter iter;
	GeditFileBrowserStore * store;
	
	data = get_plugin_data (window);

	/* Get the current directory */
	if (!gedit_file_browser_widget_get_selected_directory (data->tree_widget, &iter))
		return;

	store = gedit_file_browser_widget_get_browser_store (data->tree_widget);
	gtk_tree_model_get (GTK_TREE_MODEL (store), 
	                    &iter,
	                    GEDIT_FILE_BROWSER_STORE_COLUMN_URI,
	                    &wd,
	                    -1);
	
	if (wd == NULL)
		return;

	terminal = get_terminal ();
	local = gnome_vfs_get_local_path_from_uri (wd);

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
	g_free (wd);
	g_free (local);
}

#define POPUP_UI ""                          \
"<ui>"                                       \
"  <popup name=\"Popup\">"                   \
"    <placeholder name=\"Popup_Opt1\">"      \
"      <menuitem action=\"SetActiveRoot\"/>" \
"    </placeholder>"                         \
"    <placeholder name=\"Popup_Opt4\">"      \
"      <menuitem action=\"OpenTerminal\"/>"  \
"    </placeholder>"                         \
"  </popup>"                                 \
"</ui>"

static GtkActionEntry extra_actions[] = 
{
	{"SetActiveRoot", GTK_STOCK_JUMP_TO, N_("_Set root to active document"),
	 NULL,
	 N_("Set the root to the active document location"),
	 G_CALLBACK (on_action_set_active_root)},
	{"OpenTerminal", GTK_STOCK_EXECUTE, N_("_Open terminal here"),
	 NULL,
	 N_("Open a terminal at the currently opened directory"),
	 G_CALLBACK (on_action_open_terminal)},
};

static void
add_popup_ui (GeditWindow * window)
{
	GeditFileBrowserPluginData * data;
	GtkUIManager * manager;
	GtkActionGroup * action_group;
	GError * error = NULL;

	data = get_plugin_data (window);
	manager = gedit_file_browser_widget_get_ui_manager (data->tree_widget);

	action_group =
	    gtk_action_group_new ("FileBrowserPluginExtra");
	gtk_action_group_add_actions (action_group, extra_actions,
				      G_N_ELEMENTS (extra_actions),
				      window);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	data->action_group = action_group;

	data->merge_id = gtk_ui_manager_add_ui_from_string (manager, 
	                                                    POPUP_UI, 
	                                                    -1, 
	                                                    &error);

	if (data->merge_id == 0) {
		g_warning("Unable to merge UI: %s", error->message);
		g_error_free(error);
	}
}

static void
remove_popup_ui (GeditWindow * window)
{
	GeditFileBrowserPluginData * data;
	GtkUIManager * manager;
	
	data = get_plugin_data (window);
	manager = gedit_file_browser_widget_get_ui_manager (data->tree_widget);
	gtk_ui_manager_remove_ui (manager, data->merge_id);
	
	gtk_ui_manager_remove_action_group (manager, data->action_group);
	g_object_unref (data->action_group);
}

static void
impl_updateui (GeditPlugin * plugin, GeditWindow * window) 
{
	GeditFileBrowserPluginData * data;
	GeditDocument * doc;
	
	data = get_plugin_data (window);
	
	doc = gedit_window_get_active_document (window);
	
	gtk_action_set_sensitive (gtk_action_group_get_action (data->action_group, 
	                                                       "SetActiveRoot"),
	                          doc != NULL && 
	                          !gedit_document_is_untitled (doc));
}

static void
impl_activate (GeditPlugin * plugin, GeditWindow * window)
{
	GeditPanel * panel;
	GeditFileBrowserPluginData * data;
	GtkWidget * image;
	GdkPixbuf * pixbuf;
	GeditFileBrowserStore * store;

	data = g_new (GeditFileBrowserPluginData, 1);
	data->tree_widget = GEDIT_FILE_BROWSER_WIDGET (gedit_file_browser_widget_new ());

	g_signal_connect (data->tree_widget,
			  "uri-activated",
			  G_CALLBACK (on_uri_activated_cb), window);

	g_signal_connect (data->tree_widget,
			  "error", G_CALLBACK (on_error_cb), window);

	g_signal_connect (data->tree_widget,
	                  "notify::filter-pattern",
	                  G_CALLBACK (on_filter_pattern_changed_cb),
	                  window);

	g_signal_connect (data->tree_widget,
	                  "confirm-delete",
	                  G_CALLBACK (on_confirm_delete_cb),
	                  window);

	g_signal_connect (data->tree_widget,
	                  "confirm-no-trash",
	                  G_CALLBACK (on_confirm_no_trash_cb),
	                  window);

	panel = gedit_window_get_side_panel (window);
	pixbuf = gedit_file_browser_utils_pixbuf_from_theme("file-manager",  
	                                                    GTK_ICON_SIZE_MENU);
	
	if (pixbuf) {
		image = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);
	} else {
		image = gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU);
	}

	gtk_widget_show(image);
	gedit_panel_add_item (panel,
	                      GTK_WIDGET (data->tree_widget),
	                      _("File Browser"),
	                      image);
	gtk_widget_show (GTK_WIDGET (data->tree_widget));
	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, data);

	add_popup_ui (window);

	/* Restore the default location */
	restore_default_location (GEDIT_FILE_BROWSER_PLUGIN (plugin), data);
	
	/* Restore filter options */
	restore_filter (data);
	
	/* Connect signals to store the last visited location */
	g_signal_connect (gedit_file_browser_widget_get_browser_view (data->tree_widget),
	                  "notify::model",
	                  G_CALLBACK (on_model_set_cb),
	                  window);

	store = gedit_file_browser_widget_get_browser_store (data->tree_widget);
	g_signal_connect (store,
	                  "notify::virtual-root",
	                  G_CALLBACK (on_virtual_root_changed_cb),
	                  window);

	g_signal_connect (store,
	                  "notify::filter-mode",
	                  G_CALLBACK (on_filter_mode_changed_cb),
	                  window);

	g_signal_connect (window,
	                  "tab-added",
	                  G_CALLBACK (on_tab_added_cb),
	                  data);
	                  
	impl_updateui (plugin, window);
}

static void
impl_deactivate (GeditPlugin * plugin, GeditWindow * window)
{
	GeditFileBrowserPluginData * data;
	GeditPanel * panel;

	data = get_plugin_data (window);

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (window, 
	                                      G_CALLBACK (on_tab_added_cb),
	                                      data);

	remove_popup_ui (window);

	panel = gedit_window_get_side_panel (window);
	gedit_panel_remove_item (panel, GTK_WIDGET (data->tree_widget));

	g_free (data);
	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
filetree_plugin_class_init (GeditFileBrowserPluginClass * klass)
{
	GObjectClass  *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass * plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = filetree_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_updateui;

	g_type_class_add_private (object_class,
				  sizeof (GeditFileBrowserPluginPrivate));
}

/* Callbacks */
static void
on_uri_activated_cb (GeditFileBrowserWidget * tree_widget,
		     gchar const *uri, GeditWindow * window)
{
	gedit_commands_load_uri (window, uri, NULL, 0);
}

static void
on_error_cb (GeditFileBrowserWidget * tree_widget,
	     guint code, gchar const *message, GeditWindow * window)
{
	gchar * title;
	GtkWidget * dlg;

	switch (code) {
	case GEDIT_FILE_BROWSER_ERROR_NEW_DIRECTORY:
		title =
		    _("An error occurred while creating a new directory");
		break;
	case GEDIT_FILE_BROWSER_ERROR_NEW_FILE:
		title = _("An error occurred while creating a new file");
		break;
	case GEDIT_FILE_BROWSER_ERROR_RENAME:
		title =
		    _
		    ("An error occurred while renaming a file or directory");
		break;
	case GEDIT_FILE_BROWSER_ERROR_DELETE:
		title =
		    _
		    ("An error occurred while deleting a file or directory");
		break;
	case GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY:
		title =
		    _
		    ("An error occurred while opening a directory in the file manager");
		break;
	case GEDIT_FILE_BROWSER_ERROR_SET_ROOT:
		title =
		    _("An error occurred while setting a root directory");
		break;
	case GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY:
		title = 
		    _("An error occurred while loading a directory");
		break;
	default:
		title = _("An error occurred");
		break;
	}

	dlg = gtk_message_dialog_new (GTK_WINDOW (window),
				      GTK_DIALOG_MODAL |
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				      title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
						  message);

	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
}

static void 
on_model_set_cb (GeditFileBrowserView * widget,
                 GParamSpec *arg1,
                 GeditWindow * window)
{
	GeditFileBrowserPluginData * data = get_plugin_data (window);
	GtkTreeModel * model;
	GConfClient * client;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (gedit_file_browser_widget_get_browser_view (data->tree_widget)));
	
	if (model == NULL)
		return;
	
	client = gconf_client_get_default ();
	gconf_client_set_bool (client,
	                       FILE_BROWSER_BASE_KEY "/on_load/tree_view",
	                       GEDIT_IS_FILE_BROWSER_STORE (model),
	                       NULL);
	g_object_unref (client);
}

static void 
on_filter_mode_changed_cb (GeditFileBrowserStore * model,
                           GParamSpec * param,
                           GeditWindow * window)
{
	GConfClient * client;
	GeditFileBrowserStoreFilterMode mode;

	client = gconf_client_get_default ();
	
	if (!client)
		return;
	
	mode = gedit_file_browser_store_get_filter_mode (model);
	
	if ((mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN) &&
	    (mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY)) {
		gconf_client_set_string (client,
		                         FILE_BROWSER_BASE_KEY "/filter_mode",
		                         "hidden_and_binary",
		                         NULL);
	} else if (mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN) {
		gconf_client_set_string (client,
		                         FILE_BROWSER_BASE_KEY "/filter_mode",
		                         "hidden",
		                         NULL);	
	} else if (mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY) {
		gconf_client_set_string (client,
		                         FILE_BROWSER_BASE_KEY "/filter_mode",
		                         "binary",
		                         NULL);
	} else {
		gconf_client_set_string (client,
		                         FILE_BROWSER_BASE_KEY "/filter_mode",
		                         "none",
		                         NULL);
	}
	
	g_object_unref (client);
	
}

static void 
on_filter_pattern_changed_cb (GeditFileBrowserWidget * widget,
                              GParamSpec * param,
                              GeditWindow * window)
{
	GConfClient * client;
	gchar * pattern;

	client = gconf_client_get_default ();
	
	if (!client)
		return;
	
	g_object_get (G_OBJECT (widget), "filter-pattern", &pattern, NULL);
	
	if (pattern == NULL)
		gconf_client_set_string (client,
		                         FILE_BROWSER_BASE_KEY "/filter_pattern",
		                         "",
		                         NULL);
	else
		gconf_client_set_string (client,
		                         FILE_BROWSER_BASE_KEY "/filter_pattern",
		                         pattern,
		                         NULL);

	g_free (pattern);
}

static void 
on_virtual_root_changed_cb (GeditFileBrowserStore * store,
                            GParamSpec * param,
                            GeditWindow * window) 
{
	GeditFileBrowserPluginData * data = get_plugin_data (window);
	gchar * root;
	gchar * virtual;
	GConfClient * client;
	GnomeVFSURI * guri;

	root = gedit_file_browser_store_get_root (store);
	
	if (!root)
		return;

	client = gconf_client_get_default ();
	
	if (!client)
		return;

	gconf_client_set_string (client,
	                         FILE_BROWSER_BASE_KEY "/on_load/root",
	                         root,
	                         NULL);
	
	virtual = gedit_file_browser_store_get_virtual_root (store);

	if (!virtual) {
		/* Set virtual to same as root then */
		gconf_client_set_string (client,
	                                 FILE_BROWSER_BASE_KEY "/on_load/virtual_root",
	                                 root,
	                                 NULL);
	} else {
		guri = gnome_vfs_uri_new (virtual);
		gtk_action_set_sensitive (
			gtk_action_group_get_action (data->action_group, 
	                                            "OpenTerminal"),
	                guri != NULL && gnome_vfs_uri_is_local (guri));
		
		if (guri != NULL)
			gnome_vfs_uri_unref (guri);
		
		gconf_client_set_string (client,
	                                 FILE_BROWSER_BASE_KEY "/on_load/virtual_root",
	                                 virtual,
	                                 NULL);	
	}

	g_signal_handlers_disconnect_by_func (window, 
	                                      G_CALLBACK (on_tab_added_cb),
	                                      data);

	g_object_unref (client);
	g_free (root);
	g_free (virtual);
}

static void 
on_tab_added_cb (GeditWindow * window,
                 GeditTab * tab,
                 GeditFileBrowserPluginData * data)
{
	GConfClient * client;
	GeditDocument * doc;
	gboolean open;

	client = gconf_client_get_default ();
	
	if (!client)
		return;

	open = gconf_client_get_bool (client,
	                              FILE_BROWSER_BASE_KEY "/open_at_first_doc",
	                              NULL);

	if (open) {
		doc = gedit_tab_get_document (tab);
		
		if (!gedit_document_is_untitled (doc))
			set_root_from_doc (data, doc);
		
	}
	
	g_object_unref (client);
	
	/* Disconnect this signal, it's only called once */
	g_signal_handlers_disconnect_by_func (window, 
	                                      G_CALLBACK (on_tab_added_cb),
	                                      data);
	
}

static gboolean
on_confirm_no_trash_cb (GeditFileBrowserWidget * widget,
                        GeditFileBrowserStore * store,
                        GtkTreeIter * iter,
                        GeditWindow * window)
{
	gchar *uri;
	guint flags;
	gchar *normal;
	gchar *message;
	gchar *secondary;
	gboolean result;
	GeditFileBrowserPluginData *data;

	data = get_plugin_data (window);

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_URI, &uri,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	message = _("Cannot move file to trash, do you\nwant to delete immediately?");

	normal = gedit_file_browser_utils_uri_basename (uri);
    	secondary = g_strdup_printf(_("The file \"%s\" cannot be moved to the trash."), normal);
	g_free (normal);

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
on_confirm_delete_cb (GeditFileBrowserWidget * widget,
                      GeditFileBrowserStore * store,
                      GtkTreeIter * iter,
                      GeditWindow * window)
{
	gchar *uri;
	gchar *normal;
	gchar *message;
	gchar *secondary;
	gboolean result;
	GeditFileBrowserPluginData *data;
	
	data = get_plugin_data (window);

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_URI, &uri,
			    -1);

	normal = gedit_file_browser_utils_uri_basename  (uri);
	message =
	    g_strdup_printf (_
			     ("Are you sure you want to\npermanently delete \"%s\"?"),
			     normal);
	g_free (normal);

	secondary = _("If you delete an item it is permanently lost.");

	result = gedit_file_browser_utils_confirmation_dialog (window,
	                                                       GTK_MESSAGE_QUESTION, 
	                                                       message, 
	                                                       secondary, 
	                                                       GTK_STOCK_DELETE, 
	                                                       NULL);

	g_free (message);

	return result;
}

// ex:ts=8:noet:
