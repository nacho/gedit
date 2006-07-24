/*
 * gedit-file-browser-view.c - Gedit plugin providing easy file access 
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

#include <string.h>
#include <gedit/gedit-plugin.h>

#include "gedit-file-browser-store.h"
#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-view.h"
#include "gedit-file-browser-marshal.h"

#define GEDIT_FILE_BROWSER_VIEW_GET_PRIVATE(object)( \
		G_TYPE_INSTANCE_GET_PRIVATE((object), \
		GEDIT_TYPE_FILE_BROWSER_VIEW, GeditFileBrowserViewPrivate))

struct _GeditFileBrowserViewPrivate 
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *pixbuf_renderer;
	GtkCellRenderer *text_renderer;

	GtkTreeModel *model;
	GtkTreePath *editable;
};

/* Signals */
enum 
{
	ERROR,
	NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0 };

GEDIT_PLUGIN_DEFINE_TYPE (GeditFileBrowserView, gedit_file_browser_view,
	                  GTK_TYPE_TREE_VIEW)

static void
on_cell_edited (GtkCellRendererText * cell, gchar * path,
		gchar * new_text, GeditFileBrowserView * tree_view);

static void
gedit_file_browser_view_finalize (GObject * object)
{
	//GeditFileBrowserView *obj = GEDIT_FILE_BROWSER_VIEW(object);

	G_OBJECT_CLASS (gedit_file_browser_view_parent_class)->
	    finalize (object);
}

typedef struct _IdleInfo 
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
} IdleInfo;

static gboolean
row_expanded_idle (gpointer data)
{
	IdleInfo *info = (IdleInfo *) data;

	_gedit_file_browser_store_iter_expanded (GEDIT_FILE_BROWSER_STORE
						 (info->model),
						 &(info->iter));

	g_free (info);
	return FALSE;
}

static gboolean
row_collapsed_idle (gpointer data)
{
	IdleInfo *info = (IdleInfo *) data;

	_gedit_file_browser_store_iter_collapsed (GEDIT_FILE_BROWSER_STORE
						  (info->model),
						  &(info->iter));

	g_free (info);
	return FALSE;
}

static void
gedit_file_browser_view_row_expanded (GtkTreeView * tree_view,
				      GtkTreeIter * iter,
				      GtkTreePath * path)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (tree_view);
	IdleInfo *info;

	if (GEDIT_IS_FILE_BROWSER_STORE (view->priv->model)) {
		info = g_new (IdleInfo, 1);
		info->model = view->priv->model;
		info->iter = *iter;

		g_idle_add (row_expanded_idle, info);
	}

	if (GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->
	    row_expanded)
		GTK_TREE_VIEW_CLASS
		    (gedit_file_browser_view_parent_class)->
		    row_expanded (tree_view, iter, path);
}

static void
gedit_file_browser_view_row_collapsed (GtkTreeView * tree_view,
				       GtkTreeIter * iter,
				       GtkTreePath * path)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (tree_view);
	IdleInfo *info;

	if (GEDIT_IS_FILE_BROWSER_STORE (view->priv->model)) {
		info = g_new (IdleInfo, 1);
		info->model = view->priv->model;
		info->iter = *iter;

		g_idle_add (row_collapsed_idle, info);
	}

	if (GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->
	    row_collapsed)
		GTK_TREE_VIEW_CLASS
		    (gedit_file_browser_view_parent_class)->
		    row_collapsed (tree_view, iter, path);
}

static void
gedit_file_browser_view_row_activated (GtkTreeView * tree_view,
				       GtkTreePath * path,
				       GtkTreeViewColumn * column)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (tree_view);
	GtkTreeIter iter;
	guint flags;

	if (GEDIT_IS_FILE_BROWSER_STORE (view->priv->model)) {
		if (gtk_tree_model_get_iter
		    (view->priv->model, &iter, path)) {
			gtk_tree_model_get (view->priv->model, &iter,
					    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS,
					    &flags, -1);

			if (FILE_IS_DIR (flags)) {
				gedit_file_browser_store_set_virtual_root
				    (GEDIT_FILE_BROWSER_STORE
				     (view->priv->model), &iter);
			}
		}
	}

	if (GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->
	    row_activated)
		GTK_TREE_VIEW_CLASS
		    (gedit_file_browser_view_parent_class)->
		    row_activated (tree_view, path, column);
}

static void
gedit_file_browser_view_class_init (GeditFileBrowserViewClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTreeViewClass *tree_view_class = GTK_TREE_VIEW_CLASS (klass);

	object_class->finalize = gedit_file_browser_view_finalize;

	tree_view_class->row_expanded =
	    gedit_file_browser_view_row_expanded;
	tree_view_class->row_collapsed =
	    gedit_file_browser_view_row_collapsed;
	tree_view_class->row_activated =
	    gedit_file_browser_view_row_activated;

	signals[ERROR] =
	    g_signal_new ("error",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserViewClass,
					   error), NULL, NULL,
			  gedit_file_browser_marshal_VOID__UINT_STRING,
			  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

	g_type_class_add_private (object_class,
				  sizeof (GeditFileBrowserViewPrivate));
}

static void
cell_data_cb (GtkTreeViewColumn * tree_column, GtkCellRenderer * cell,
	      GtkTreeModel * tree_model, GtkTreeIter * iter,
	      GeditFileBrowserView * obj)
{
	GtkTreePath *path;

	if (!GEDIT_IS_FILE_BROWSER_STORE (tree_model))
		return;

	if (obj->priv->editable != NULL) {
		path = gtk_tree_model_get_path (tree_model, iter);

		g_object_set (cell, "editable",
			      (gtk_tree_path_compare
			       (path, obj->priv->editable) == 0), NULL);
		gtk_tree_path_free (path);
	} else {
		g_object_set (cell, "editable", FALSE, NULL);
	}
}

static void
gedit_file_browser_view_init (GeditFileBrowserView * obj)
{
	obj->priv = GEDIT_FILE_BROWSER_VIEW_GET_PRIVATE (obj);

	obj->priv->column = gtk_tree_view_column_new ();

	obj->priv->pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (obj->priv->column,
					 obj->priv->pixbuf_renderer,
					 FALSE);
	gtk_tree_view_column_add_attribute (obj->priv->column,
					    obj->priv->pixbuf_renderer,
					    "pixbuf",
					    GEDIT_FILE_BROWSER_STORE_COLUMN_ICON);

	obj->priv->text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (obj->priv->column,
					 obj->priv->text_renderer, TRUE);
	gtk_tree_view_column_add_attribute (obj->priv->column,
					    obj->priv->text_renderer,
					    "text",
					    GEDIT_FILE_BROWSER_STORE_COLUMN_NAME);

	g_signal_connect (obj->priv->text_renderer, "edited",
			  G_CALLBACK (on_cell_edited), obj);

	gtk_tree_view_append_column (GTK_TREE_VIEW (obj),
				     obj->priv->column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (obj), FALSE);
}

static gboolean
bookmarks_separator_func (GtkTreeModel * model, GtkTreeIter * iter,
			  gpointer user_data)
{
	guint flags;

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
			    &flags, -1);

	return (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR);
}

/* Public */
GtkWidget *
gedit_file_browser_view_new ()
{
	GeditFileBrowserView *obj =
	    GEDIT_FILE_BROWSER_VIEW (g_object_new
				     (GEDIT_TYPE_FILE_BROWSER_VIEW, NULL));

	return GTK_WIDGET (obj);
}

void
gedit_file_browser_view_set_model (GeditFileBrowserView * tree_view,
				   GtkTreeModel * model)
{
	if (tree_view->priv->model == model)
		return;

	if (GEDIT_IS_FILE_BOOKMARKS_STORE (model)) {
		gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW
						      (tree_view),
						      bookmarks_separator_func,
						      NULL, NULL);
		gtk_tree_view_column_set_cell_data_func (tree_view->priv->
							 column,
							 tree_view->priv->
							 text_renderer,
							 NULL, tree_view,
							 NULL);
	} else {
		gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW
						      (tree_view), NULL,
						      NULL, NULL);
		gtk_tree_view_column_set_cell_data_func (tree_view->priv->
							 column,
							 tree_view->priv->
							 text_renderer,
							 (GtkTreeCellDataFunc)
							 cell_data_cb,
							 tree_view, NULL);
	}

	tree_view->priv->model = model;
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), model);
}

void
gedit_file_browser_view_start_rename (GeditFileBrowserView * tree_view,
				      GtkTreeIter * iter)
{
	guint flags;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_VIEW (tree_view));
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE
			  (tree_view->priv->model));
	g_return_if_fail (iter != NULL);

	gtk_tree_model_get (tree_view->priv->model, iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (FILE_IS_DIR (flags) || !FILE_IS_DUMMY (flags)) {
		tree_view->priv->editable =
		    gtk_tree_model_get_path (tree_view->priv->model, iter);

		/* Start editing */
		gtk_widget_grab_focus (GTK_WIDGET (tree_view));
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree_view),
					      tree_view->priv->editable);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree_view),
					  tree_view->priv->editable,
					  tree_view->priv->column, TRUE);
	}
}

/* Signal handlers */
static void
on_cell_edited (GtkCellRendererText * cell, gchar * path, gchar * new_text,
		GeditFileBrowserView * tree_view)
{
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GError *error = NULL;

	gtk_tree_path_free (tree_view->priv->editable);
	tree_view->priv->editable = NULL;

	if (new_text == NULL || *new_text == '\0')
		return;

	treepath = gtk_tree_path_new_from_string (path);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_view->priv->model),
				 &iter, treepath);
	gtk_tree_path_free (treepath);

	if (!gedit_file_browser_store_rename
	    (GEDIT_FILE_BROWSER_STORE (tree_view->priv->model), &iter,
	     new_text, &error)) {
		if (error) {
			g_signal_emit (tree_view, signals[ERROR], 0,
				       error->code, error->message);
			g_error_free (error);
		}
	}
}

// ex:ts=8:noet:
