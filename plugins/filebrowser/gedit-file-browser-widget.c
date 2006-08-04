/*
 * gedit-file-browser-widget.c - Gedit plugin providing easy file access 
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

#include <sys/types.h>
#include <sys/stat.h>
#include <glib/gi18n-lib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomeui/libgnomeui.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-plugin.h>

#include "gedit-file-browser-utils.h"
#include "gedit-file-browser-error.h"
#include "gedit-file-browser-widget.h"
#include "gedit-file-browser-view.h"
#include "gedit-file-browser-store.h"
#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-marshal.h"
#include "gedit-file-browser-enum-types.h"

#define GEDIT_FILE_BROWSER_WIDGET_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BROWSER_WIDGET, GeditFileBrowserWidgetPrivate))
#define XML_UI_FILE GEDIT_FILE_BROWSER_DIR "/gedit-file-browser-widget-ui.xml"
#define LOCATION_DATA_KEY "gedit-file-browser-widget-location"

enum 
{
	BOOKMARKS_ID,
	SEPARATOR_CUSTOM_ID,
	SEPARATOR_ID,
	PATH_ID,
	NUM_DEFAULT_IDS
};

enum 
{
	COLUMN_INDENT,
	COLUMN_ICON,
	COLUMN_NAME,
	COLUMN_OBJECT,
	COLUMN_ID,
	N_COLUMNS
};

/* Properties */
enum 
{
	PROP_0,
	
	PROP_FILTER_PATTERN
};

/* Signals */
enum 
{
	URI_ACTIVATED,
	ERROR,
	CONFIRM_DELETE,
	CONFIRM_NO_TRASH,
	NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0 };

typedef struct _SignalNode 
{
	GObject *object;
	gulong id;
} SignalNode;

typedef struct
{
	gulong id;
	GeditFileBrowserWidgetFilterFunc func;
	gpointer user_data;
} FilterFunc;

typedef struct 
{
	gchar *root;
	gchar *virtual_root;
} Location;

typedef struct
{
	gchar *name;
	GdkPixbuf *icon;
} NameIcon;

struct _GeditFileBrowserWidgetPrivate 
{
	GeditFileBrowserView *treeview;
	GeditFileBrowserStore *file_store;
	GeditFileBookmarksStore *bookmarks_store;

	GHashTable *bookmarks_hash;

	GtkWidget *combo;
	GtkTreeStore *combo_model;
	GdkCursor *cursor;

	GtkWidget *filter_expander;
	GtkWidget *filter_vbox;
	GtkWidget *filter_entry;

	GtkUIManager *manager;
	GtkActionGroup *action_group;
	GtkActionGroup *action_group_selection;
	GtkActionGroup *action_group_sensitive;

	GSList *signal_pool;

	GSList *filter_funcs;
	gulong filter_id;
	gulong glob_filter_id;
	GPatternSpec *filter_pattern;
	gchar *filter_pattern_str;

	GList *locations;
	GList *current_location;
	gboolean changing_location;
	GtkWidget *location_previous_menu;
	GtkWidget *location_next_menu;
	GtkWidget *current_location_menu_item;
};

static void on_model_set                       (GObject * gobject, 
						GParamSpec * arg1,
						GeditFileBrowserWidget * obj);
static void on_treeview_error                  (GeditFileBrowserView * tree_view,
						guint code, 
						gchar * message,
						GeditFileBrowserWidget * obj);
static void on_file_store_error                (GeditFileBrowserStore * store, 
						guint code,
						gchar * message,
						GeditFileBrowserWidget * obj);
static void on_combo_changed                   (GtkComboBox * combo,
						GeditFileBrowserWidget * obj);
static gboolean on_treeview_popup_menu         (GeditFileBrowserView * treeview,
						GeditFileBrowserWidget * obj);
static gboolean on_treeview_button_press_event (GeditFileBrowserView * treeview,
						GdkEventButton * event,
						GeditFileBrowserWidget * obj);
static gboolean on_treeview_key_press_event    (GeditFileBrowserView * treeview, 
						GdkEventKey * event,
						GeditFileBrowserWidget * obj);
static void on_selection_changed               (GtkTreeSelection * selection,
						GeditFileBrowserWidget * obj);

static void on_virtual_root_changed            (GeditFileBrowserStore * model,
						GParamSpec *param,
						GeditFileBrowserWidget * obj);

static gboolean on_entry_filter_activate       (GeditFileBrowserWidget * obj);
static void on_location_jump_activate          (GtkMenuItem * item,
						GeditFileBrowserWidget * obj);
static void on_bookmarks_row_inserted          (GtkTreeModel * model, 
                                                GtkTreePath * path,
                                                GtkTreeIter * iter,
                                                GeditFileBrowserWidget * obj);
static void on_filter_mode_changed	       (GeditFileBrowserStore * model,
                                                GParamSpec * param,
                                                GeditFileBrowserWidget * obj);
static void on_action_directory_previous       (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_directory_next           (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_directory_up             (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_directory_new            (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_file_new                 (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_file_rename              (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_file_delete              (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_file_move_to_trash       (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_directory_refresh        (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_directory_open           (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_filter_hidden            (GtkAction * action,
						GeditFileBrowserWidget * obj);
static void on_action_filter_binary            (GtkAction * action,
						GeditFileBrowserWidget * obj);

GEDIT_PLUGIN_DEFINE_TYPE (GeditFileBrowserWidget, gedit_file_browser_widget,
	                  GTK_TYPE_VBOX)

static void
free_name_icon (gpointer data)
{
	NameIcon * item;
	
	if (data == NULL)
		return;

	item = (NameIcon *)(data);

	g_free (item->name);
	
	if (item->icon)
		g_object_unref (item->icon);
	
	g_free (item);
}

static FilterFunc *
filter_func_new (GeditFileBrowserWidget * obj,
		 GeditFileBrowserWidgetFilterFunc func, gpointer user_data)
{
	FilterFunc *result;

	result = g_new (FilterFunc, 1);

	result->id = ++obj->priv->filter_id;
	result->func = func;
	result->user_data = user_data;

	return result;
}

static void
location_free (Location * loc)
{
	g_free (loc->root);
	g_free (loc->virtual_root);
	g_free (loc);
}

static gboolean
combo_find_by_id (GeditFileBrowserWidget * obj, guint id,
		  GtkTreeIter * iter)
{
	guint checkid;
	GtkTreeModel *model = GTK_TREE_MODEL (obj->priv->combo_model);

	if (iter == NULL)
		return FALSE;

	if (gtk_tree_model_get_iter_first (model, iter)) {
		do {
			gtk_tree_model_get (model, iter, COLUMN_ID,
					    &checkid, -1);

			if (checkid == id)
				return TRUE;
		} while (gtk_tree_model_iter_next (model, iter));
	}

	return FALSE;
}

static void
remove_path_items (GeditFileBrowserWidget * obj)
{
	GtkTreeIter iter;
	gchar *uri;

	while (combo_find_by_id (obj, PATH_ID, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL
				    (obj->priv->combo_model), &iter,
				    COLUMN_OBJECT, &uri, -1);
		g_free (uri);

		gtk_tree_store_remove (obj->priv->combo_model, &iter);
	}
}

static void
gedit_file_browser_widget_finalize (GObject * object)
{
	GeditFileBrowserWidget *obj = GEDIT_FILE_BROWSER_WIDGET (object);
	GSList *item;
	GList *loc;

	remove_path_items (obj);

	gdk_cursor_unref (obj->priv->cursor);

	gedit_file_browser_store_set_filter_func (obj->priv->file_store,
						  NULL, NULL);

	g_object_unref (obj->priv->manager);
	g_object_unref (obj->priv->file_store);
	g_object_unref (obj->priv->bookmarks_store);
	g_object_unref (obj->priv->combo_model);

	for (item = obj->priv->filter_funcs; item; item = item->next)
		g_free (item->data);

	g_slist_free (obj->priv->filter_funcs);

	for (loc = obj->priv->locations; loc; loc = loc->next)
		location_free ((Location *) (loc->data));

	if (obj->priv->current_location_menu_item)
		gtk_widget_unref (obj->priv->current_location_menu_item);

	g_list_free (obj->priv->locations);

	g_hash_table_destroy (obj->priv->bookmarks_hash);

	G_OBJECT_CLASS (gedit_file_browser_widget_parent_class)->
	    finalize (object);
}

static void
gedit_file_browser_widget_get_property (GObject    *object,
			               guint       prop_id,
			               GValue     *value,
			               GParamSpec *pspec)
{
	GeditFileBrowserWidget *obj = GEDIT_FILE_BROWSER_WIDGET (object);

	switch (prop_id)
	{
		case PROP_FILTER_PATTERN:
			g_value_set_string (value, obj->priv->filter_pattern_str);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_widget_set_property (GObject      *object,
			               guint         prop_id,
			               const GValue *value,
			               GParamSpec   *pspec)
{
	GeditFileBrowserWidget *obj = GEDIT_FILE_BROWSER_WIDGET (object);

	switch (prop_id)
	{
		case PROP_FILTER_PATTERN:
			gedit_file_browser_widget_set_filter_pattern (obj,
			                                              g_value_get_string (value));
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_widget_class_init (GeditFileBrowserWidgetClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_file_browser_widget_finalize;

	object_class->get_property = gedit_file_browser_widget_get_property;
	object_class->set_property = gedit_file_browser_widget_set_property;

	g_object_class_install_property (object_class, PROP_FILTER_PATTERN,
					 g_param_spec_string ("filter-pattern",
					 		      "Filter Pattern",
					 		      "The filter pattern",
					 		      NULL,
					 		      G_PARAM_READWRITE));

	signals[URI_ACTIVATED] =
	    g_signal_new ("uri-activated",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass,
					   uri_activated), NULL, NULL,
			  g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1,
			  GTK_TYPE_STRING);
	signals[ERROR] =
	    g_signal_new ("error", G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass,
					   error), NULL, NULL,
			  gedit_file_browser_marshal_VOID__UINT_STRING,
			  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

	signals[CONFIRM_DELETE] =
	    g_signal_new ("confirm-delete", G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass,
	                                   confirm_delete),
	                  g_signal_accumulator_true_handled,
	                  NULL,
	                  gedit_file_browser_marshal_BOOL__OBJECT_BOXED,
	                  G_TYPE_BOOLEAN,
	                  2,
	                  G_TYPE_OBJECT,
	                  GTK_TYPE_TREE_ITER);

	signals[CONFIRM_NO_TRASH] =
	    g_signal_new ("confirm-no-trash", G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass,
	                                   confirm_no_trash),
	                  g_signal_accumulator_true_handled,
	                  NULL,
	                  gedit_file_browser_marshal_BOOL__OBJECT_BOXED,
	                  G_TYPE_BOOLEAN,
	                  2,
	                  G_TYPE_OBJECT,
	                  GTK_TYPE_TREE_ITER);

	g_type_class_add_private (object_class,
				  sizeof (GeditFileBrowserWidgetPrivate));
}

static void
add_signal (GeditFileBrowserWidget * obj, gpointer object, gulong id)
{
	SignalNode *node = g_new (SignalNode, 1);

	node->object = G_OBJECT (object);
	node->id = id;

	obj->priv->signal_pool =
	    g_slist_prepend (obj->priv->signal_pool, node);
}

/*static void
remove_signal_by_func(GeditFileBrowserWidget *obj, gpointer object, gpointer func, 
		gpointer data) {
	gulong id;
	GSList *item;
	SignalNode *node;

	id = g_signal_handler_find(object, 
			G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
			0, 0, NULL, func, data);
	
	if (id != 0) {
		for (item = obj->priv->signal_pool; item; item = item->next) {
			node = (SignalNode *)(item->data);
			
			if (node->id == id) {
				g_signal_handler_disconnect(node->object, node->id);

				obj->priv->signal_pool = g_slist_remove_link(
						obj->priv->signal_pool, item);
				g_free(node);
				break;
			}
		}
	}
}*/

static void
clear_signals (GeditFileBrowserWidget * obj)
{
	GSList *item;
	SignalNode *node;

	for (item = obj->priv->signal_pool; item; item = item->next) {
		node = (SignalNode *) (item->data);

		g_signal_handler_disconnect (node->object, node->id);
		g_free (item->data);
	}

	g_slist_free (obj->priv->signal_pool);
	obj->priv->signal_pool = NULL;
}

static gboolean
separator_func (GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	guint id;

	gtk_tree_model_get (model, iter, COLUMN_ID, &id, -1);

	return (id == SEPARATOR_ID);
}

static gboolean
get_from_bookmark_uri (GeditFileBrowserWidget * obj, GnomeVFSURI * guri,
		       gchar ** name, GdkPixbuf ** icon)
{
	gpointer data;
	NameIcon * item;

	data = g_hash_table_lookup (obj->priv->bookmarks_hash, GUINT_TO_POINTER (gnome_vfs_uri_hash (guri)));
	
	if (data == NULL)
		return FALSE;
	
	item = (NameIcon *)data;
	*name = item->name;
	*icon = item->icon;

	return TRUE;
}

static gboolean
get_from_bookmark (GeditFileBrowserWidget * obj, gchar const *uri,
		   gchar ** name, GdkPixbuf ** icon)
{
	GnomeVFSURI *guri;
	gboolean result;

	guri = gnome_vfs_uri_new (uri);

	if (guri == NULL)
		return FALSE;
	
	result = get_from_bookmark_uri (obj, guri, name, icon);
	gnome_vfs_uri_unref (guri);
	
	return result;
}

static void
insert_path_item (GeditFileBrowserWidget * obj, 
                  GnomeVFSURI * uri,
		  GtkTreeIter * after, 
		  GtkTreeIter * iter, 
		  guint indent)
{
	gchar *str2;
	gchar *unescape;
	gchar *str;
	GdkPixbuf * icon = NULL;

	if (!get_from_bookmark_uri (obj, uri, &unescape, &icon)) {
		str = gnome_vfs_uri_to_string (uri, 
	        	                       GNOME_VFS_URI_HIDE_PASSWORD |
	                	               GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD | 
	                        	       GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER);
		unescape = gedit_file_browser_utils_uri_basename (str);
		g_free (str);
		
		/* Get the icon */
		str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
		str2 = gnome_vfs_get_mime_type (str);
		icon = gedit_file_browser_utils_pixbuf_from_mime_type (str, 
		                                                       str2, 
		                                                       GTK_ICON_SIZE_MENU);

		g_free (str);
		g_free (str2);
	} else {
		unescape = g_strdup (unescape);
	}
	
	gtk_tree_store_insert_after (obj->priv->combo_model, iter, NULL,
				     after);
	gtk_tree_store_set (obj->priv->combo_model, 
	                    iter, 
	                    COLUMN_INDENT, indent,
	                    COLUMN_ICON, icon, 
	                    COLUMN_NAME, unescape, 
	                    COLUMN_OBJECT, gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
			    COLUMN_ID, PATH_ID, 
			    -1);

	g_free (unescape);
}

static void
insert_separator_item (GeditFileBrowserWidget * obj)
{
	GtkTreeIter iter;

	gtk_tree_store_insert (obj->priv->combo_model, &iter, NULL, 1);
	gtk_tree_store_set (obj->priv->combo_model, &iter,
			    COLUMN_ICON, NULL,
			    COLUMN_NAME, NULL,
			    COLUMN_ID, SEPARATOR_ID, -1);
}

static void
combo_set_active_by_id (GeditFileBrowserWidget * obj, guint id)
{
	GtkTreeIter iter;

	if (combo_find_by_id (obj, id, &iter))
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX
					       (obj->priv->combo), &iter);
}

static guint
uri_num_parents (GnomeVFSURI * from, GnomeVFSURI * to)
{
	guint parents = 0;
	GnomeVFSURI * tmp;

	if (from == NULL)
		return 0;
	
	from = gnome_vfs_uri_dup (from);
	
	while (gnome_vfs_uri_has_parent (from) && 
	       !(to && gnome_vfs_uri_equal (from, to))) {
		tmp = gnome_vfs_uri_get_parent (from);
		gnome_vfs_uri_unref (from);
		from = tmp;
		
		++parents;
	}
	
	gnome_vfs_uri_unref (from);
	
	return parents;
}

static void
insert_location_path (GeditFileBrowserWidget * obj)
{
	Location *loc;
	GnomeVFSURI *virtual;
	GnomeVFSURI *root;
	GnomeVFSURI *current = NULL;
	GnomeVFSURI * tmp;
	GtkTreeIter separator;
	GtkTreeIter iter;
	guint indent;
	
	if (!obj->priv->current_location) {
		g_message ("insert_location_path: no current location");
		return;
	}

	loc = (Location *) (obj->priv->current_location->data);

	virtual = gnome_vfs_uri_new (loc->virtual_root);
	root = gnome_vfs_uri_new (loc->root);
	current = virtual;

	combo_find_by_id (obj, SEPARATOR_ID, &separator);
	
	indent = uri_num_parents (virtual, root);

	while (current != NULL) {
		insert_path_item (obj, current, &separator, &iter, indent--);

		if (current == virtual) {
			g_signal_handlers_block_by_func (obj->priv->combo,
							 on_combo_changed,
							 obj);
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX
						       (obj->priv->combo),
						       &iter);
			g_signal_handlers_unblock_by_func (obj->priv->
							   combo,
							   on_combo_changed,
							   obj);
		}

		if (gnome_vfs_uri_equal (current, root) || !gnome_vfs_uri_has_parent (current)) {
			if (current != virtual)
				gnome_vfs_uri_unref (current);
			break;
		}

		tmp = gnome_vfs_uri_get_parent (current);
		
		if (current != virtual)
			gnome_vfs_uri_unref (current);

		current = tmp;
	}

	gnome_vfs_uri_unref (virtual);
	gnome_vfs_uri_unref (root);
}

static void
check_current_item (GeditFileBrowserWidget * obj, gboolean show_path)
{
	GtkTreeIter separator;
	gboolean has_sep;

	remove_path_items (obj);
	has_sep = combo_find_by_id (obj, SEPARATOR_ID, &separator);

	if (show_path) {
		if (!has_sep)
			insert_separator_item (obj);

		insert_location_path (obj);
	} else if (has_sep)
		gtk_tree_store_remove (obj->priv->combo_model, &separator);
}


static void
fill_combo_model (GeditFileBrowserWidget * obj)
{
	GtkTreeStore *store = obj->priv->combo_model;
	GtkTreeIter iter;

	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
			    COLUMN_ICON, gedit_file_browser_utils_pixbuf_from_theme(GTK_STOCK_HOME, GTK_ICON_SIZE_MENU),
			    COLUMN_NAME, _("Bookmarks"),
			    COLUMN_ID, BOOKMARKS_ID, -1);

	/*gtk_tree_store_append(store, &iter, NULL);
	   gtk_tree_store_set(store, &iter,
	   COLUMN_ICON, GTK_STOCK_OPEN,
	   COLUMN_NAME, "Recent",
	   COLUMN_ID, RECENT_ID, -1); */

	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX
					      (obj->priv->combo),
					      separator_func, obj, NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (obj->priv->combo), 0);
}

static void
indent_cell_data_func (GtkCellLayout * cell_layout, 
                       GtkCellRenderer * cell,
                       GtkTreeModel * model, 
                       GtkTreeIter * iter, 
                       gpointer data)
{
	gchar * indent;
	guint num;
	
	gtk_tree_model_get (model, iter, COLUMN_INDENT, &num, -1);
	
	if (num == 0)
		g_object_set (cell, "text", "", NULL);
	else {
		indent = g_strnfill (num * 2, ' ');
	
		g_object_set (cell, "text", indent, NULL);
		g_free (indent);
	}
}

static void
create_combo (GeditFileBrowserWidget * obj)
{
	GtkCellRenderer *renderer;

	obj->priv->combo_model = gtk_tree_store_new (N_COLUMNS,
						     G_TYPE_UINT,
						     GDK_TYPE_PIXBUF,
						     G_TYPE_STRING,
						     G_TYPE_POINTER,
						     G_TYPE_UINT);
	obj->priv->combo =
	    gtk_combo_box_new_with_model (GTK_TREE_MODEL
					  (obj->priv->combo_model));

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (obj->priv->combo),
	                            renderer, FALSE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT
					    (obj->priv->combo), renderer,
					    indent_cell_data_func, obj, NULL);


	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (obj->priv->combo),
				    renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (obj->priv->combo),
				       renderer, "pixbuf", COLUMN_ICON);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (obj->priv->combo),
				    renderer, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (obj->priv->combo),
				       renderer, "text", COLUMN_NAME);

	gtk_box_pack_start (GTK_BOX (obj), GTK_WIDGET (obj->priv->combo),
			    FALSE, FALSE, 0);

	fill_combo_model (obj);
	g_signal_connect (obj->priv->combo, "changed",
			  G_CALLBACK (on_combo_changed), obj);

	gtk_widget_show (obj->priv->combo);
}

static GtkActionEntry toplevel_actions[] = 
{
	{"FilterMenuAction", NULL, "_Filter"}
};

static GtkActionEntry tree_actions[] = 
{
	{"DirectoryNew", GTK_STOCK_ADD, N_("_New Directory"), NULL,
	 N_("Add new empty directory"),
	 G_CALLBACK (on_action_directory_new)},
	{"FileNew", GTK_STOCK_NEW, N_("New F_ile"), NULL,
	 N_("Add new empty file"), G_CALLBACK (on_action_file_new)},
	{"DirectoryUp", GTK_STOCK_GO_UP, N_("Up"), NULL,
	 N_("Open the parent folder"), G_CALLBACK (on_action_directory_up)}
};

static GtkActionEntry tree_actions_selection[] = 
{
	{"FileRename", NULL, N_("_Rename"), NULL,
	 N_("Rename selected file or directory"),
	 G_CALLBACK (on_action_file_rename)},
	{"FileMoveToTrash", "gnome-stock-trash", N_("_Move To Trash"), NULL,
	 N_("Move selected file or directory to trash"),
	 G_CALLBACK (on_action_file_move_to_trash)},
	{"FileDelete", GTK_STOCK_DELETE, N_("_Delete"), NULL,
	 N_("Delete selected file or directory"),
	 G_CALLBACK (on_action_file_delete)}
};

static GtkActionEntry tree_actions_sensitive[] = 
{
	{"DirectoryPrevious", GTK_STOCK_GO_BACK, N_("_Previous Location"),
	 NULL,
	 N_("Go to the previous visited location"),
	 G_CALLBACK (on_action_directory_previous)},
	{"DirectoryNext", GTK_STOCK_GO_FORWARD, N_("_Next Location"), NULL,
	 N_("Go to the next visited location"), G_CALLBACK (on_action_directory_next)},
	{"DirectoryRefresh", GTK_STOCK_REFRESH, N_("Re_fresh View"), NULL,
	 N_("Refresh the view"), G_CALLBACK (on_action_directory_refresh)},
	{"DirectoryOpen", GTK_STOCK_OPEN, N_("_View Directory"), NULL,
	 N_("View directory in file manager"),
	 G_CALLBACK (on_action_directory_open)}
};

static GtkToggleActionEntry tree_actions_toggle[] = 
{
	{"FilterHidden", GTK_STOCK_DIALOG_AUTHENTICATION,
	 N_("Show _Hidden"), NULL,
	 N_("Show hidden files and directories"),
	 G_CALLBACK (on_action_filter_hidden), FALSE},
	{"FilterBinary", NULL, N_("Show _Binary"), NULL,
	 N_("Show binary files"), G_CALLBACK (on_action_filter_binary),
	 FALSE}
};

static void
create_toolbar (GeditFileBrowserWidget * obj)
{
	GtkUIManager *manager;
	GError *error = NULL;
	GtkActionGroup *action_group;
	GtkWidget *toolbar;
	GtkWidget *widget;
	GtkAction *action;

	manager = gtk_ui_manager_new ();
	obj->priv->manager = manager;

	gtk_ui_manager_add_ui_from_file (manager, XML_UI_FILE, &error);

	if (error != NULL) {
		g_warning ("Error in adding ui from file %s: %s",
			   XML_UI_FILE, error->message);
		g_error_free (error);
		return;
	}

	action_group =
	    gtk_action_group_new ("FileBrowserWidgetActionGroupToplevel");
	gtk_action_group_add_actions (action_group, toplevel_actions,
				      G_N_ELEMENTS (toplevel_actions),
				      obj);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);

	action_group =
	    gtk_action_group_new ("FileBrowserWidgetActionGroup");
	gtk_action_group_add_actions (action_group, tree_actions,
				      G_N_ELEMENTS (tree_actions), obj);
	gtk_action_group_add_toggle_actions (action_group,
					     tree_actions_toggle,
					     G_N_ELEMENTS
					     (tree_actions_toggle), obj);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	obj->priv->action_group = action_group;

	action_group =
	    gtk_action_group_new ("FileBrowserWidgetSelectionActionGroup");
	gtk_action_group_add_actions (action_group, tree_actions_selection,
				      G_N_ELEMENTS
				      (tree_actions_selection), obj);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	obj->priv->action_group_selection = action_group;

	action_group =
	    gtk_action_group_new ("FileBrowserWidgetSensitiveActionGroup");
	gtk_action_group_add_actions (action_group, tree_actions_sensitive,
				      G_N_ELEMENTS
				      (tree_actions_sensitive), obj);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	obj->priv->action_group_sensitive = action_group;

	gtk_action_set_sensitive (gtk_action_group_get_action
				  (obj->priv->action_group_sensitive,
				   "DirectoryPrevious"), FALSE);
	gtk_action_set_sensitive (gtk_action_group_get_action
				  (obj->priv->action_group_sensitive,
				   "DirectoryNext"), FALSE);

	toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

	/* Previous directory menu tool item */
	obj->priv->location_previous_menu = gtk_menu_new ();
	gtk_widget_show (obj->priv->location_previous_menu);

	widget =
	    GTK_WIDGET (gtk_menu_tool_button_new_from_stock
			(GTK_STOCK_GO_BACK));
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (widget),
				       obj->priv->location_previous_menu);

	g_object_set (widget, "label", _("Previous location"), NULL);

	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (widget),
				   GTK_TOOLBAR (toolbar)->tooltips,
				   _("Go to previous location"), NULL);
	gtk_menu_tool_button_set_arrow_tooltip (GTK_MENU_TOOL_BUTTON
						(widget),
						GTK_TOOLBAR (toolbar)->
						tooltips,
						_
						("Go to a previously opened location"),
						NULL);

	action =
	    gtk_action_group_get_action (obj->priv->action_group_sensitive,
					 "DirectoryPrevious");
	g_object_set (action, "is_important", TRUE, "short_label",
		      _("Previous location"), NULL);
	gtk_action_connect_proxy (action, widget);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (widget),
			    0);

	/* Next directory menu tool item */
	obj->priv->location_next_menu = gtk_menu_new ();
	gtk_widget_show (obj->priv->location_next_menu);

	widget =
	    GTK_WIDGET (gtk_menu_tool_button_new_from_stock
			(GTK_STOCK_GO_FORWARD));
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (widget),
				       obj->priv->location_next_menu);

	g_object_set (widget, "label", _("Next location"), NULL);

	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (widget),
				   GTK_TOOLBAR (toolbar)->tooltips,
				   _("Go to next location"), NULL);
	gtk_menu_tool_button_set_arrow_tooltip (GTK_MENU_TOOL_BUTTON
						(widget),
						GTK_TOOLBAR (toolbar)->
						tooltips,
						_
						("Go to a previously opened location"),
						NULL);

	action =
	    gtk_action_group_get_action (obj->priv->action_group_sensitive,
					 "DirectoryNext");
	g_object_set (action, "is_important", TRUE, "short_label",
		      _("Previous location"), NULL);
	gtk_action_connect_proxy (action, widget);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (widget),
			    1);

	gtk_box_pack_start (GTK_BOX (obj), toolbar, FALSE, FALSE, 0);
	gtk_widget_show (toolbar);
}

static gboolean
filter_real (GeditFileBrowserStore * model, GtkTreeIter * iter,
	     GeditFileBrowserWidget * obj)
{
	GSList *item;
	FilterFunc *func;

	for (item = obj->priv->filter_funcs; item; item = item->next) {
		func = (FilterFunc *) (item->data);

		if (!func->func (obj, model, iter, func->user_data))
			return FALSE;
	}

	return TRUE;
}

static void
add_bookmark_hash (GeditFileBrowserWidget * obj,
                   GtkTreeIter * iter)
{
	GtkTreeModel *model;
	GdkPixbuf * pixbuf;
	gchar * name;
	gchar * uri;
	GnomeVFSURI * guri;
	NameIcon * item;

	model = GTK_TREE_MODEL (obj->priv->bookmarks_store);

	uri = gedit_file_bookmarks_store_get_uri (obj->priv->
					    	          bookmarks_store,
							  iter);
	
	if (uri == NULL)
		return;
	
	guri = gnome_vfs_uri_new (uri);
	g_free (uri);
	
	if (guri == NULL)
		return;

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_ICON,
			    &pixbuf,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_NAME,
			    &name, -1);	
	
	item = g_new (NameIcon, 1);
	item->name = name;
	item->icon = pixbuf;

	g_hash_table_insert (obj->priv->bookmarks_hash, GUINT_TO_POINTER (gnome_vfs_uri_hash (guri)), item);
	gnome_vfs_uri_unref (guri);
}

static void
init_bookmarks_hash (GeditFileBrowserWidget * obj)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (obj->priv->bookmarks_store);
	
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;
	
	do {
		add_bookmark_hash (obj, &iter);
	} while (gtk_tree_model_iter_next (model, &iter));
	
	g_signal_connect (obj->priv->bookmarks_store,
		          "row-inserted",
		          G_CALLBACK (on_bookmarks_row_inserted),
		          obj);
}

static void
create_tree (GeditFileBrowserWidget * obj)
{
	GtkWidget *sw;

	obj->priv->file_store = gedit_file_browser_store_new (NULL);
	obj->priv->bookmarks_store = gedit_file_bookmarks_store_new ();
	obj->priv->treeview =
	    GEDIT_FILE_BROWSER_VIEW (gedit_file_browser_view_new ());
	obj->priv->cursor = gdk_cursor_new (GDK_WATCH);

	gedit_file_browser_store_set_filter_mode (obj->priv->file_store,
						  GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN
						  |
						  GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY);
	gedit_file_browser_store_set_filter_func (obj->priv->file_store,
						  (GeditFileBrowserStoreFilterFunc)
						  filter_real, obj);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_ALWAYS);

	gtk_container_add (GTK_CONTAINER (sw),
			   GTK_WIDGET (obj->priv->treeview));
	gtk_box_pack_start (GTK_BOX (obj), sw, TRUE, TRUE, 0);

	g_signal_connect (obj->priv->treeview, "notify::model",
			  G_CALLBACK (on_model_set), obj);
	g_signal_connect (obj->priv->treeview, "error",
			  G_CALLBACK (on_treeview_error), obj);
	g_signal_connect (obj->priv->treeview, "popup-menu",
			  G_CALLBACK (on_treeview_popup_menu), obj);
	g_signal_connect (obj->priv->treeview, "button-press-event",
			  G_CALLBACK (on_treeview_button_press_event),
			  obj);
	g_signal_connect (obj->priv->treeview, "key-press-event",
			  G_CALLBACK (on_treeview_key_press_event), obj);

	g_signal_connect (gtk_tree_view_get_selection
			  (GTK_TREE_VIEW (obj->priv->treeview)), "changed",
			  G_CALLBACK (on_selection_changed), obj);
	g_signal_connect (obj->priv->file_store, "notify::filter-mode",
			  G_CALLBACK (on_filter_mode_changed), obj);

	init_bookmarks_hash (obj);

	gtk_widget_show (sw);
	gtk_widget_show (GTK_WIDGET (obj->priv->treeview));
}

static void
create_filter (GeditFileBrowserWidget * obj)
{
	GtkWidget *expander;
	GtkWidget *vbox;
	GtkWidget *entry;

	expander = gtk_expander_new (_("Advanced filtering"));
	gtk_widget_show (expander);
	gtk_box_pack_start (GTK_BOX (obj), expander, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_widget_show (vbox);

	obj->priv->filter_expander = expander;
	obj->priv->filter_vbox = vbox;

	entry = gtk_entry_new ();
	gtk_widget_show (entry);

	obj->priv->filter_entry = entry;

	g_signal_connect_swapped (entry, "activate",
				  G_CALLBACK (on_entry_filter_activate),
				  obj);
	g_signal_connect_swapped (entry, "focus_out_event",
				  G_CALLBACK (on_entry_filter_activate),
				  obj);

	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (expander), vbox);
}

static guint
uint_hash (gconstpointer key)
{
	return GPOINTER_TO_UINT (key);
}

static gboolean
uint_equal (gconstpointer a, gconstpointer b)
{
	return GPOINTER_TO_UINT (a) == GPOINTER_TO_UINT (b);
}

static void
gedit_file_browser_widget_init (GeditFileBrowserWidget * obj)
{
	obj->priv = GEDIT_FILE_BROWSER_WIDGET_GET_PRIVATE (obj);

	obj->priv->bookmarks_hash = g_hash_table_new_full (uint_hash,
			                                   uint_equal,
			                                   NULL,
			                                   free_name_icon);

	create_toolbar (obj);
	create_combo (obj);
	create_tree (obj);
	create_filter (obj);

	gtk_box_set_spacing (GTK_BOX (obj), 3);

	gedit_file_browser_widget_show_bookmarks (obj);
}

/* Private */

static void
update_sensitivity (GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkAction *action;
	gint mode;

	if (GEDIT_IS_FILE_BROWSER_STORE (model)) {
		gtk_action_group_set_sensitive (obj->priv->action_group,
						TRUE);
		mode =
		    gedit_file_browser_store_get_filter_mode
		    (GEDIT_FILE_BROWSER_STORE (model));

		action =
		    gtk_action_group_get_action (obj->priv->action_group,
						 "FilterHidden");
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
					      !(mode &
						GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN));
	} else if (GEDIT_IS_FILE_BOOKMARKS_STORE (model)) {
		gtk_action_group_set_sensitive (obj->priv->action_group,
						FALSE);
		gtk_action_group_set_sensitive (obj->priv->action_group,
						FALSE);

		/* Set the filter toggle to normal up state, just for visual pleasure */
		action =
		    gtk_action_group_get_action (obj->priv->action_group,
						 "FilterHidden");
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
					      FALSE);
	}

	on_selection_changed (gtk_tree_view_get_selection
			      (GTK_TREE_VIEW (obj->priv->treeview)), obj);
}

gboolean
gedit_file_browser_widget_get_selected_directory (GeditFileBrowserWidget * obj, GtkTreeIter * iter)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeSelection *selection;
	GtkTreeIter parent;
	GtkTreeModel *selmodel;
	guint flags;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return FALSE;

	selection =
	    gtk_tree_view_get_selection (GTK_TREE_VIEW
					 (obj->priv->treeview));

	if (!gtk_tree_selection_get_selected (selection, &selmodel, iter)) {
		gedit_file_browser_store_get_iter_virtual_root
		    (GEDIT_FILE_BROWSER_STORE (model), iter);
		selmodel = model;
	}

	gtk_tree_model_get (selmodel, iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (!FILE_IS_DIR (flags)) {
		/* Get the parent, because the selection is a file */
		gtk_tree_model_iter_parent (selmodel, &parent, iter);
		*iter = parent;
	}

	return TRUE;
}

static gboolean
popup_menu (GeditFileBrowserWidget * obj, GdkEventButton * event)
{
	GtkWidget *menu;

	menu = gtk_ui_manager_get_widget (obj->priv->manager, "/Popup");
	g_return_val_if_fail (menu != NULL, FALSE);

	if (event != NULL) {
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				event->button, event->time);
	} else {
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
				gedit_utils_menu_position_under_tree_view,
				obj->priv->treeview, 0,
				gtk_get_current_event_time ());
		gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
	}

	return TRUE;
}

static gboolean
filter_glob (GeditFileBrowserWidget * obj, GeditFileBrowserStore * store,
	     GtkTreeIter * iter, gpointer user_data)
{
	gchar *name;
	gboolean result;
	guint flags;

	if (obj->priv->filter_pattern == NULL)
		return TRUE;

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_NAME, &name,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (FILE_IS_DIR (flags) || FILE_IS_DUMMY (flags))
		result = TRUE;
	else
		result =
		    g_pattern_match_string (obj->priv->filter_pattern,
					    name);

	g_free (name);

	return result;
}

static gboolean
delete_selected_file (GeditFileBrowserWidget * obj, gboolean trash)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *selmodel;
	guint flags;
	gboolean confirm;
	GeditFileBrowserStoreResult result;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return FALSE;

	selection =
	    gtk_tree_view_get_selection (GTK_TREE_VIEW
					 (obj->priv->treeview));

	if (!gtk_tree_selection_get_selected (selection, &selmodel, &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (FILE_IS_DUMMY (flags))
		return FALSE;

	if (!trash) {
		g_signal_emit (obj, signals[CONFIRM_DELETE], 0, model, &iter, &confirm);

		if (!confirm)
			return FALSE;
	}

	result = gedit_file_browser_store_delete (GEDIT_FILE_BROWSER_STORE (model),
					          &iter, trash);

	if (result == GEDIT_FILE_BROWSER_STORE_RESULT_NO_TRASH) {
		g_signal_emit (obj, signals[CONFIRM_NO_TRASH], 0, model, &iter, &confirm);
		
		if (confirm)
			result = gedit_file_browser_store_delete (GEDIT_FILE_BROWSER_STORE (model),
					                          &iter, FALSE);
	}

	return result == GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

static GnomeVFSURI *
get_topmost_uri (GnomeVFSURI const *uri)
{
	GnomeVFSURI * tmp;
	GnomeVFSURI * current;
	
	current = gnome_vfs_uri_dup (uri);
	
	while (gnome_vfs_uri_has_parent (current)) {
		tmp = gnome_vfs_uri_get_parent (current);
		gnome_vfs_uri_unref (current);
		current = tmp;
	}
	
	return current;
}

static GtkWidget *
create_goto_menu_item (GeditFileBrowserWidget * obj, GList * item,
		       GdkPixbuf * pixbuf)
{
	GtkWidget *result;
	GtkWidget *image;
	gchar *base;
	gchar *unescape;
	Location *loc;

	loc = (Location *) (item->data);

	if (!get_from_bookmark
	    (obj, loc->virtual_root, &unescape, &pixbuf)) {
		if (gedit_file_browser_utils_is_local (loc->virtual_root)) {
			unescape =
			    gnome_vfs_get_local_path_from_uri (loc->
							       virtual_root);
			base = g_path_get_basename (unescape);
			g_free (unescape);
		} else {
			base = g_path_get_basename (loc->virtual_root);
		}

		unescape = gnome_vfs_unescape_string_for_display (base);
		g_free (base);
	} else {
		unescape = g_strdup (unescape);
	}

	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_widget_show (image);

	result = gtk_image_menu_item_new_with_label (unescape);
	g_object_set_data (G_OBJECT (result), LOCATION_DATA_KEY, item);
	g_signal_connect (result, "activate",
			  G_CALLBACK (on_location_jump_activate), obj);

	gtk_widget_show (result);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (result),
				       image);

	g_free (unescape);

	return result;
}

static GList *
list_next_iterator (GList * list)
{
	if (!list)
		return NULL;

	return list->next;
}

static GList *
list_prev_iterator (GList * list)
{
	if (!list)
		return NULL;

	return list->prev;
}

static void
jump_to_location (GeditFileBrowserWidget * obj, GList * item,
		  gboolean previous)
{
	Location *loc;
	GtkWidget *widget;
	GList *children;
	GList *child;
	GList *(*iter_func) (GList *);
	GtkWidget *menu_from;
	GtkWidget *menu_to;

	if (!obj->priv->locations)
		return;

	if (previous) {
		iter_func = list_next_iterator;
		menu_from = obj->priv->location_previous_menu;
		menu_to = obj->priv->location_next_menu;
	} else {
		iter_func = list_prev_iterator;
		menu_from = obj->priv->location_next_menu;
		menu_to = obj->priv->location_previous_menu;
	}

	children = gtk_container_get_children (GTK_CONTAINER (menu_from));
	child = children;

	/* This is the menuitem for the current location, which is the first
	   to be added to the menu */
	widget = obj->priv->current_location_menu_item;

	while (obj->priv->current_location != item) {
		if (widget) {
			/* Prepend the menu item to the menu */
			gtk_menu_shell_prepend (GTK_MENU_SHELL (menu_to),
						widget);

			gtk_widget_unref (widget);
		}

		widget = GTK_WIDGET (child->data);

		/* Make sure the widget isn't destroyed when removed */
		gtk_widget_ref (widget);
		gtk_container_remove (GTK_CONTAINER (menu_from), widget);

		obj->priv->current_location_menu_item = widget;

		if (obj->priv->current_location == NULL) {
			obj->priv->current_location = obj->priv->locations;

			if (obj->priv->current_location == item)
				break;
		} else {
			obj->priv->current_location =
			    iter_func (obj->priv->current_location);
		}

		child = child->next;
	}

	g_list_free (children);

	obj->priv->changing_location = TRUE;

	loc = (Location *) (obj->priv->current_location->data);

	/* Set the new root + virtual root */
	gedit_file_browser_widget_set_root_and_virtual_root (obj,
							     loc->root,
							     loc->
							     virtual_root);

	obj->priv->changing_location = FALSE;
}

static void
clear_next_locations (GeditFileBrowserWidget * obj)
{
	GList *children;
	GList *item;

	if (obj->priv->current_location == NULL)
		return;

	while (obj->priv->current_location->prev) {
		location_free ((Location *) (obj->priv->current_location->
					     prev->data));
		obj->priv->locations =
		    g_list_remove_link (obj->priv->locations,
					obj->priv->current_location->prev);
	}

	children =
	    gtk_container_get_children (GTK_CONTAINER
					(obj->priv->location_next_menu));

	for (item = children; item; item = item->next) {
		gtk_container_remove (GTK_CONTAINER
				      (obj->priv->location_next_menu),
				      GTK_WIDGET (item->data));
	}

	g_list_free (children);

	gtk_action_set_sensitive (gtk_action_group_get_action
				  (obj->priv->action_group_sensitive,
				   "DirectoryNext"), FALSE);
}

static void
update_filter_mode (GeditFileBrowserWidget * obj, 
                    GtkAction * action, 
                    GeditFileBrowserStoreFilterMode mode)
{
	gboolean active =
	    gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	gint now;

	if (GEDIT_IS_FILE_BROWSER_STORE (model)) {
		now =
		    gedit_file_browser_store_get_filter_mode
		    (GEDIT_FILE_BROWSER_STORE (model));

		if (active)
			now &= ~mode;
		else
			now |= mode;

		gedit_file_browser_store_set_filter_mode
		    (GEDIT_FILE_BROWSER_STORE (model), now);
	}
}

static void
set_filter_pattern_real (GeditFileBrowserWidget * obj,
                        gchar const * pattern,
                        gboolean update_entry)
{
	GtkTreeModel *model;

	model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	if (pattern != NULL && *pattern == '\0')
		pattern = NULL;

	if (pattern == NULL && obj->priv->filter_pattern_str == NULL)
		return;
	
	if (pattern != NULL && obj->priv->filter_pattern_str != NULL && 
	    strcmp (pattern, obj->priv->filter_pattern_str) == 0)
		return;

	/* Free the old pattern */
	g_free (obj->priv->filter_pattern_str);
	obj->priv->filter_pattern_str = g_strdup (pattern);

	if (obj->priv->filter_pattern) {
		g_pattern_spec_free (obj->priv->filter_pattern);
		obj->priv->filter_pattern = NULL;
	}

	if (pattern == NULL) {
		if (obj->priv->glob_filter_id != 0) {
			gedit_file_browser_widget_remove_filter (obj,
								 obj->
								 priv->
								 glob_filter_id);
			obj->priv->glob_filter_id = 0;
		}
	} else {
		obj->priv->filter_pattern = g_pattern_spec_new (pattern);

		if (obj->priv->glob_filter_id == 0)
			obj->priv->glob_filter_id =
			    gedit_file_browser_widget_add_filter (obj,
								  filter_glob,
								  NULL);
	}

	if (update_entry) {
		if (obj->priv->filter_pattern_str == NULL)
			gtk_entry_set_text (GTK_ENTRY (obj->priv->filter_entry),
			                    "");
		else {
			gtk_entry_set_text (GTK_ENTRY (obj->priv->filter_entry),
			                    obj->priv->filter_pattern_str);

			gtk_expander_set_expanded (GTK_EXPANDER (obj->priv->filter_expander),
		        	                   TRUE);
		}
	}

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		gedit_file_browser_store_refilter (GEDIT_FILE_BROWSER_STORE
						   (model));

	g_object_notify (G_OBJECT (obj), "filter-pattern");
}


/* Public */

GtkWidget *
gedit_file_browser_widget_new ()
{
	GeditFileBrowserWidget *obj =
	    g_object_new (GEDIT_TYPE_FILE_BROWSER_WIDGET, NULL);

	return GTK_WIDGET (obj);
}

void
gedit_file_browser_widget_show_bookmarks (GeditFileBrowserWidget * obj)
{
	/* Select bookmarks in the combo box */
	g_signal_handlers_block_by_func (obj->priv->combo,
					 on_combo_changed, obj);
	combo_set_active_by_id (obj, BOOKMARKS_ID);
	g_signal_handlers_unblock_by_func (obj->priv->combo,
					   on_combo_changed, obj);

	check_current_item (obj, FALSE);

	gedit_file_browser_view_set_model (obj->priv->treeview,
					   GTK_TREE_MODEL (obj->priv->
							   bookmarks_store));
}

void
gedit_file_browser_widget_set_root_and_virtual_root (GeditFileBrowserWidget
						     * obj,
						     gchar const *root,
						     gchar const *virtual)
{
	GeditFileBrowserStoreResult result;

	gedit_file_browser_view_set_model (obj->priv->treeview,
					   GTK_TREE_MODEL (obj->priv->
							   file_store));

	if (!virtual)
		result =
		    gedit_file_browser_store_set_root_and_virtual_root 
		    (obj->priv->file_store, root, root);
	else
		result =
		    gedit_file_browser_store_set_root_and_virtual_root
		    (obj->priv->file_store, root, virtual);

	if (result == GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE)
		on_virtual_root_changed (obj->priv->file_store, NULL, obj);
}

void
gedit_file_browser_widget_set_root (GeditFileBrowserWidget * obj,
				    gchar const *root, 
				    gboolean virtual)
{
	GnomeVFSURI *uri;
	GnomeVFSURI *parent;
	gchar *str;

	if (!virtual) {
		gedit_file_browser_widget_set_root_and_virtual_root (obj,
								     root,
								     NULL);
	} else {
		uri = gnome_vfs_uri_new (root);

		if (uri) {
			parent = get_topmost_uri (uri);
			str =
			    gnome_vfs_uri_to_string (parent,
						     GNOME_VFS_URI_HIDE_NONE);

			gedit_file_browser_widget_set_root_and_virtual_root
			    (obj, str, root);

			g_free (str);
			gnome_vfs_uri_unref (uri);
			gnome_vfs_uri_unref (parent);
		} else {
			str =
			    g_strconcat (_("Invalid uri"), ": ", root,
					 NULL);
			g_signal_emit (obj, signals[ERROR], 0,
				       GEDIT_FILE_BROWSER_ERROR_SET_ROOT,
				       str);
			g_free (str);
		}
	}
}

GeditFileBrowserStore *
gedit_file_browser_widget_get_browser_store (GeditFileBrowserWidget * obj)
{
	return obj->priv->file_store;
}

GeditFileBookmarksStore *
gedit_file_browser_widget_get_bookmarks_store (GeditFileBrowserWidget * obj)
{
	return obj->priv->bookmarks_store;
}

GeditFileBrowserView *
gedit_file_browser_widget_get_browser_view (GeditFileBrowserWidget * obj)
{
	return obj->priv->treeview;
}

GtkUIManager *
gedit_file_browser_widget_get_ui_manager (GeditFileBrowserWidget * obj)
{
	return obj->priv->manager;
}

GtkWidget *
gedit_file_browser_widget_get_filter_entry (GeditFileBrowserWidget * obj)
{
	return obj->priv->filter_entry;
}


gulong
gedit_file_browser_widget_add_filter (GeditFileBrowserWidget * obj,
				      GeditFileBrowserWidgetFilterFunc
				      func, gpointer user_data)
{
	FilterFunc *f;
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	f = filter_func_new (obj, func, user_data);
	obj->priv->filter_funcs =
	    g_slist_append (obj->priv->filter_funcs, f);

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		gedit_file_browser_store_refilter (GEDIT_FILE_BROWSER_STORE
						   (model));

	return f->id;
}

void
gedit_file_browser_widget_remove_filter (GeditFileBrowserWidget * obj,
					 gulong id)
{
	GSList *item;
	FilterFunc *func;

	for (item = obj->priv->filter_funcs; item; item = item->next) {
		func = (FilterFunc *) (item->data);

		if (func->id == id) {
			obj->priv->filter_funcs =
			    g_slist_remove_link (obj->priv->filter_funcs,
						 item);
			g_free (func);
			break;
		}
	}
}

GtkVBox *
gedit_file_browser_widget_get_filter_vbox (GeditFileBrowserWidget * obj)
{
	return GTK_VBOX (obj->priv->filter_vbox);
}

void 
gedit_file_browser_widget_set_filter_pattern (GeditFileBrowserWidget * obj,
                                              gchar const *pattern)
{
	set_filter_pattern_real (obj, pattern, TRUE);
}

/* Callbacks */
static void
on_row_activated (GtkTreeView * treeview, GtkTreePath * path,
		  GtkTreeViewColumn * column, GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	gchar *uri;
	GtkTreeIter iter;
	gint flags;

	if (GEDIT_IS_FILE_BOOKMARKS_STORE (model)) {
		gtk_tree_model_get_iter (model, &iter, path);
		uri =
		    gedit_file_bookmarks_store_get_uri
		    (GEDIT_FILE_BOOKMARKS_STORE (model), &iter);

		gtk_tree_model_get (model, &iter,
				    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
				    &flags, -1);

		if (uri) {
			if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT) {
				gedit_file_browser_widget_set_root (obj,
								    uri,
								    FALSE);
			} else {
				gedit_file_browser_widget_set_root (obj,
								    uri,
								    TRUE);
			}
		} else {
			g_warning ("No uri!");
		}

		g_free (uri);
	} else if (GEDIT_IS_FILE_BROWSER_STORE (model)) {
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			gtk_tree_model_get (model, &iter,
					    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS,
					    &flags,
					    GEDIT_FILE_BROWSER_STORE_COLUMN_URI,
					    &uri, -1);

			if (!FILE_IS_DIR (flags) && !FILE_IS_DUMMY (flags))
				g_signal_emit (obj, signals[URI_ACTIVATED],
					       0, uri);

			g_free (uri);
		}
	}
}

static void
on_begin_loading (GeditFileBrowserStore * model, GtkTreeIter * iter,
		  GeditFileBrowserWidget * obj)
{
	if (GDK_IS_WINDOW (GTK_WIDGET (obj->priv->treeview)->window))
		gdk_window_set_cursor (GTK_WIDGET (obj->priv->treeview)->window,
				       obj->priv->cursor);
}

static void
on_end_loading (GeditFileBrowserStore * model, GtkTreeIter * iter,
		GeditFileBrowserWidget * obj)
{
	if (GDK_IS_WINDOW (GTK_WIDGET (obj->priv->treeview)->window))
		gdk_window_set_cursor (GTK_WIDGET (obj->priv->treeview)->window,
				       NULL);
}

static gboolean
virtual_root_is_root (GeditFileBrowserWidget * obj, 
                      GeditFileBrowserStore  * model)
{
	GtkTreeIter root;
	GtkTreeIter virtual;
	
	if (!gedit_file_browser_store_get_iter_root (model, &root))
		return TRUE;
	
	if (!gedit_file_browser_store_get_iter_virtual_root (model, &virtual))
		return TRUE;
	
	return gedit_file_browser_store_iter_equal (model, &root, &virtual);
}

static void
on_virtual_root_changed (GeditFileBrowserStore * model,
			 GParamSpec * param,
			 GeditFileBrowserWidget * obj)
{
	GtkTreeIter iter;
	gchar *uri;
	gchar *root_uri;
	GtkTreeIter root;
	GtkAction *action;
	Location *loc;
	GdkPixbuf *pixbuf;

	if (gedit_file_browser_store_get_iter_virtual_root (model, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_URI,
				    &uri, -1);

		if (gedit_file_browser_store_get_iter_root (model, &root)) {
			if (!obj->priv->changing_location) {
				/* Remove all items from obj->priv->current_location on */
				if (obj->priv->current_location)
					clear_next_locations (obj);

				root_uri =
				    gedit_file_browser_store_get_root
				    (model);

				loc = g_new (Location, 1);
				loc->root = root_uri;
				loc->virtual_root = g_strdup (uri);

				if (obj->priv->current_location) {
					/* Add current location to the menu so we can go back
					   to it later */
					gtk_menu_shell_prepend
					    (GTK_MENU_SHELL
					     (obj->priv->
					      location_previous_menu),
					     obj->priv->
					     current_location_menu_item);
				}

				obj->priv->locations =
				    g_list_prepend (obj->priv->locations,
						    loc);

				gtk_tree_model_get (GTK_TREE_MODEL (model),
						    &iter,
						    GEDIT_FILE_BROWSER_STORE_COLUMN_ICON,
						    &pixbuf, -1);

				obj->priv->current_location =
				    obj->priv->locations;
				obj->priv->current_location_menu_item =
				    create_goto_menu_item (obj,
							   obj->priv->
							   current_location,
							   pixbuf);

				g_object_ref_sink (obj->priv->
						   current_location_menu_item);

				if (pixbuf)
					g_object_unref (pixbuf);

			}
			
			action =
			    gtk_action_group_get_action (obj->priv->
			                                 action_group,
			                                 "DirectoryUp");
			gtk_action_set_sensitive (action, 
			                          !virtual_root_is_root (obj, model));

			action =
			    gtk_action_group_get_action (obj->priv->
							 action_group_sensitive,
							 "DirectoryPrevious");
			gtk_action_set_sensitive (action,
						  obj->priv->
						  current_location != NULL
						  && obj->priv->
						  current_location->next !=
						  NULL);

			action =
			    gtk_action_group_get_action (obj->priv->
							 action_group_sensitive,
							 "DirectoryNext");
			gtk_action_set_sensitive (action,
						  obj->priv->
						  current_location != NULL
						  && obj->priv->
						  current_location->prev !=
						  NULL);
		}

		check_current_item (obj, TRUE);
		g_free (uri);
	} else {
		g_message ("NO!");
	}
}

static void
on_model_set (GObject * gobject, GParamSpec * arg1,
	      GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (gobject));

	clear_signals (obj);

	add_signal (obj, gobject,
		    g_signal_connect (gobject,
				      "row-activated",
				      G_CALLBACK
				      (on_row_activated),
				      obj));

	if (GEDIT_IS_FILE_BOOKMARKS_STORE (model)) {
		clear_next_locations (obj);

		/* Add the current location to the back menu */
		if (obj->priv->current_location) {
			gtk_menu_shell_prepend (GTK_MENU_SHELL
						(obj->priv->
						 location_previous_menu),
						obj->priv->
						current_location_menu_item);

			gtk_widget_unref (obj->priv->
					  current_location_menu_item);
			obj->priv->current_location = NULL;
			obj->priv->current_location_menu_item = NULL;

			gtk_action_set_sensitive
			    (gtk_action_group_get_action
			     (obj->priv->action_group_sensitive,
			      "DirectoryPrevious"), TRUE);
		}

		gtk_widget_set_sensitive (obj->priv->filter_vbox, FALSE);
	} else if (GEDIT_IS_FILE_BROWSER_STORE (model)) {
		add_signal (obj, model,
			    g_signal_connect (model, "begin-loading",
					      G_CALLBACK
					      (on_begin_loading), obj));
		add_signal (obj, model,
			    g_signal_connect (model, "end-loading",
					      G_CALLBACK (on_end_loading),
					      obj));
		add_signal (obj, model,
			    g_signal_connect (model,
					      "notify::virtual-root",
					      G_CALLBACK
					      (on_virtual_root_changed),
					      obj));
		add_signal (obj, model,
			    g_signal_connect (model, "error",
					      G_CALLBACK
					      (on_file_store_error), obj));
		gtk_widget_set_sensitive (obj->priv->filter_vbox, TRUE);
	}

	update_sensitivity (obj);
}

static void
on_file_store_error (GeditFileBrowserStore * store, guint code,
		     gchar * message, GeditFileBrowserWidget * obj)
{
	g_signal_emit (obj, signals[ERROR], 0, code, message);
}

static void
on_treeview_error (GeditFileBrowserView * tree_view, guint code,
		   gchar * message, GeditFileBrowserWidget * obj)
{
	g_signal_emit (obj, signals[ERROR], 0, code, message);
}

static void
on_combo_changed (GtkComboBox * combo, GeditFileBrowserWidget * obj)
{
	GtkTreeIter iter;
	guint id;
	gchar *uri;

	if (!gtk_combo_box_get_active_iter (combo, &iter)) {
		g_warning ("Could not get iter in combo!");
		return;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (obj->priv->combo_model), &iter,
			    COLUMN_ID, &id, -1);

	switch (id) {
	case BOOKMARKS_ID:
		gedit_file_browser_widget_show_bookmarks (obj);
		break;
	case PATH_ID:
		gtk_tree_model_get (GTK_TREE_MODEL
				    (obj->priv->combo_model), &iter,
				    COLUMN_OBJECT, &uri, -1);
		gedit_file_browser_store_set_virtual_root_from_string
		    (obj->priv->file_store, uri);
		break;
	}
}

static gboolean
on_treeview_popup_menu (GeditFileBrowserView * treeview,
			GeditFileBrowserWidget * obj)
{
	if (!GEDIT_IS_FILE_BROWSER_STORE
	    (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview))))
		return FALSE;

	return popup_menu (obj, NULL);
}

static gboolean
on_treeview_button_press_event (GeditFileBrowserView * treeview,
				GdkEventButton * event,
				GeditFileBrowserWidget * obj)
{
	if (!GEDIT_IS_FILE_BROWSER_STORE
	    (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview))))
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
		popup_menu (obj, event);

	return FALSE;
}

static gboolean
do_change_directory (GeditFileBrowserWidget * obj,
                     GdkEventKey            * event)
{
	GtkAction * action = NULL;

	if ((event->state & 
	    (~GDK_CONTROL_MASK & ~GDK_SHIFT_MASK & ~GDK_MOD1_MASK)) == 
	     event->state && event->keyval == GDK_BackSpace)
		action = gtk_action_group_get_action (obj->priv->
		                                      action_group_sensitive,
		                                      "DirectoryPrevious");
	else if (!((event->state & GDK_MOD1_MASK) && 
	    (event->state & (~GDK_CONTROL_MASK & ~GDK_SHIFT_MASK)) == event->state))
		return FALSE;

	switch (event->keyval) {
		case GDK_Left:
			action = gtk_action_group_get_action (obj->priv->
			                                      action_group_sensitive,
			                                      "DirectoryPrevious");
		break;
		case GDK_Right:
			action = gtk_action_group_get_action (obj->priv->
			                                      action_group_sensitive,
			                                      "DirectoryNext");
		break;
		case GDK_Up:
			action = gtk_action_group_get_action (obj->priv->
			                                      action_group,
			                                      "DirectoryUp");
		break;
		default:
		break;
	}
	
	if (action != NULL) {
		gtk_action_activate (action);
		return TRUE;
	}
	
	return FALSE;
}

	
static gboolean
on_treeview_key_press_event (GeditFileBrowserView * treeview,
			     GdkEventKey * event,
			     GeditFileBrowserWidget * obj)
{
	if (do_change_directory (obj, event))
		return TRUE;

	if (!GEDIT_IS_FILE_BROWSER_STORE
	    (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview))))
		return FALSE;

	if (event->keyval == GDK_Delete
	    && !(event->state & GDK_CONTROL_MASK)) {
		if (event->state & GDK_SHIFT_MASK) {
			delete_selected_file (obj, FALSE);
		} else {
			delete_selected_file (obj, TRUE);
		}

		return TRUE;
	}

	return FALSE;
}

static void
on_selection_changed (GtkTreeSelection * selection,
		      GeditFileBrowserWidget * obj)
{
	gboolean has_selection = FALSE;
	GtkTreeModel *model;
	GtkTreeIter iter;
	guint flags;

	if (GEDIT_IS_FILE_BROWSER_STORE
	    (gtk_tree_view_get_model
	     (GTK_TREE_VIEW (obj->priv->treeview))))
		has_selection =
		    gtk_tree_selection_get_selected (selection, &model,
						     &iter);

	if (has_selection) {
		gtk_tree_model_get (model, &iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS,
				    &flags, -1);

		if (FILE_IS_DUMMY (flags))
			has_selection = FALSE;
	}

	gtk_action_group_set_sensitive (obj->priv->action_group_selection,
					has_selection);
}

static gboolean
on_entry_filter_activate (GeditFileBrowserWidget * obj)
{
	gchar const *text;

	text = gtk_entry_get_text (GTK_ENTRY (obj->priv->filter_entry));
	set_filter_pattern_real (obj, text, FALSE);

	return FALSE;
}

static void
on_location_jump_activate (GtkMenuItem * item,
			   GeditFileBrowserWidget * obj)
{
	GList *location;

	location = g_object_get_data (G_OBJECT (item), LOCATION_DATA_KEY);

	if (obj->priv->current_location) {
		jump_to_location (obj, location,
				  g_list_position (obj->priv->locations,
						   location) >
				  g_list_position (obj->priv->locations,
						   obj->priv->
						   current_location));
	} else {
		jump_to_location (obj, location, TRUE);
	}

}

static void
on_bookmarks_row_inserted (GtkTreeModel * model, 
                           GtkTreePath * path,
                           GtkTreeIter * iter,
                           GeditFileBrowserWidget *obj)
{
	add_bookmark_hash (obj, iter);
}

static void 
on_filter_mode_changed (GeditFileBrowserStore * model,
                        GParamSpec * param,
                        GeditFileBrowserWidget * obj)
{
	gint mode;
	GtkToggleAction * action;
	gboolean active;

	mode = gedit_file_browser_store_get_filter_mode (model);
	
	action = GTK_TOGGLE_ACTION (gtk_action_group_get_action (obj->priv->action_group, 
	                                                         "FilterHidden"));
	active = !(mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN);
	
	if (active != gtk_toggle_action_get_active (action))
		gtk_toggle_action_set_active (action, active);

	action = GTK_TOGGLE_ACTION (gtk_action_group_get_action (obj->priv->action_group, 
	                                                         "FilterBinary"));
	active = !(mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY);
	
	if (active != gtk_toggle_action_get_active (action))
		gtk_toggle_action_set_active (action, active);	
}

static void
on_action_directory_next (GtkAction * action, GeditFileBrowserWidget * obj)
{
	if (obj->priv->locations)
		jump_to_location (obj, obj->priv->current_location->prev,
				  FALSE);
}

static void
on_action_directory_previous (GtkAction * action,
			      GeditFileBrowserWidget * obj)
{
	if (obj->priv->locations) {
		if (obj->priv->current_location)
			jump_to_location (obj,
					  obj->priv->current_location->
					  next, TRUE);
		else {
			jump_to_location (obj, obj->priv->locations, TRUE);
		}
	}
}

static void 
on_action_directory_up (GtkAction              * action,
			GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	   
	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;
	
	gedit_file_browser_store_set_virtual_root_up (GEDIT_FILE_BROWSER_STORE (model));	
}


static void
on_action_directory_new (GtkAction * action, GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter parent;
	GtkTreeIter iter;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	if (!gedit_file_browser_widget_get_selected_directory (obj, &parent))
		return;

	if (gedit_file_browser_store_new_directory
	    (GEDIT_FILE_BROWSER_STORE (model), &parent, &iter)) {
		gedit_file_browser_view_start_rename (obj->priv->treeview,
						      &iter);
	}
}

static void
on_action_file_new (GtkAction * action, GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter parent;
	GtkTreeIter iter;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	if (!gedit_file_browser_widget_get_selected_directory (obj, &parent))
		return;

	if (gedit_file_browser_store_new_file
	    (GEDIT_FILE_BROWSER_STORE (model), &parent, &iter)) {
		gedit_file_browser_view_start_rename (obj->priv->treeview,
						      &iter);
	}
}

static void
on_action_file_rename (GtkAction * action, GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *selmodel;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	selection =
	    gtk_tree_view_get_selection (GTK_TREE_VIEW
					 (obj->priv->treeview));

	if (gtk_tree_selection_get_selected (selection, &selmodel, &iter))
		gedit_file_browser_view_start_rename (obj->priv->treeview,
						      &iter);
}

static void
on_action_file_delete (GtkAction * action, GeditFileBrowserWidget * obj)
{
	delete_selected_file (obj, FALSE);
}

static void
on_action_file_move_to_trash (GtkAction * action, GeditFileBrowserWidget * obj)
{
	delete_selected_file (obj, TRUE);
}

static void
on_action_directory_refresh (GtkAction * action,
			     GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		gedit_file_browser_store_refresh (GEDIT_FILE_BROWSER_STORE
						  (model));
	else if (GEDIT_IS_FILE_BOOKMARKS_STORE (model)) {
		g_hash_table_ref (obj->priv->bookmarks_hash);
		g_hash_table_destroy (obj->priv->bookmarks_hash);

		gedit_file_bookmarks_store_refresh
		    (GEDIT_FILE_BOOKMARKS_STORE (model));
	}
}

static void
on_action_directory_open (GtkAction * action, GeditFileBrowserWidget * obj)
{
	GtkTreeModel *model =
	    gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter parent;
	GError *error = NULL;
	gchar *uri = NULL;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	if (!gedit_file_browser_widget_get_selected_directory (obj, &parent))
		return;


	gtk_tree_model_get (model, &parent,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_URI, &uri, -1);

	if (!gnome_url_show (uri, &error)) {
		g_signal_emit (obj, signals[ERROR], 0,
			       GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY,
			       error->message);
	}

	g_free (uri);
}

static void
on_action_filter_hidden (GtkAction * action, GeditFileBrowserWidget * obj)
{
	update_filter_mode (obj, 
	                    action, 
	                    GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN);
}

static void
on_action_filter_binary (GtkAction * action, GeditFileBrowserWidget * obj)
{
	update_filter_mode (obj, 
	                    action, 
	                    GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY);
}

// ex:ts=8:noet:
