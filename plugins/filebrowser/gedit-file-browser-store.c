/*
 * gedit-file-browser-store.c - Gedit plugin providing easy file access 
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

#include <glib/gi18n-lib.h>
#include <libgnomeui/libgnomeui.h>
#include <gedit/gedit-plugin.h>

#include "gedit-file-browser-store.h"
#include "gedit-file-browser-marshal.h"
#include "gedit-file-browser-enum-types.h"
#include "gedit-file-browser-error.h"
#include "gedit-file-browser-utils.h"

#define GEDIT_FILE_BROWSER_STORE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BROWSER_STORE, GeditFileBrowserStorePrivate))

#define NODE_IS_DIR(node)				(FILE_IS_DIR((node)->flags))
#define NODE_IS_HIDDEN(node)			(FILE_IS_HIDDEN((node)->flags))
#define NODE_IS_TEXT(node)				(FILE_IS_TEXT((node)->flags))
#define NODE_LOADED(node)				(FILE_LOADED((node)->flags))
#define NODE_IS_FILTERED(node)			(FILE_IS_FILTERED((node)->flags))
#define NODE_IS_DUMMY(node)				(FILE_IS_DUMMY((node)->flags))

#define FILE_BROWSER_NODE_DIR(node)		((FileBrowserNodeDir *)(node))

typedef struct _FileBrowserNode    FileBrowserNode;
typedef struct _FileBrowserNodeDir FileBrowserNodeDir;

typedef gint (*SortFunc) (FileBrowserNode * node1,
			  FileBrowserNode * node2);

typedef struct 
{
	GnomeVFSAsyncHandle *handle;
	GeditFileBrowserStore *model;
	gpointer user_data;

	gboolean alive;
} AsyncHandle;

struct _FileBrowserNode 
{
	GnomeVFSURI *uri;
	gchar *mime_type;
	guint flags;
	gchar *name;

	GdkPixbuf *icon;
	GdkPixbuf *emblem;

	FileBrowserNode *parent;
	gint pos;
};

struct _FileBrowserNodeDir 
{
	FileBrowserNode node;
	GSList *children;

	GnomeVFSAsyncHandle *load_handle;
	GnomeVFSMonitorHandle *monitor_handle;
	GeditFileBrowserStore *model;
};

struct _GeditFileBrowserStorePrivate 
{
	FileBrowserNode *root;
	FileBrowserNode *virtual_root;
	GType column_types[GEDIT_FILE_BROWSER_STORE_COLUMN_NUM];

	GeditFileBrowserStoreFilterMode filter_mode;
	GeditFileBrowserStoreFilterFunc filter_func;
	gpointer filter_user_data;

	SortFunc sort_func;

	GSList *async_handles;
};

static void model_remove_node                               (GeditFileBrowserStore * model,
							     FileBrowserNode * node, 
							     GtkTreePath * path,
							     gboolean free_nodes);

static void gedit_file_browser_store_iface_init             (GtkTreeModelIface * iface);
static GtkTreeModelFlags gedit_file_browser_store_get_flags (GtkTreeModel * tree_model);
static gint gedit_file_browser_store_get_n_columns          (GtkTreeModel * tree_model);
static GType gedit_file_browser_store_get_column_type       (GtkTreeModel * tree_model,
							     gint index);

static gboolean gedit_file_browser_store_get_iter           (GtkTreeModel * tree_model,
							     GtkTreeIter * iter,
							     GtkTreePath * path);

static GtkTreePath *gedit_file_browser_store_get_path       (GtkTreeModel * tree_model,
							     GtkTreeIter * iter);

static void gedit_file_browser_store_get_value              (GtkTreeModel * tree_model,
							     GtkTreeIter * iter,
							     gint column,
							     GValue * value);

static gboolean gedit_file_browser_store_iter_next          (GtkTreeModel * tree_model,
							     GtkTreeIter * iter);

static gboolean gedit_file_browser_store_iter_children      (GtkTreeModel * tree_model,
							     GtkTreeIter * iter,
							     GtkTreeIter * parent);

static gboolean gedit_file_browser_store_iter_has_child     (GtkTreeModel * tree_model,
							     GtkTreeIter * iter);

static gint gedit_file_browser_store_iter_n_children        (GtkTreeModel * tree_model,
							     GtkTreeIter * iter);

static gboolean gedit_file_browser_store_iter_nth_child     (GtkTreeModel * tree_model,
							     GtkTreeIter * iter,
							     GtkTreeIter * parent, 
							     gint n);

static gboolean gedit_file_browser_store_iter_parent        (GtkTreeModel * tree_model,
							     GtkTreeIter * iter,
							     GtkTreeIter * child);

static void file_browser_node_free                          (GeditFileBrowserStore * model,
							     FileBrowserNode * node);
static void model_add_node                                  (GeditFileBrowserStore * model,
							     FileBrowserNode * child,
							     FileBrowserNode * parent);
static void model_clear                                     (GeditFileBrowserStore * model,
							     gboolean free_nodes);
static gint model_sort_default                              (FileBrowserNode * node1,
							     FileBrowserNode * node2);
static void print_tree                                      (GeditFileBrowserStore * model,
							     FileBrowserNode * parent, 
							     gchar * prefix);
static void model_check_dummy                               (GeditFileBrowserStore * model,
							     FileBrowserNode * node);

static void on_directory_monitor_event                      (GnomeVFSMonitorHandle * handle,
							     gchar const *monitor_uri,
							     const gchar * info_uri,
							     GnomeVFSMonitorEventType event_type,
							     FileBrowserNode * parent);

GEDIT_PLUGIN_DEFINE_TYPE_WITH_CODE (GeditFileBrowserStore, gedit_file_browser_store,
			G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
					       gedit_file_browser_store_iface_init))

/* Properties */
enum {
	PROP_0,

	PROP_ROOT,
	PROP_VIRTUAL_ROOT,
	PROP_FILTER_MODE
};

/* Signals */
enum 
{
	BEGIN_LOADING,
	END_LOADING,
	ERROR,
	NUM_SIGNALS
};

static guint model_signals[NUM_SIGNALS] = { 0 };

static void
gedit_file_browser_store_finalize (GObject * object)
{
	GeditFileBrowserStore *obj = GEDIT_FILE_BROWSER_STORE (object);
	GSList *item;

	// Free all the nodes
	file_browser_node_free (obj, obj->priv->root);

	for (item = obj->priv->async_handles; item; item = item->next)
		((AsyncHandle *) (item->data))->alive = FALSE;

	g_slist_free (obj->priv->async_handles);
	G_OBJECT_CLASS (gedit_file_browser_store_parent_class)->
	    finalize (object);
}

static void
set_gvalue_from_node (GValue          *value,
                      FileBrowserNode *node)
{
	gchar * uri;

	if (node == NULL || node->uri)
		g_value_set_string (value, NULL);
	else {
		uri = gnome_vfs_uri_to_string (node->uri, 
		                               GNOME_VFS_URI_HIDE_NONE);
		g_value_take_string (value, uri);
	}
}

static void
gedit_file_browser_store_get_property (GObject    *object,
			               guint       prop_id,
			               GValue     *value,
			               GParamSpec *pspec)
{
	GeditFileBrowserStore *obj = GEDIT_FILE_BROWSER_STORE (object);

	switch (prop_id)
	{
		case PROP_ROOT:
			set_gvalue_from_node (value, obj->priv->root);
			break;
		case PROP_VIRTUAL_ROOT:
			set_gvalue_from_node (value, obj->priv->virtual_root);
			break;		
		case PROP_FILTER_MODE:
			g_value_set_flags (value, obj->priv->filter_mode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_store_set_property (GObject      *object,
			               guint         prop_id,
			               const GValue *value,
			               GParamSpec   *pspec)
{
	GeditFileBrowserStore *obj = GEDIT_FILE_BROWSER_STORE (object);

	switch (prop_id)
	{
		case PROP_FILTER_MODE:
			gedit_file_browser_store_set_filter_mode (obj,
			                                          g_value_get_flags (value));
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_store_class_init (GeditFileBrowserStoreClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_file_browser_store_finalize;

	object_class->get_property = gedit_file_browser_store_get_property;
	object_class->set_property = gedit_file_browser_store_set_property;

	g_object_class_install_property (object_class, PROP_ROOT,
					 g_param_spec_string ("root",
					 		      "Root",
					 		      "The root uri",
					 		      NULL,
					 		      G_PARAM_READABLE));

	g_object_class_install_property (object_class, PROP_VIRTUAL_ROOT,
					 g_param_spec_string ("virtual-root",
					 		      "Virtual Root",
					 		      "The virtual root uri",
					 		      NULL,
					 		      G_PARAM_READABLE));

	g_object_class_install_property (object_class, PROP_FILTER_MODE,
					 g_param_spec_flags ("filter-mode",
					 		      "Filter Mode",
					 		      "The filter mode",
					 		      GEDIT_TYPE_FILE_BROWSER_STORE_FILTER_MODE,
					 		      gedit_file_browser_store_filter_mode_get_default (),
					 		      G_PARAM_READWRITE));

	model_signals[BEGIN_LOADING] =
	    g_signal_new ("begin-loading",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserStoreClass,
					   begin_loading), NULL, NULL,
			  g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1,
			  GTK_TYPE_TREE_ITER);
	model_signals[END_LOADING] =
	    g_signal_new ("end-loading",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserStoreClass,
					   end_loading), NULL, NULL,
			  g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1,
			  GTK_TYPE_TREE_ITER);
	model_signals[ERROR] =
	    g_signal_new ("error", G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserStoreClass,
					   error), NULL, NULL,
			  gedit_file_browser_marshal_VOID__UINT_STRING,
			  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

	g_type_class_add_private (object_class,
				  sizeof (GeditFileBrowserStorePrivate));
}

static void
gedit_file_browser_store_iface_init (GtkTreeModelIface * iface)
{
	iface->get_flags = gedit_file_browser_store_get_flags;
	iface->get_n_columns = gedit_file_browser_store_get_n_columns;
	iface->get_column_type = gedit_file_browser_store_get_column_type;
	iface->get_iter = gedit_file_browser_store_get_iter;
	iface->get_path = gedit_file_browser_store_get_path;
	iface->get_value = gedit_file_browser_store_get_value;
	iface->iter_next = gedit_file_browser_store_iter_next;
	iface->iter_children = gedit_file_browser_store_iter_children;
	iface->iter_has_child = gedit_file_browser_store_iter_has_child;
	iface->iter_n_children = gedit_file_browser_store_iter_n_children;
	iface->iter_nth_child = gedit_file_browser_store_iter_nth_child;
	iface->iter_parent = gedit_file_browser_store_iter_parent;
}

static void
gedit_file_browser_store_init (GeditFileBrowserStore * obj)
{
	obj->priv = GEDIT_FILE_BROWSER_STORE_GET_PRIVATE (obj);

	obj->priv->column_types[GEDIT_FILE_BROWSER_STORE_COLUMN_URI] =
	    G_TYPE_STRING;
	obj->priv->column_types[GEDIT_FILE_BROWSER_STORE_COLUMN_NAME] =
	    G_TYPE_STRING;
	obj->priv->column_types[GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS] =
	    G_TYPE_UINT;
	obj->priv->column_types[GEDIT_FILE_BROWSER_STORE_COLUMN_ICON] =
	    GDK_TYPE_PIXBUF;
	obj->priv->column_types[GEDIT_FILE_BROWSER_STORE_COLUMN_EMBLEM] =
	    GDK_TYPE_PIXBUF;

	// Default filter mode is hiding the hidden files
	obj->priv->filter_mode = gedit_file_browser_store_filter_mode_get_default ();
	obj->priv->sort_func = model_sort_default;
}

static gboolean
node_has_parent (FileBrowserNode * node, FileBrowserNode * parent)
{
	if (node->parent == NULL)
		return FALSE;

	if (node->parent == parent)
		return TRUE;

	return node_has_parent (node->parent, parent);
}

static gboolean
node_in_tree (GeditFileBrowserStore * model, FileBrowserNode * node)
{
	return node_has_parent (node, model->priv->virtual_root);
}

static gboolean
model_node_visibility (GeditFileBrowserStore * model,
		       FileBrowserNode * node)
{
	if (node == NULL)
		return FALSE;

	if (NODE_IS_DUMMY (node))
		return !NODE_IS_HIDDEN (node);

	if (node == model->priv->virtual_root)
		return TRUE;

	if (!node_has_parent (node, model->priv->virtual_root))
		return FALSE;

	return !NODE_IS_FILTERED (node);
}

/* Interface implementation */

static GtkTreeModelFlags
gedit_file_browser_store_get_flags (GtkTreeModel * tree_model)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      (GtkTreeModelFlags) 0);

	return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
gedit_file_browser_store_get_n_columns (GtkTreeModel * tree_model)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model), 0);

	return GEDIT_FILE_BROWSER_STORE_COLUMN_NUM;
}

static GType
gedit_file_browser_store_get_column_type (GtkTreeModel * tree_model,
					  gint index)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      G_TYPE_INVALID);
	g_return_val_if_fail (index < GEDIT_FILE_BROWSER_STORE_COLUMN_NUM
			      && index >= 0, G_TYPE_INVALID);

	return GEDIT_FILE_BROWSER_STORE (tree_model)->priv->
	    column_types[index];
}

static gboolean
gedit_file_browser_store_get_iter (GtkTreeModel * tree_model,
				   GtkTreeIter * iter, GtkTreePath * path)
{
	gint *indices, depth, i;
	FileBrowserNode *node;
	GeditFileBrowserStore *model;
	GSList *item;
	gint num;

	g_assert (GEDIT_IS_FILE_BROWSER_STORE (tree_model));
	g_assert (path != NULL);

	model = GEDIT_FILE_BROWSER_STORE (tree_model);
	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);
	node = model->priv->virtual_root;

	for (i = 0; i < depth; ++i) {
		if (node == NULL)
			return FALSE;

		num = 0;

		if (!NODE_IS_DIR (node))
			return FALSE;

		for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
		     item = item->next) {
			if (model_node_visibility
			    (model, (FileBrowserNode *) (item->data))) {
				if (num == indices[i]) {
					node =
					    (FileBrowserNode *) (item->
								 data);
					break;
				}

				num++;
			}
		}

		if (item == NULL)
			return FALSE;

		node = (FileBrowserNode *) (item->data);
	}

	iter->user_data = node;
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;

	return node != NULL;
}

static GtkTreePath *
gedit_file_browser_store_get_path_real (GeditFileBrowserStore * model,
					FileBrowserNode * node)
{
	GtkTreePath *path = gtk_tree_path_new ();
	FileBrowserNode *check;
	GSList *item;
	gint num = 0;

	while (node != model->priv->virtual_root) {
		if (node->parent == NULL) {
			gtk_tree_path_free (path);
			return NULL;
		}

		num = 0;

		for (item = FILE_BROWSER_NODE_DIR (node->parent)->children;
		     item; item = item->next) {
			check = (FileBrowserNode *) (item->data);

			if (model_node_visibility (model, check)) {
				if (check == node) {
					gtk_tree_path_prepend_index (path,
								     num);
					break;
				}

				++num;
			} else if (check == node) {
				if (NODE_IS_DUMMY (node))
					g_warning ("Dummy not visible???");

				gtk_tree_path_free (path);
				return NULL;
			}
		}

		node = node->parent;
	}

	return path;
}

static GtkTreePath *
gedit_file_browser_store_get_path (GtkTreeModel * tree_model,
				   GtkTreeIter * iter)
{
	FileBrowserNode *node;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      NULL);
	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);

	node = (FileBrowserNode *) (iter->user_data);

	return
	    gedit_file_browser_store_get_path_real
	    (GEDIT_FILE_BROWSER_STORE (tree_model), node);
}

static void
gedit_file_browser_store_get_value (GtkTreeModel * tree_model,
				    GtkTreeIter * iter, gint column,
				    GValue * value)
{
	FileBrowserNode *node;
	gchar *uri;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->user_data != NULL);

	node = (FileBrowserNode *) (iter->user_data);

	g_value_init (value,
		      GEDIT_FILE_BROWSER_STORE (tree_model)->priv->
		      column_types[column]);

	switch (column) {
	case GEDIT_FILE_BROWSER_STORE_COLUMN_URI:
		if (node->uri == NULL)
			uri = NULL;
		else
			uri =
			    gnome_vfs_uri_to_string (node->uri,
						     GNOME_VFS_URI_HIDE_NONE);

		g_value_take_string (value, uri);
		break;
	case GEDIT_FILE_BROWSER_STORE_COLUMN_NAME:
		g_value_set_string (value, node->name);
		break;
	case GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS:
		g_value_set_uint (value, node->flags);
		break;
	case GEDIT_FILE_BROWSER_STORE_COLUMN_ICON:
		g_value_set_object (value, node->icon);
		break;
	case GEDIT_FILE_BROWSER_STORE_COLUMN_EMBLEM:
		g_value_set_object (value, node->emblem);
		break;
	}
}

static gboolean
gedit_file_browser_store_iter_next (GtkTreeModel * tree_model,
				    GtkTreeIter * iter)
{
	GeditFileBrowserStore *model;
	FileBrowserNode *node;
	GSList *item;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	model = GEDIT_FILE_BROWSER_STORE (tree_model);
	node = (FileBrowserNode *) (iter->user_data);

	if (node->parent == NULL)
		return FALSE;

	for (item =
	     g_slist_next (g_slist_find
			   (FILE_BROWSER_NODE_DIR (node->parent)->children,
			    node)); item; item = item->next) {
		if (model_node_visibility
		    (model, (FileBrowserNode *) (item->data))) {
			iter->user_data = item->data;
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
gedit_file_browser_store_iter_children (GtkTreeModel * tree_model,
					GtkTreeIter * iter,
					GtkTreeIter * parent)
{
	FileBrowserNode *node;
	GeditFileBrowserStore *model;
	GSList *item;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      FALSE);
	g_return_val_if_fail (parent == NULL
			      || parent->user_data != NULL, FALSE);

	model = GEDIT_FILE_BROWSER_STORE (tree_model);

	if (parent == NULL)
		node = model->priv->virtual_root;
	else
		node = (FileBrowserNode *) (parent->user_data);

	if (node == NULL)
		return FALSE;

	if (!NODE_IS_DIR (node))
		return FALSE;

	for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
	     item = item->next) {
		if (model_node_visibility
		    (model, (FileBrowserNode *) (item->data))) {
			iter->user_data = item->data;
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
filter_tree_model_iter_has_child_real (GeditFileBrowserStore * model,
				       FileBrowserNode * node)
{
	GSList *item;

	if (!NODE_IS_DIR (node))
		return FALSE;

	for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
	     item = item->next)
		if (model_node_visibility
		    (model, (FileBrowserNode *) (item->data)))
			return TRUE;

	return FALSE;
}

static gboolean
gedit_file_browser_store_iter_has_child (GtkTreeModel * tree_model,
					 GtkTreeIter * iter)
{
	FileBrowserNode *node;
	GeditFileBrowserStore *model;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      FALSE);
	g_return_val_if_fail (iter == NULL
			      || iter->user_data != NULL, FALSE);

	model = GEDIT_FILE_BROWSER_STORE (tree_model);

	if (iter == NULL)
		node = model->priv->virtual_root;
	else
		node = (FileBrowserNode *) (iter->user_data);

	return filter_tree_model_iter_has_child_real (model, node);
}

static gint
gedit_file_browser_store_iter_n_children (GtkTreeModel * tree_model,
					  GtkTreeIter * iter)
{
	FileBrowserNode *node;
	GeditFileBrowserStore *model;
	GSList *item;
	gint num = 0;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      FALSE);
	g_return_val_if_fail (iter == NULL
			      || iter->user_data != NULL, FALSE);

	model = GEDIT_FILE_BROWSER_STORE (tree_model);

	if (iter == NULL)
		node = model->priv->virtual_root;
	else
		node = (FileBrowserNode *) (iter->user_data);

	if (!NODE_IS_DIR (node))
		return 0;

	for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
	     item = item->next)
		if (model_node_visibility
		    (model, (FileBrowserNode *) (item->data)))
			++num;

	return num;
}

static gboolean
gedit_file_browser_store_iter_nth_child (GtkTreeModel * tree_model,
					 GtkTreeIter * iter,
					 GtkTreeIter * parent, gint n)
{
	FileBrowserNode *node;
	GeditFileBrowserStore *model;
	GSList *item;
	gint num = 0;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      FALSE);
	g_return_val_if_fail (parent == NULL
			      || parent->user_data != NULL, FALSE);

	model = GEDIT_FILE_BROWSER_STORE (tree_model);

	if (parent == NULL)
		node = model->priv->virtual_root;
	else
		node = (FileBrowserNode *) (parent->user_data);

	if (!NODE_IS_DIR (node))
		return FALSE;

	for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
	     item = item->next) {
		if (model_node_visibility
		    (model, (FileBrowserNode *) (item->data))) {
			if (num == n) {
				iter->user_data = item->data;
				return TRUE;
			}

			++num;
		}
	}

	return FALSE;
}

static gboolean
gedit_file_browser_store_iter_parent (GtkTreeModel * tree_model,
				      GtkTreeIter * iter,
				      GtkTreeIter * child)
{
	FileBrowserNode *node;
	GeditFileBrowserStore *model;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model),
			      FALSE);
	g_return_val_if_fail (child != NULL, FALSE);
	g_return_val_if_fail (child->user_data != NULL, FALSE);

	node = (FileBrowserNode *) (child->user_data);
	model = GEDIT_FILE_BROWSER_STORE (tree_model);

	if (!node_in_tree (GEDIT_FILE_BROWSER_STORE (tree_model), node))
		return FALSE;

	if (node->parent == NULL)
		return FALSE;

	iter->user_data = node->parent;
	return TRUE;
}

#define FILTER_HIDDEN(mode) (mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN)
#define FILTER_BINARY(mode) (mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY)

/* Private */
static void
model_begin_loading (GeditFileBrowserStore * model, FileBrowserNode * node)
{
	GtkTreeIter iter;

	iter.user_data = node;
	g_signal_emit (model, model_signals[BEGIN_LOADING], 0, &iter);
}

static void
model_end_loading (GeditFileBrowserStore * model, FileBrowserNode * node)
{
	GtkTreeIter iter;

	iter.user_data = node;
	g_signal_emit (model, model_signals[END_LOADING], 0, &iter);
}

static void
model_node_update_visibility (GeditFileBrowserStore * model,
			      FileBrowserNode * node)
{
	GtkTreeIter iter;

	node->flags &= ~GEDIT_FILE_BROWSER_STORE_FLAG_IS_FILTERED;

	if (FILTER_HIDDEN (model->priv->filter_mode) &&
	    NODE_IS_HIDDEN (node))
		node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_FILTERED;
	else if (FILTER_BINARY (model->priv->filter_mode) &&
		 (!NODE_IS_TEXT (node) && !NODE_IS_DIR (node)))
		node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_FILTERED;
	else if (model->priv->filter_func) {
		iter.user_data = node;

		if (!model->priv->
		    filter_func (model, &iter,
				 model->priv->filter_user_data))
			node->flags |=
			    GEDIT_FILE_BROWSER_STORE_FLAG_IS_FILTERED;
	}
}

static gint
model_sort_default (FileBrowserNode * node1, FileBrowserNode * node2)
{
	gchar *case1;
	gchar *case2;
	gint result;
	gint f1;
	gint f2;

	f1 = NODE_IS_DUMMY (node1);
	f2 = NODE_IS_DUMMY (node2);

	if (f1 && f2)
		return 0;
	else if (f1)
		return -1;
	else if (f2)
		return 1;

	f1 = NODE_IS_DIR (node1);
	f2 = NODE_IS_DIR (node2);

	if (f1 != f2) {
		if (f1) {
			return -1;
		} else {
			return 1;
		}
	}

	f1 = NODE_IS_HIDDEN (node1);
	f2 = NODE_IS_HIDDEN (node2);

	if (f1 != f2) {
		if (f1)
			return -1;
		else
			return 1;
	}

	if (node1->name == NULL)
		return -1;
	else if (node2->name == NULL)
		return 1;
	else {
		case1 = g_utf8_casefold (node1->name, -1);
		case2 = g_utf8_casefold (node2->name, -1);

		result = g_utf8_collate (case1, case2);

		g_free (case1);
		g_free (case2);

		return result;
	}
}

static void
model_resort_children (GeditFileBrowserStore * model,
		       FileBrowserNode * parent, GtkTreePath * path)
{
	FileBrowserNodeDir *dir;
	FileBrowserNode *node;
	GSList *item;
	gint *neworder;
	gint pos = 0;
	gboolean visible;
	GtkTreeIter iter;
	gboolean free_path = FALSE;

	if (!NODE_IS_DIR (parent))
		return;

	dir = FILE_BROWSER_NODE_DIR (parent);

	if (!dir->children)
		return;

	visible = model_node_visibility (model, parent);

	if (visible) {
		pos = 0;

		if (path == NULL) {
			path = gtk_tree_path_new ();
			free_path = TRUE;
		}
	}

	gtk_tree_path_up (path);

	for (item = dir->children; item; item = item->next) {
		node = (FileBrowserNode *) (item->data);
		model_resort_children (model, node, path);

		if (visible && model_node_visibility (model, node)) {
			node->pos = pos++;
			gtk_tree_path_next (path);
		}
	}

	gtk_tree_path_down (path);

	if (visible) {
		dir->children =
		    g_slist_sort (dir->children,
				  (GCompareFunc) (model->priv->sort_func));
		neworder = g_new (gint, pos);

		pos = 0;

		for (item = dir->children; item; item = item->next) {
			node = (FileBrowserNode *) (item->data);

			if (model_node_visibility (model, node))
				neworder[pos++] = node->pos;
		}

		iter.user_data = parent;
		gtk_tree_model_rows_reordered (GTK_TREE_MODEL (model),
					       path, &iter, neworder);
		g_free (neworder);

		if (free_path)
			gtk_tree_path_free (path);
	}
}

static void
model_resort_node (GeditFileBrowserStore * model, FileBrowserNode * node)
{
	FileBrowserNodeDir *dir;
	GSList *item;
	FileBrowserNode *child;
	gint pos = 0;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint *neworder;

	dir = FILE_BROWSER_NODE_DIR (node->parent);

	if (!model_node_visibility (model, node->parent)) {
		/* Just sort the children of the parent */
		dir->children = g_slist_sort (dir->children,
					      (GCompareFunc) (model->priv->
							      sort_func));
	} else {
		/* Store current positions */
		for (item = dir->children; item; item = item->next) {
			child = (FileBrowserNode *) (item->data);

			if (model_node_visibility (model, child))
				child->pos = pos++;
		}

		dir->children = g_slist_sort (dir->children,
					      (GCompareFunc) (model->priv->
							      sort_func));
		neworder = g_new (gint, pos);
		pos = 0;

		/* Store the new positions */
		for (item = dir->children; item; item = item->next) {
			child = (FileBrowserNode *) (item->data);

			if (model_node_visibility (model, child))
				neworder[pos++] = child->pos;
		}

		iter.user_data = node->parent;
		path =
		    gedit_file_browser_store_get_path_real (model,
							    node->parent);

		gtk_tree_model_rows_reordered (GTK_TREE_MODEL (model),
					       path, &iter, neworder);

		g_free (neworder);
		gtk_tree_path_free (path);
	}
}

static void
model_refilter_node (GeditFileBrowserStore * model, FileBrowserNode * node,
		     GtkTreePath * path)
{
	gboolean old_visible;
	gboolean new_visible;
	FileBrowserNodeDir *dir;
	GSList *item;
	GtkTreeIter iter;
	gboolean free_path = FALSE;
	gboolean in_tree;

	if (node == NULL)
		return;

	old_visible = model_node_visibility (model, node);
	model_node_update_visibility (model, node);

	in_tree = node_in_tree (model, node);

	if (path == NULL) {
		if (in_tree)
			path =
			    gedit_file_browser_store_get_path_real (model,
								    node);
		else
			path = gtk_tree_path_new_first ();

		free_path = TRUE;
	}

	if (NODE_IS_DIR (node)) {
		if (in_tree)
			gtk_tree_path_down (path);

		dir = FILE_BROWSER_NODE_DIR (node);

		for (item = dir->children; item; item = item->next) {
			model_refilter_node (model,
					     (FileBrowserNode *) (item->
								  data),
					     path);
		}

		if (in_tree)
			gtk_tree_path_up (path);
	}

	if (in_tree) {
		new_visible = model_node_visibility (model, node);

		if (old_visible != new_visible) {
			iter.user_data = node;

			if (old_visible) {
				gtk_tree_model_row_deleted (GTK_TREE_MODEL
							    (model), path);
			} else {
				gtk_tree_model_row_inserted (GTK_TREE_MODEL
							     (model), path,
							     &iter);

				model_check_dummy (model, node);
				gtk_tree_path_next (path);
			}
		} else if (old_visible) {
			gtk_tree_path_next (path);
		}
	}

	if (free_path)
		gtk_tree_path_free (path);
}

static void
model_refilter (GeditFileBrowserStore * model)
{
	model_refilter_node (model, model->priv->root, NULL);
}

static void
file_browser_node_set_name (FileBrowserNode * node)
{
	g_free (node->name);

	if (node->uri) {
		node->name = gedit_file_browser_utils_uri_basename (gnome_vfs_uri_get_path (node->uri));
	} else {
		node->name = NULL;
	}
}

static void
file_browser_node_init (FileBrowserNode * node, GnomeVFSURI * uri,
			FileBrowserNode * parent)
{
	if (uri != NULL) {
		node->uri = gnome_vfs_uri_ref (uri);
		file_browser_node_set_name (node);
	}

	node->parent = parent;
}

static FileBrowserNode *
file_browser_node_new (GnomeVFSURI * uri, FileBrowserNode * parent)
{
	FileBrowserNode *node = g_new0 (FileBrowserNode, 1);

	file_browser_node_init (node, uri, parent);
	return node;
}

static FileBrowserNode *
file_browser_node_dir_new (GeditFileBrowserStore * model,
			   GnomeVFSURI * uri, FileBrowserNode * parent)
{
	FileBrowserNode *node =
	    (FileBrowserNode *) g_new0 (FileBrowserNodeDir, 1);

	file_browser_node_init (node, uri, parent);

	node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_DIRECTORY;
	node->mime_type = g_strdup ("text/directory");

	FILE_BROWSER_NODE_DIR (node)->model = model;

	return node;
}

static void
file_browser_node_free_children (GeditFileBrowserStore * model,
				 FileBrowserNode * node)
{
	GSList *item;

	if (node == NULL)
		return;

	if (NODE_IS_DIR (node)) {
		for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
		     item = item->next)
			file_browser_node_free (model,
						(FileBrowserNode *) (item->
								     data));

		g_slist_free (FILE_BROWSER_NODE_DIR (node)->children);
		FILE_BROWSER_NODE_DIR (node)->children = NULL;

		/* This node is no longer loaded */
		node->flags &= ~GEDIT_FILE_BROWSER_STORE_FLAG_LOADED;
	}
}

static void
file_browser_node_free (GeditFileBrowserStore * model,
			FileBrowserNode * node)
{
	FileBrowserNodeDir *dir;

	if (node == NULL)
		return;

	if (NODE_IS_DIR (node)) {
		dir = FILE_BROWSER_NODE_DIR (node);

		if (dir->load_handle) {
			gnome_vfs_async_cancel (dir->load_handle);
			model_end_loading (model, node);
		}

		file_browser_node_free_children (model, node);

		if (dir->monitor_handle)
			gnome_vfs_monitor_cancel (dir->monitor_handle);
	}

	if (node->uri)
		gnome_vfs_uri_unref (node->uri);

	if (node->icon)
		g_object_unref (node->icon);

	if (node->emblem)
		g_object_unref (node->emblem);

	g_free (node->name);
	g_free (node->mime_type);
	g_free (node);
}

/* Should only be called for removing nodes that are visible in the model 
 * ======================================================================
 */

/**
 * model_remove_node_children:
 * @model: the #GeditFileBrowserStore
 * @node: the FileBrowserNode to remove
 * @path: the path of the node, or NULL to let the path be calculated
 * @free_nodes: whether to also remove the nodes from memory
 *
 * Removes all the children of node from the model. This function is used
 * to remove the child nodes from the _model_. Don't use it to just free
 * a node.
 **/
static void
model_remove_node_children (GeditFileBrowserStore * model,
			    FileBrowserNode * node, GtkTreePath * path,
			    gboolean free_nodes)
{
	FileBrowserNodeDir *dir;
	GtkTreePath *path_child;
	GSList *list;
	GSList *item;

	if (node == NULL || !NODE_IS_DIR (node))
		return;

	dir = FILE_BROWSER_NODE_DIR (node);

	if (dir->children == NULL)
		return;

	if (path == NULL)
		path_child =
		    gedit_file_browser_store_get_path_real (model, node);
	else
		path_child = gtk_tree_path_copy (path);

	gtk_tree_path_down (path_child);
	list = g_slist_copy (dir->children);

	for (item = list; item; item = item->next)
		model_remove_node (model, (FileBrowserNode *) (item->data),
				   path_child, free_nodes);

	g_slist_free (list);
	gtk_tree_path_free (path_child);
}

/**
 * model_remove_node:
 * @model: the #GeditFileBrowserStore
 * @node: the FileBrowserNode to remove
 * @path: the path to use to remove this node, or NULL to use the path 
 * calculated from the node itself
 * @free_nodes: whether to also remove the nodes from memory
 * 
 * Removes this node and all its children from the model. This function is used
 * to remove the node from the _model_. Don't use it to just free
 * a node.
 **/
static void
model_remove_node (GeditFileBrowserStore * model, FileBrowserNode * node,
		   GtkTreePath * path, gboolean free_nodes)
{
	gboolean free_path = FALSE;
	FileBrowserNode *parent;

	if (node == NULL)
		return;

	if (path == NULL) {
		path =
		    gedit_file_browser_store_get_path_real (model, node);
		free_path = TRUE;
	}

	model_remove_node_children (model, node, path, free_nodes);

	if (model_node_visibility (model, node))
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

	if (free_path)
		gtk_tree_path_free (path);

	parent = node->parent;

	if (free_nodes) {
		/* Remove the node from the parents children list */
		if (parent)
			FILE_BROWSER_NODE_DIR (node->parent)->children =
			    g_slist_remove (FILE_BROWSER_NODE_DIR
					    (node->parent)->children,
					    node);

		file_browser_node_free (model, node);
	}

	if (parent && model_node_visibility (model, parent))
		model_check_dummy (model, parent);
}

/**
 * model_clear:
 * @model: the #GeditFileBrowserStore
 * @free_nodes: whether to also remove the nodes from memory
 *
 * Removes all nodes from the model. This function is used
 * to remove all the nodes from the _model_. Don't use it to just free the
 * nodes in the model.
 **/
static void
model_clear (GeditFileBrowserStore * model, gboolean free_nodes)
{
	GtkTreePath *path;
	FileBrowserNodeDir *dir;
	FileBrowserNode *dummy;

	path = gtk_tree_path_new ();
	model_remove_node_children (model, model->priv->virtual_root, path,
				    free_nodes);
	gtk_tree_path_free (path);

	/* Remove the dummy if there is one */
	if (model->priv->virtual_root) {
		dir = FILE_BROWSER_NODE_DIR (model->priv->virtual_root);

		if (dir->children != NULL) {
			dummy = (FileBrowserNode *) (dir->children->data);

			if (NODE_IS_DUMMY (dummy)
			    && model_node_visibility (model, dummy)) {
				path = gtk_tree_path_new_first ();
				gtk_tree_model_row_deleted (GTK_TREE_MODEL
							    (model), path);
				gtk_tree_path_free (path);
			}
		}
	}
}

/* ====================================================================== */

static void
file_browser_node_unload (GeditFileBrowserStore * model,
			  FileBrowserNode * node, gboolean remove_children)
{
	FileBrowserNodeDir *dir;

	if (!NODE_IS_DIR (node) || !NODE_LOADED (node))
		return;

	dir = FILE_BROWSER_NODE_DIR (node);

	if (remove_children)
		model_remove_node_children (model, node, NULL, TRUE);

	if (dir->load_handle) {
		gnome_vfs_async_cancel (dir->load_handle);
		model_end_loading (model, node);
		dir->load_handle = NULL;
	}

	if (dir->monitor_handle) {
		gnome_vfs_monitor_cancel (dir->monitor_handle);
		dir->monitor_handle = NULL;
	}

	node->flags &= ~GEDIT_FILE_BROWSER_STORE_FLAG_LOADED;
}

static void
model_recomposite_icon_real (GeditFileBrowserStore * tree_model,
			     FileBrowserNode * node)
{
	GtkIconTheme *theme;
	gchar *icon;
	gchar *uri;
	gint icon_size;
	GdkPixbuf *pixbuf;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model));
	g_return_if_fail (node != NULL);

	if (node->uri == NULL)
		return;

	theme = gtk_icon_theme_get_default ();

	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, NULL, &icon_size);
	uri = gnome_vfs_uri_to_string (node->uri, GNOME_VFS_URI_HIDE_NONE);
	icon =
	    gnome_icon_lookup (theme, NULL, uri, NULL, NULL,
			       node->mime_type,
			       GNOME_ICON_LOOKUP_FLAGS_NONE,
			       GNOME_ICON_LOOKUP_RESULT_FLAGS_NONE);

	if (node->icon)
		g_object_unref (node->icon);

	if (icon == NULL)
		pixbuf = NULL;
	else
		pixbuf =
		    gtk_icon_theme_load_icon (theme, icon, icon_size, 0,
					      NULL);

	if (node->emblem) {
		if (!pixbuf) {
			node->icon =
			    gdk_pixbuf_new (gdk_pixbuf_get_colorspace
					    (node->emblem),
					    gdk_pixbuf_get_has_alpha
					    (node->emblem),
					    gdk_pixbuf_get_bits_per_sample
					    (node->emblem), icon_size,
					    icon_size);
		} else {
			node->icon = gdk_pixbuf_copy (pixbuf);
			g_object_unref (pixbuf);
		}

		gdk_pixbuf_composite (node->emblem, node->icon,
				      icon_size - 10, icon_size - 10, 10,
				      10, icon_size - 10, icon_size - 10,
				      1, 1, GDK_INTERP_NEAREST, 255);
	} else {
		node->icon = pixbuf;
	}

	g_free (uri);
	g_free (icon);
}

static void
model_recomposite_icon (GeditFileBrowserStore * tree_model,
			GtkTreeIter * iter)
{
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->user_data != NULL);

	model_recomposite_icon_real (tree_model,
				     (FileBrowserNode *) (iter->
							  user_data));
}

static FileBrowserNode *
model_create_dummy_node (GeditFileBrowserStore * model,
			 FileBrowserNode * parent)
{
	FileBrowserNode *dummy;

	dummy = file_browser_node_new (NULL, parent);
	dummy->name = g_strdup ("(Empty)");

	dummy->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_DUMMY;
	dummy->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;

	return dummy;
}

static FileBrowserNode *
model_add_dummy_node (GeditFileBrowserStore * model,
		      FileBrowserNode * parent)
{
	FileBrowserNode *dummy;

	dummy = model_create_dummy_node (model, parent);

	if (model_node_visibility (model, parent))
		dummy->flags &= ~GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;

	model_add_node (model, dummy, parent);

	return dummy;
}

static void
model_check_dummy (GeditFileBrowserStore * model, FileBrowserNode * node)
{
	FileBrowserNode *dummy;
	FileBrowserNodeDir *dir;
	GtkTreeIter iter;
	GtkTreePath *path;
	guint flags;

	// Hide the dummy child if needed
	if (NODE_IS_DIR (node)) {
		dir = FILE_BROWSER_NODE_DIR (node);

		if (dir->children == NULL) {
			model_add_dummy_node (model, node);
			return;
		}

		dummy = (FileBrowserNode *) (dir->children->data);

		if (!NODE_IS_DUMMY (dummy)) {
			dummy = model_create_dummy_node (model, node);
			dir->children =
			    g_slist_prepend (dir->children, dummy);
		}

		if (!model_node_visibility (model, node)) {
			dummy->flags |=
			    GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;
			return;
		}
		// Temporarily set the node to invisible to check for real childs
		flags = dummy->flags;
		dummy->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;

		if (!filter_tree_model_iter_has_child_real (model, node)) {
			dummy->flags &=
			    ~GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;

			if (FILE_IS_HIDDEN (flags)) {
				// Was hidden, needs to be inserted
				iter.user_data = dummy;
				path =
				    gedit_file_browser_store_get_path_real
				    (model, dummy);

				gtk_tree_model_row_inserted (GTK_TREE_MODEL
							     (model), path,
							     &iter);
				gtk_tree_path_free (path);
			}
		} else {
			if (!FILE_IS_HIDDEN (flags)) {
				// Was shown, needs to be removed

				// To get the path we need to set it to visible temporarily
				dummy->flags &=
				    ~GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;
				path =
				    gedit_file_browser_store_get_path_real
				    (model, dummy);
				dummy->flags |=
				    GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;

				gtk_tree_model_row_deleted (GTK_TREE_MODEL
							    (model), path);
				gtk_tree_path_free (path);
			}
		}
	}
}

static void
insert_node_sorted (GeditFileBrowserStore * model, FileBrowserNode * child,
		    FileBrowserNode * parent)
{
	FileBrowserNodeDir *dir;

	dir = FILE_BROWSER_NODE_DIR (parent);

	if (model->priv->sort_func == NULL) {
		dir->children = g_slist_append (dir->children, child);
	} else {
		dir->children =
		    g_slist_insert_sorted (dir->children, child,
					   (GCompareFunc) (model->priv->
							   sort_func));
	}
}

static void
model_add_node (GeditFileBrowserStore * model, FileBrowserNode * child,
		FileBrowserNode * parent)
{
	GtkTreeIter iter;
	GtkTreePath *path;

	// Add child to parents children
	insert_node_sorted (model, child, parent);

	if (model_node_visibility (model, parent) &&
	    model_node_visibility (model, child)) {
		iter.user_data = child;
		path =
		    gedit_file_browser_store_get_path_real (model, child);

		// Emit row inserted
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path,
					     &iter);
		gtk_tree_path_free (path);
	}

	model_check_dummy (model, parent);
	model_check_dummy (model, child);
}

static void
file_browser_node_set_from_info (GeditFileBrowserStore * model,
				 FileBrowserNode * node,
				 GnomeVFSFileInfo * info)
{
	gchar * filename;
	gchar const * mime;

	g_free (node->mime_type);
	node->mime_type = NULL;

	if (info->name) {
		if (*(info->name) == '.') {
			node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;
		} else if (g_utf8_get_char (g_utf8_offset_to_pointer 
		           (&(info->name[strlen(info->name)]), -1)) == '~') {
			node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;
			
			/* FIXME: this isn't totally correct. We need this 
			   though because the mime type system somehow says that
			   files ending on ~ are application/x-trash. Don't
			   ask why. The fact is that most ~ files are backup 
			   files and are text. Therefore we will try to guess
			   the mime type from the filename (minus ~) and default
			   to text/plain. Eat that, mime type system! */
			filename = g_strndup (info->name, strlen(info->name) - 1);
		   	mime = gnome_vfs_get_mime_type_for_name (filename);
			g_free (filename);
			
			if (strcmp(mime, GNOME_VFS_MIME_TYPE_UNKNOWN) == 0)
				node->mime_type = g_strdup("text/plain");
			else
				node->mime_type = g_strdup(mime);
		}
	}
	
	if (node->mime_type == NULL)
		node->mime_type = g_strdup (info->mime_type);

	if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_DIRECTORY;
	else if (node->mime_type && 
	         gnome_vfs_mime_type_get_equivalence (node->mime_type,
						      "text/plain") !=
		 GNOME_VFS_MIME_UNRELATED)
		node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_IS_TEXT;

	model_node_update_visibility (model, node);
	model_recomposite_icon_real (model, node);
}

static FileBrowserNode *
model_uri_exists (GeditFileBrowserStore * model, FileBrowserNode * parent,
		  GnomeVFSURI * uri)
{
	GSList *item;
	FileBrowserNode *node;

	if (!NODE_IS_DIR (parent))
		return NULL;

	for (item = FILE_BROWSER_NODE_DIR (parent)->children; item;
	     item = item->next) {
		node = (FileBrowserNode *) (item->data);

		if (node->uri != NULL
		    && gnome_vfs_uri_equal (node->uri, uri))
			return node;
	}

	return NULL;
}

static FileBrowserNode *
model_add_node_from_uri (GeditFileBrowserStore * model,
			 FileBrowserNode * parent, GnomeVFSURI * uri,
			 GnomeVFSFileInfo * info)
{
	FileBrowserNode *node;
	gboolean free_info = FALSE;

	// Check if it already exists
	if ((node = model_uri_exists (model, parent, uri)) == NULL) {
		if (info == NULL) {
			info = gnome_vfs_file_info_new ();
			free_info = TRUE;
			gnome_vfs_get_file_info_uri (uri, info,
						     GNOME_VFS_FILE_INFO_DEFAULT
						     |
						     GNOME_VFS_FILE_INFO_GET_MIME_TYPE);
		}

		if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
			node =
			    file_browser_node_dir_new (model, uri, parent);
		} else {
			node = file_browser_node_new (uri, parent);
		}

		file_browser_node_set_from_info (model, node, info);
		model_add_node (model, node, parent);

		if (free_info)
			gnome_vfs_file_info_unref (info);
	}

	return node;
}

static void
model_load_directory_cb (GnomeVFSAsyncHandle * handle,
			 GnomeVFSResult result, GList * list,
			 guint entries_read, gpointer user_data)
{
	FileBrowserNode *parent = (FileBrowserNode *) user_data;
	GeditFileBrowserStore *model =
	    FILE_BROWSER_NODE_DIR (parent)->model;
	GnomeVFSFileInfo *info;
	GnomeVFSURI *uri;
	FileBrowserNodeDir *dir;
	GList *item;

	if (result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF) {
		// Do something with these nodes!
		for (item = list; item; item = item->next) {
			info = (GnomeVFSFileInfo *) (item->data);

			// Skip all non regular, non directory files
			if (info->type != GNOME_VFS_FILE_TYPE_REGULAR &&
			    info->type != GNOME_VFS_FILE_TYPE_DIRECTORY &&
			    info->type !=
			    GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK)
				continue;

			// Skip '.' and '..' directories
			if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY &&
			    (strcmp (info->name, ".") == 0 ||
			     strcmp (info->name, "..") == 0))
				continue;

			// Create uri
			uri =
			    gnome_vfs_uri_append_path (parent->uri,
						       info->name);
			model_add_node_from_uri (model, parent, uri, info);
			gnome_vfs_uri_unref (uri);
		}

		if (result == GNOME_VFS_ERROR_EOF) {
			dir = FILE_BROWSER_NODE_DIR (parent);
			dir->load_handle = NULL;

			if (gnome_vfs_uri_is_local (parent->uri)
			    && dir->monitor_handle == NULL) {
				gnome_vfs_monitor_add (&
						       (dir->
							monitor_handle),
						       gnome_vfs_uri_get_path
						       (parent->uri),
						       GNOME_VFS_MONITOR_DIRECTORY,
						       (GnomeVFSMonitorCallback)
on_directory_monitor_event, parent);
			}

			model_end_loading (model, parent);
			model_check_dummy (model, parent);
		}
	} else {
		g_warning ("Error on loading directory: %s",
			   gnome_vfs_result_to_string (result));
		g_signal_emit (model, 
		               model_signals[ERROR], 
		               0,
		               GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY,
		               gnome_vfs_result_to_string (result));

		file_browser_node_unload (model, parent, TRUE);
	}
}

static void
model_load_directory (GeditFileBrowserStore * model,
		      FileBrowserNode * node)
{
	FileBrowserNodeDir *dir;

	g_return_if_fail (NODE_IS_DIR (node));

	dir = FILE_BROWSER_NODE_DIR (node);

	// Cancel a previous load
	if (dir->load_handle != NULL) {
		gnome_vfs_async_cancel (dir->load_handle);
		dir->load_handle = NULL;
	}

	node->flags |= GEDIT_FILE_BROWSER_STORE_FLAG_LOADED;
	model_begin_loading (model, node);

	// Start loading async
	gnome_vfs_async_load_directory_uri (&(dir->load_handle), node->uri,
					    GNOME_VFS_FILE_INFO_GET_MIME_TYPE
					    |
					    GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
					    100,
					    GNOME_VFS_PRIORITY_DEFAULT,
					    model_load_directory_cb, node);
}

static GList *
get_parent_uris (GeditFileBrowserStore * model, GnomeVFSURI * uri)
{
	GList *result = NULL;

	result = g_list_prepend (result, gnome_vfs_uri_ref (uri));

	while (gnome_vfs_uri_has_parent (uri)) {
		uri = gnome_vfs_uri_get_parent (uri);

		if (gnome_vfs_uri_equal (uri, model->priv->root->uri)) {
			gnome_vfs_uri_unref (uri);
			break;
		}

		result = g_list_prepend (result, uri);
	}

	return result;
}

static void
model_fill (GeditFileBrowserStore * model, FileBrowserNode * node,
	    GtkTreePath * path)
{
	gboolean free_path = FALSE;
	GtkTreeIter iter;
	GSList *item;
	FileBrowserNode *child;

	if (node == NULL) {
		node = model->priv->virtual_root;
		path = gtk_tree_path_new ();
		free_path = TRUE;
	}

	if (path == NULL) {
		path =
		    gedit_file_browser_store_get_path_real (model, node);
		free_path = TRUE;
	}

	if (!model_node_visibility (model, node)) {
		if (free_path)
			gtk_tree_path_free (path);

		return;
	}

	if (node != model->priv->virtual_root) {
		/* Insert node */
		iter.user_data = node;
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path,
					     &iter);
		model_check_dummy (model, node);
	}

	if (NODE_IS_DIR (node)) {
		/* Go to the first child */
		gtk_tree_path_down (path);

		for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
		     item = item->next) {
			child = (FileBrowserNode *) (item->data);

			if (model_node_visibility (model, child)) {
				model_fill (model, child, path);

				/* Increase path for next child */
				gtk_tree_path_next (path);
			}
		}

		/* Move back up to node path */
		gtk_tree_path_up (path);
	}

	if (free_path)
		gtk_tree_path_free (path);
}

static void
print_tree (GeditFileBrowserStore * model, FileBrowserNode * parent,
	    gchar * prefix)
{
	GSList *item;
	gchar *newpref;

	if (parent == NULL)
		parent = model->priv->root;

	if (parent == NULL)
		return;

	if (prefix == NULL)
		prefix = "";

	g_message ("%s - %s (%d), loaded: %d", prefix, parent->name,
		   model_node_visibility (model, parent),
		   NODE_LOADED (parent));

	if (NODE_IS_DIR (parent)) {
		newpref = g_strdup_printf ("\t%s", prefix);

		for (item = FILE_BROWSER_NODE_DIR (parent)->children; item;
		     item = item->next) {
			print_tree (model,
				    (FileBrowserNode *) (item->data),
				    newpref);
		}

		g_free (newpref);
	}
}

static void
set_virtual_root_from_node (GeditFileBrowserStore * model,
			    FileBrowserNode * node)
{
	FileBrowserNode *next;
	FileBrowserNode *prev;
	FileBrowserNode *check;
	FileBrowserNodeDir *dir;
	GSList *item;
	GSList *copy;

	prev = node;
	next = prev->parent;

	/* Free all the nodes below that we don't need in cache */
	while (prev != model->priv->root) {
		dir = FILE_BROWSER_NODE_DIR (next);
		copy = g_slist_copy (dir->children);

		for (item = copy; item; item = item->next) {
			check = (FileBrowserNode *) (item->data);

			if (prev == node) {
				/* Only free the children, keeping this depth in cache */
				if (check != node) {
					file_browser_node_free_children
					    (model, check);
					file_browser_node_unload (model,
								  check,
								  FALSE);
				}
			} else if (check != prev) {
				/* Only free when the node is not in the chain */
				dir->children =
				    g_slist_remove (dir->children, check);
				file_browser_node_free (model, check);
			}
		}

		if (prev != node)
			file_browser_node_unload (model, next, FALSE);

		g_slist_free (copy);
		prev = next;
		next = prev->parent;
	}

	/* Free all the nodes up that we don't need in cache */
	for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
	     item = item->next) {
		check = (FileBrowserNode *) (item->data);

		if (NODE_IS_DIR (check)) {
			for (copy =
			     FILE_BROWSER_NODE_DIR (check)->children; copy;
			     copy = copy->next) {
				file_browser_node_free_children (model,
								 (FileBrowserNode
								  *)
								 (item->
								  data));
				file_browser_node_unload (model,
							  (FileBrowserNode
							   *) (item->data),
							  FALSE);
			}
		} else if (NODE_IS_DUMMY (check)) {
			check->flags |=
			    GEDIT_FILE_BROWSER_STORE_FLAG_IS_HIDDEN;
		}
	}

	/* Now finally, set the virtual root, and load it up! */
	model->priv->virtual_root = node;
	model_fill (model, NULL, NULL);

	if (!NODE_LOADED (node))
		model_load_directory (model, node);

	g_object_notify (G_OBJECT (model), "virtual-root");
}

static void
set_virtual_root_from_uri (GeditFileBrowserStore * model,
			   GnomeVFSURI * uri)
{
	GList *uris;
	GList *item;
	FileBrowserNode *node;
	FileBrowserNode *parent;
	GnomeVFSURI *check;
	gboolean created = FALSE;
	GnomeVFSFileInfo *info;

	/* Always clear the model before altering the nodes */
	model_clear (model, FALSE);

	/* Create the node path, get all the uri's */
	uris = get_parent_uris (model, uri);
	parent = model->priv->root;
	node = NULL;

	for (item = uris; item; item = item->next) {
		check = (GnomeVFSURI *) (item->data);
		node = NULL;

		if (!created)
			node = model_uri_exists (model, parent, check);

		if (node == NULL) {
			// Create the node
			node =
			    file_browser_node_dir_new (model, check,
						       parent);

			info = gnome_vfs_file_info_new ();
			gnome_vfs_get_file_info_uri (check, info,
						     GNOME_VFS_FILE_INFO_DEFAULT
						     |
						     GNOME_VFS_FILE_INFO_GET_MIME_TYPE);
			file_browser_node_set_from_info (model, node,
							 info);
			gnome_vfs_file_info_unref (info);

			model_add_node (model, node, parent);
			created = TRUE;
		}

		parent = node;
		gnome_vfs_uri_unref (check);
	}

	g_list_free (uris);

	set_virtual_root_from_node (model, parent);
}

static int
progress_update_callback (GnomeVFSAsyncHandle * handle,
			  GnomeVFSXferProgressInfo * progress_info,
			  gpointer data)
{
	AsyncHandle *ahandle;

	ahandle = (AsyncHandle *) (data);

	switch (progress_info->status) {
	case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		if (ahandle->alive)
			g_signal_emit (ahandle->model,
				       model_signals[ERROR], 0,
				       GEDIT_FILE_BROWSER_ERROR_DELETE,
				       gnome_vfs_result_to_string
				       (progress_info->vfs_status));

		return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
		break;
	case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
		switch (progress_info->phase) {
		case GNOME_VFS_XFER_PHASE_COMPLETED:
			if (ahandle->alive) {
				/* Delete the file */
				model_remove_node (ahandle->model,
						   (FileBrowserNode
						    *) (ahandle->
							user_data), NULL,
						   TRUE);
				ahandle->model->priv->async_handles =
				    g_slist_remove (ahandle->model->priv->
						    async_handles,
						    ahandle);
			}

			g_free (ahandle);
			break;
		default:
			break;
		}
	default:
		break;
	}

	return 1;
}

static int
progress_sync_callback (GnomeVFSXferProgressInfo * progress_info,
			gpointer data)
{
	return 1;
}

static GQuark
gedit_file_browser_store_error_quark (void)
{
	static GQuark quark = 0;

	if (G_UNLIKELY (quark == 0))
		quark =
		    g_quark_from_static_string
		    ("gedit_file_browser_store_error");

	return quark;
}

static GnomeVFSURI *
unique_new_name (GnomeVFSURI * uri, gchar const *name)
{
	GnomeVFSURI *newuri = NULL;
	gchar *newname;
	guint num = 0;

	while (newuri == NULL || gnome_vfs_uri_exists (newuri)) {
		if (newuri != NULL)
			gnome_vfs_uri_unref (newuri);

		if (num == 0)
			newname = g_strdup (name);
		else
			newname = g_strdup_printf ("%s(%d)", name, num);

		newuri = gnome_vfs_uri_append_file_name (uri, newname);

		++num;
	}

	return newuri;
}

static GnomeVFSURI *
append_basename (GnomeVFSURI * target_uri, GnomeVFSURI * uri)
{
	gchar *basename;
	GnomeVFSURI *ret;

	basename = gnome_vfs_uri_extract_short_name (uri);
	ret = gnome_vfs_uri_append_file_name (target_uri, basename);
	g_free (basename);

	return ret;
}

/* Public */
GeditFileBrowserStore *
gedit_file_browser_store_new (gchar const *root)
{
	GeditFileBrowserStore *obj =
	    GEDIT_FILE_BROWSER_STORE (g_object_new
				      (GEDIT_TYPE_FILE_BROWSER_STORE,
				       NULL));

	gedit_file_browser_store_set_root (obj, root);
	return obj;
}

void
gedit_file_browser_store_set_value (GeditFileBrowserStore * tree_model,
				    GtkTreeIter * iter, gint column,
				    GValue * value)
{
	gpointer data;
	FileBrowserNode *node;
	GtkTreePath *path;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_model));
	g_return_if_fail (column ==
			  GEDIT_FILE_BROWSER_STORE_COLUMN_EMBLEM);
	g_return_if_fail (G_VALUE_HOLDS_OBJECT (value));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->user_data != NULL);

	data = g_value_get_object (value);

	if (data)
		g_return_if_fail (GDK_IS_PIXBUF (data));

	node = (FileBrowserNode *) (iter->user_data);

	if (node->emblem)
		g_object_unref (node->emblem);

	if (data)
		node->emblem = g_object_ref (GDK_PIXBUF (data));
	else
		node->emblem = NULL;

	model_recomposite_icon (tree_model, iter);

	if (model_node_visibility (tree_model, node)) {
		path =
		    gedit_file_browser_store_get_path (GTK_TREE_MODEL
						       (tree_model), iter);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (tree_model),
					    path, iter);
		gtk_tree_path_free (path);
	}
}

GeditFileBrowserStoreResult
gedit_file_browser_store_set_virtual_root (GeditFileBrowserStore * model,
					   GtkTreeIter * iter)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model),
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);
	g_return_val_if_fail (iter != NULL,
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);
	g_return_val_if_fail (iter->user_data != NULL,
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);

	model_clear (model, FALSE);
	set_virtual_root_from_node (model,
				    (FileBrowserNode *) (iter->user_data));

	return TRUE;
}

GeditFileBrowserStoreResult
    gedit_file_browser_store_set_virtual_root_from_string
    (GeditFileBrowserStore * model, gchar const *root) {
	GnomeVFSURI *uri = gnome_vfs_uri_new (root);
	gchar *str, *str1;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model),
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);

	if (uri == NULL) {
		g_warning ("Invalid uri (%s)", root);
		return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;
	}

	/* Check if uri is already the virtual root */
	if (model->priv->virtual_root &&
	    gnome_vfs_uri_equal (model->priv->virtual_root->uri, uri)) {
		gnome_vfs_uri_unref (uri);
		return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;
	}

	/* Check if uri is the root itself */
	if (gnome_vfs_uri_equal (model->priv->root->uri, uri)) {
		gnome_vfs_uri_unref (uri);

		/* Always clear the model before altering the nodes */
		model_clear (model, FALSE);
		set_virtual_root_from_node (model, model->priv->root);
		return GEDIT_FILE_BROWSER_STORE_RESULT_OK;
	}

	if (!gnome_vfs_uri_is_parent (model->priv->root->uri, uri, TRUE)) {
		str =
		    gnome_vfs_uri_to_string (model->priv->root->uri,
					     GNOME_VFS_URI_HIDE_PASSWORD);
		str1 =
		    gnome_vfs_uri_to_string (uri,
					     GNOME_VFS_URI_HIDE_PASSWORD);
		g_warning
		    ("Virtual root (%s) is not below actual root (%s)",
		     str1, str);
		g_free (str);
		g_free (str1);

		gnome_vfs_uri_unref (uri);
		return GEDIT_FILE_BROWSER_STORE_RESULT_ERROR;
	}

	set_virtual_root_from_uri (model, uri);
	gnome_vfs_uri_unref (uri);

	return GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

GeditFileBrowserStoreResult
gedit_file_browser_store_set_virtual_root_top (GeditFileBrowserStore *
					       model)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model),
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);

	if (model->priv->virtual_root == model->priv->root)
		return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;

	model_clear (model, FALSE);
	set_virtual_root_from_node (model, model->priv->root);

	return GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

GeditFileBrowserStoreResult
gedit_file_browser_store_set_virtual_root_up (GeditFileBrowserStore *
					      model)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model),
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);

	if (model->priv->virtual_root == model->priv->root)
		return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;

	model_clear (model, FALSE);
	set_virtual_root_from_node (model,
				    model->priv->virtual_root->parent);

	return GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

gboolean
gedit_file_browser_store_get_iter_virtual_root (GeditFileBrowserStore *
						model, GtkTreeIter * iter)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	if (model->priv->virtual_root == NULL)
		return FALSE;

	iter->user_data = model->priv->virtual_root;
	return TRUE;
}

gboolean
gedit_file_browser_store_get_iter_root (GeditFileBrowserStore * model,
					GtkTreeIter * iter)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	if (model->priv->root == NULL)
		return FALSE;

	iter->user_data = model->priv->root;
	return TRUE;
}

gboolean
gedit_file_browser_store_iter_equal (GeditFileBrowserStore * model,
				     GtkTreeIter * iter1,
				     GtkTreeIter * iter2)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), FALSE);
	g_return_val_if_fail (iter1 != NULL, FALSE);
	g_return_val_if_fail (iter2 != NULL, FALSE);
	g_return_val_if_fail (iter1->user_data != NULL, FALSE);
	g_return_val_if_fail (iter2->user_data != NULL, FALSE);

	return (iter1->user_data == iter2->user_data);
}

GeditFileBrowserStoreResult
gedit_file_browser_store_set_root_and_virtual_root (GeditFileBrowserStore *
						    model,
						    gchar const *root,
						    gchar const *virtual)
{
	GnomeVFSURI *uri = NULL;
	GnomeVFSURI *vuri = NULL;
	FileBrowserNode *node;
	gboolean equal = FALSE;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model),
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);

	if (root == NULL && model->priv->root == NULL)
		return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;

	if (root != NULL) {
		uri = gnome_vfs_uri_new (root);

		if (uri == NULL) {
			g_signal_emit (model, model_signals[ERROR], 0,
				       GEDIT_FILE_BROWSER_ERROR_SET_ROOT,
				       _("Invalid uri"));
			return GEDIT_FILE_BROWSER_STORE_RESULT_ERROR;
		}
	}

	if (root != NULL && model->priv->root != NULL) {
		equal = gnome_vfs_uri_equal (uri, model->priv->root->uri);

		if (equal && virtual == NULL) {
			gnome_vfs_uri_unref (uri);
			return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;
		}
	}

	if (virtual) {
		vuri = gnome_vfs_uri_new (virtual);

		if (equal && model->priv->virtual_root &&
		    gnome_vfs_uri_equal (vuri,
					 model->priv->virtual_root->uri)) {
			if (uri)
				gnome_vfs_uri_unref (uri);

			gnome_vfs_uri_unref (vuri);
			return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;
		}

		gnome_vfs_uri_unref (vuri);
	}

	/* Always clear the model before altering the nodes */
	model_clear (model, TRUE);
	file_browser_node_free (model, model->priv->root);

	model->priv->root = NULL;
	model->priv->virtual_root = NULL;

	if (uri != NULL) {
		/* Create the root node */
		node = file_browser_node_dir_new (model, uri, NULL);
		gnome_vfs_uri_unref (uri);

		model->priv->root = node;
		model_check_dummy (model, node);
		
		g_object_notify (G_OBJECT (model), "root");

		if (virtual != NULL)
			return
			    gedit_file_browser_store_set_virtual_root_from_string
			    (model, virtual);
		else
			set_virtual_root_from_node (model,
						    model->priv->root);
	} else {
		g_object_notify (G_OBJECT (model), "root");
		g_object_notify (G_OBJECT (model), "virtual-root");
	}

	return GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

GeditFileBrowserStoreResult
gedit_file_browser_store_set_root (GeditFileBrowserStore * model,
				   gchar const *root)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model),
			      GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);
	return gedit_file_browser_store_set_root_and_virtual_root (model,
								   root,
								   NULL);
}

gchar *
gedit_file_browser_store_get_root (GeditFileBrowserStore * model)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), NULL);

	return gnome_vfs_uri_to_string (model->priv->root->uri,
					GNOME_VFS_URI_HIDE_NONE);
}

gchar * 
gedit_file_browser_store_get_virtual_root (GeditFileBrowserStore * model)
{
	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), NULL);
	
	return gnome_vfs_uri_to_string (model->priv->virtual_root->uri,
	                                GNOME_VFS_URI_HIDE_NONE);
}

void
_gedit_file_browser_store_iter_expanded (GeditFileBrowserStore * model,
					 GtkTreeIter * iter)
{
	FileBrowserNode *node;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->user_data != NULL);

	node = (FileBrowserNode *) (iter->user_data);

	if (NODE_IS_DIR (node) && !NODE_LOADED (node)) {
		/* Load it now */
		model_load_directory (model, node);
	}
}

void
_gedit_file_browser_store_iter_collapsed (GeditFileBrowserStore * model,
					  GtkTreeIter * iter)
{
	FileBrowserNode *node;
	GSList *item;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->user_data != NULL);

	node = (FileBrowserNode *) (iter->user_data);

	if (NODE_IS_DIR (node) && NODE_LOADED (node)) {
		/* Unload children of the children, keeping 1 depth in cache */

		for (item = FILE_BROWSER_NODE_DIR (node)->children; item;
		     item = item->next) {
			node = (FileBrowserNode *) (item->data);

			if (NODE_IS_DIR (node) && NODE_LOADED (node)) {
				file_browser_node_unload (model, node,
							  TRUE);
				model_check_dummy (model, node);
			}
		}
	}
}

GeditFileBrowserStoreFilterMode
gedit_file_browser_store_get_filter_mode (GeditFileBrowserStore * model)
{
	return model->priv->filter_mode;
}

void
gedit_file_browser_store_set_filter_mode (GeditFileBrowserStore * model,
					  GeditFileBrowserStoreFilterMode
					  mode)
{
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model));

	if (model->priv->filter_mode == mode)
		return;

	model->priv->filter_mode = mode;
	model_refilter (model);

	g_object_notify (G_OBJECT (model), "filter-mode");
}

void
gedit_file_browser_store_set_filter_func (GeditFileBrowserStore * model,
					  GeditFileBrowserStoreFilterFunc
					  func, gpointer user_data)
{
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model));

	model->priv->filter_func = func;
	model->priv->filter_user_data = user_data;
	model_refilter (model);
}

void
gedit_file_browser_store_refilter (GeditFileBrowserStore * model)
{
	model_refilter (model);
}

GeditFileBrowserStoreFilterMode
gedit_file_browser_store_filter_mode_get_default (void)
{
	return GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN;
}

void
gedit_file_browser_store_refresh (GeditFileBrowserStore * model)
{
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model));

	if (model->priv->root == NULL)
		return;

	/* Clear the model */
	file_browser_node_unload (model, model->priv->virtual_root, TRUE);
	model_load_directory (model, model->priv->virtual_root);
}

gboolean
gedit_file_browser_store_rename (GeditFileBrowserStore * model,
				 GtkTreeIter * iter, gchar const *new_name,
				 GError ** error)
{
	FileBrowserNode *node;
	GnomeVFSURI *uri;
	GnomeVFSURI *parent;
	GnomeVFSFileInfo *info;
	GtkTreePath *path;
	GnomeVFSResult ret;

	*error = NULL;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	node = (FileBrowserNode *) (iter->user_data);

	parent = gnome_vfs_uri_get_parent (node->uri);
	uri = gnome_vfs_uri_append_file_name (parent, new_name);
	gnome_vfs_uri_unref (parent);

	if (gnome_vfs_uri_equal (node->uri, uri)) {
		gnome_vfs_uri_unref (uri);
		return TRUE;
	}

	ret = gnome_vfs_move_uri (node->uri, uri, FALSE);

	if (ret == GNOME_VFS_OK) {
		parent = node->uri;
		node->uri = uri;
		info = gnome_vfs_file_info_new ();
		gnome_vfs_get_file_info_uri (uri, info,
					     GNOME_VFS_FILE_INFO_DEFAULT |
					     GNOME_VFS_FILE_INFO_GET_MIME_TYPE);

		file_browser_node_set_from_info (model, node, info);
		file_browser_node_set_name (node);
		gnome_vfs_file_info_unref (info);
		gnome_vfs_uri_unref (parent);

		path =
		    gedit_file_browser_store_get_path_real (model, node);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path,
					    iter);
		gtk_tree_path_free (path);

		/* Reorder this item */
		model_resort_node (model, node);

		return TRUE;
	} else {
		gnome_vfs_uri_unref (uri);

		if (error != NULL) {
			*error =
			    g_error_new_literal
			    (gedit_file_browser_store_error_quark (),
			     GEDIT_FILE_BROWSER_ERROR_RENAME,
			     gnome_vfs_result_to_string (ret));
		}

		return FALSE;
	}
}

GeditFileBrowserStoreResult
gedit_file_browser_store_delete (GeditFileBrowserStore * model,
				 GtkTreeIter * iter, gboolean trash)
{
	FileBrowserNode *node;
	AsyncHandle *handle;
	GList *uris;
	GList *target = NULL;
	GnomeVFSURI *trash_uri;
	GnomeVFSResult ret;
	GnomeVFSXferOptions options;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);
	g_return_val_if_fail (iter != NULL, GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);
	g_return_val_if_fail (iter->user_data != NULL, GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE);

	node = (FileBrowserNode *) (iter->user_data);

	if (NODE_IS_DUMMY (node))
		return GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE;

	uris = g_list_append (NULL, node->uri);
	handle = g_new (AsyncHandle, 1);
	handle->model = model;
	handle->user_data = node;
	handle->alive = TRUE;

	if (trash) {
		/* Find the trash */
		ret =
		    gnome_vfs_find_directory (node->uri,
					      GNOME_VFS_DIRECTORY_KIND_TRASH,
					      &trash_uri, FALSE, TRUE,
					      0777);

		if (ret == GNOME_VFS_ERROR_NOT_FOUND || trash_uri == NULL) {
			g_list_free (uris);
			g_free (handle);
			return GEDIT_FILE_BROWSER_STORE_RESULT_NO_TRASH;
		} else {
			target =
			    g_list_append (NULL,
					   append_basename (trash_uri,
							    node->uri));
			gnome_vfs_uri_unref (trash_uri);

			options =
			    GNOME_VFS_XFER_RECURSIVE |
			    GNOME_VFS_XFER_REMOVESOURCE;
		}
	} else {
		options =
		    GNOME_VFS_XFER_DELETE_ITEMS | GNOME_VFS_XFER_RECURSIVE;
	}

	gnome_vfs_async_xfer (&(handle->handle), uris, target,
			      options,
			      GNOME_VFS_XFER_ERROR_MODE_QUERY,
			      GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
			      GNOME_VFS_PRIORITY_DEFAULT,
			      progress_update_callback,
			      handle, progress_sync_callback, handle);

	model->priv->async_handles =
	    g_slist_prepend (model->priv->async_handles, handle);

	g_list_free (uris);
	
	return GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

gboolean
gedit_file_browser_store_new_file (GeditFileBrowserStore * model,
				   GtkTreeIter * parent,
				   GtkTreeIter * iter)
{
	GnomeVFSURI *uri;
	GnomeVFSHandle *handle;
	GnomeVFSResult ret;
	FileBrowserNodeDir *parent_node;
	gboolean result = FALSE;
	FileBrowserNode *node;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), FALSE);
	g_return_val_if_fail (parent != NULL, FALSE);
	g_return_val_if_fail (parent->user_data != NULL, FALSE);
	g_return_val_if_fail (NODE_IS_DIR
			      ((FileBrowserNode *) (parent->user_data)),
			      FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	parent_node = FILE_BROWSER_NODE_DIR (parent->user_data);
	uri = unique_new_name (((FileBrowserNode *) parent_node)->uri, _("file"));

	ret = gnome_vfs_create_uri (&handle, uri,
				    GNOME_VFS_OPEN_NONE |
				    GNOME_VFS_OPEN_WRITE, FALSE, 0644);

	if (ret != GNOME_VFS_OK) {
		g_signal_emit (model, model_signals[ERROR], 0,
			       GEDIT_FILE_BROWSER_ERROR_NEW_FILE,
			       gnome_vfs_result_to_string (ret));
	} else {
		node = model_add_node_from_uri (model, (FileBrowserNode *)
						parent_node, uri, NULL);

		if (model_node_visibility (model, node)) {
			iter->user_data = node;
			result = TRUE;
		} else {
			g_signal_emit (model, model_signals[ERROR], 0,
				       GEDIT_FILE_BROWSER_ERROR_NEW_FILE,
				       _
				       ("The new file is currently filtered out. You need to adjust your filter settings to make the file visible"));
		}
	}

	gnome_vfs_uri_unref (uri);
	return result;
}

gboolean
gedit_file_browser_store_new_directory (GeditFileBrowserStore * model,
					GtkTreeIter * parent,
					GtkTreeIter * iter)
{
	GnomeVFSURI *uri;
	GnomeVFSResult ret;
	FileBrowserNodeDir *parent_node;
	gboolean result = FALSE;
	FileBrowserNode *node;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_STORE (model), FALSE);
	g_return_val_if_fail (parent != NULL, FALSE);
	g_return_val_if_fail (parent->user_data != NULL, FALSE);
	g_return_val_if_fail (NODE_IS_DIR
			      ((FileBrowserNode *) (parent->user_data)),
			      FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	parent_node = FILE_BROWSER_NODE_DIR (parent->user_data);
	uri = unique_new_name (((FileBrowserNode *) parent_node)->uri, _("directory"));

	ret = gnome_vfs_make_directory_for_uri (uri, 0755);

	if (ret != GNOME_VFS_OK) {
		g_signal_emit (model, model_signals[ERROR], 0,
			       GEDIT_FILE_BROWSER_ERROR_NEW_DIRECTORY,
			       gnome_vfs_result_to_string (ret));
	} else {
		node = model_add_node_from_uri (model, (FileBrowserNode *)
						parent_node, uri, NULL);

		if (model_node_visibility (model, node)) {
			iter->user_data = node;
			result = TRUE;
		} else {
			g_signal_emit (model, model_signals[ERROR], 0,
				       GEDIT_FILE_BROWSER_ERROR_NEW_FILE,
				       _
				       ("The new file is currently filtered out. You need to adjust your filter settings to make the file visible"));
		}
	}

	gnome_vfs_uri_unref (uri);
	return result;
}

/* Signal handlers */
static void
on_directory_monitor_event (GnomeVFSMonitorHandle * handle,
			    gchar const *monitor_uri,
			    const gchar * info_uri,
			    GnomeVFSMonitorEventType event_type,
			    FileBrowserNode * parent)
{
	FileBrowserNode *node;
	FileBrowserNodeDir *dir = FILE_BROWSER_NODE_DIR (parent);
	GnomeVFSURI *uri;

	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		uri = gnome_vfs_uri_new (info_uri);
		node = model_uri_exists (dir->model, parent, uri);
		gnome_vfs_uri_unref (uri);

		if (node != NULL) {
			// Remove the node
			model_remove_node (dir->model, node, NULL, TRUE);
		}
		break;
	case GNOME_VFS_MONITOR_EVENT_CREATED:
		uri = gnome_vfs_uri_new (info_uri);

		model_add_node_from_uri (dir->model, parent, uri, NULL);
		gnome_vfs_uri_unref (uri);
		break;
	default:
		break;
	}
}

// ex:ts=8:noet:
