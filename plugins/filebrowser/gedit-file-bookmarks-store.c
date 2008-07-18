/*
 * gedit-file-bookmarks-store.c - Gedit plugin providing easy file access 
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
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-plugin.h>

#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-utils.h"

#define GEDIT_FILE_BOOKMARKS_STORE_GET_PRIVATE(object)( \
		G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BOOKMARKS_STORE, \
		GeditFileBookmarksStorePrivate))

struct _GeditFileBookmarksStorePrivate 
{
	GVolumeMonitor * volume_monitor;
	GFileMonitor * bookmarks_monitor;
};

static void remove_node               (GtkTreeModel * model, 
                                       GtkTreeIter * iter);

static void on_mount_added            (GVolumeMonitor * monitor,
                                       GMount * mount,
                                       GeditFileBookmarksStore * model);
static void on_mount_removed          (GVolumeMonitor * monitor,
                                       GMount * mount,
                                       GeditFileBookmarksStore * model);
static void on_bookmarks_file_changed (GFileMonitor * monitor,
				       GFile * file,
				       GFile * other_file,
				       GFileMonitorEvent event_type,
				       GeditFileBookmarksStore * model);
static gboolean find_with_flags       (GtkTreeModel * model, 
                                       GtkTreeIter * iter,
                                       gpointer obj, 
                                       guint flags,
                                       guint notflags);

GEDIT_PLUGIN_DEFINE_TYPE(GeditFileBookmarksStore, gedit_file_bookmarks_store, GTK_TYPE_TREE_STORE)

static void
gedit_file_bookmarks_store_dispose (GObject * object)
{
	GeditFileBookmarksStore *obj = GEDIT_FILE_BOOKMARKS_STORE (object);

	if (obj->priv->volume_monitor != NULL) {
		g_signal_handlers_disconnect_by_func (obj->priv->volume_monitor,
						      on_mount_added,
						      obj);
		g_signal_handlers_disconnect_by_func (obj->priv->volume_monitor,
						      on_mount_removed,
						      obj);
		g_object_unref (obj->priv->volume_monitor);
	}

	if (obj->priv->bookmarks_monitor != NULL) {
		g_object_unref (obj->priv->bookmarks_monitor);
	}

	G_OBJECT_CLASS (gedit_file_bookmarks_store_parent_class)->dispose (object);
}

static void
gedit_file_bookmarks_store_finalize (GObject * object)
{
	G_OBJECT_CLASS (gedit_file_bookmarks_store_parent_class)->finalize (object);
}

static void
gedit_file_bookmarks_store_class_init (GeditFileBookmarksStoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_file_bookmarks_store_dispose;
	object_class->finalize = gedit_file_bookmarks_store_finalize;

	g_type_class_add_private (object_class, sizeof (GeditFileBookmarksStorePrivate));
}

static void
gedit_file_bookmarks_store_init (GeditFileBookmarksStore * obj)
{
	obj->priv = GEDIT_FILE_BOOKMARKS_STORE_GET_PRIVATE (obj);
}

/* Private */
static void
add_node (GeditFileBookmarksStore * model, GdkPixbuf * pixbuf,
	  gchar const *name, gpointer obj, guint flags, GtkTreeIter * iter)
{
	GtkTreeIter newiter;

	gtk_tree_store_append (GTK_TREE_STORE (model), &newiter, NULL);

	gtk_tree_store_set (GTK_TREE_STORE (model), &newiter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON, pixbuf,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_NAME, name,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT, obj,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS, flags,
			    -1);

	if (iter != NULL)
		*iter = newiter;
}

static gboolean
add_file (GeditFileBookmarksStore * model, GFile * file,
	  gchar const * name, guint flags, GtkTreeIter * iter)
{
	GdkPixbuf * pixbuf = NULL;
	gboolean native;
	gchar * newname;

	native = g_file_is_native (file);

	if (native && !g_file_query_exists (file, NULL)) {
		return FALSE;
	}

	if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_HOME)
		pixbuf = gedit_file_browser_utils_pixbuf_from_theme ("user-home", GTK_ICON_SIZE_MENU);
	else if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_DESKTOP)
		pixbuf = gedit_file_browser_utils_pixbuf_from_theme ("user-desktop", GTK_ICON_SIZE_MENU);
	else if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_ROOT)
		pixbuf = gedit_file_browser_utils_pixbuf_from_theme ("drive-harddisk", GTK_ICON_SIZE_MENU);

	if (pixbuf == NULL) {
		pixbuf = gedit_file_browser_utils_pixbuf_from_file (file, GTK_ICON_SIZE_MENU);
	}

	if (name == NULL) {
		newname = gedit_file_browser_utils_file_basename (file);
	} else {
		newname = g_strdup (name);
	}

	add_node (model, pixbuf, newname, file, flags, iter);

	if (pixbuf)
		g_object_unref (pixbuf);

	g_free (newname);

	return TRUE;
}

static void
check_mount_separator (GeditFileBookmarksStore * model, guint flags,
			gboolean added)
{
	GtkTreeIter iter;
	gboolean found;

	found =
	    find_with_flags (GTK_TREE_MODEL (model), &iter, NULL,
			     flags |
			     GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR, 0);

	if (added && !found) {
		/* Add the separator */
		add_node (model, NULL, NULL, NULL,
			  flags | GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR,
			  NULL);
	} else if (!added && found) {
		remove_node (GTK_TREE_MODEL (model), &iter);
	}
}

static void
init_special_directories (GeditFileBookmarksStore * model)
{
	gchar const *path;
	GFile * file;

	path = g_get_home_dir ();
	if (path != NULL)
	{
		file = g_file_new_for_path (path);
		add_file (model, file, NULL, GEDIT_FILE_BOOKMARKS_STORE_IS_HOME |
			 GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);
		g_object_unref (file);
	}

	path = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
	if (path != NULL)
	{
		file = g_file_new_for_path (path);
		add_file (model, file, NULL, GEDIT_FILE_BOOKMARKS_STORE_IS_DESKTOP |
			  GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);
		g_object_unref (file);
	}

	path = g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);
	if (path != NULL)
	{
		file = g_file_new_for_path (path);
		add_file (model, file, NULL, GEDIT_FILE_BOOKMARKS_STORE_IS_DOCUMENTS |
			 GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);
		g_object_unref (file);
	}
	
	file = g_file_new_for_uri ("file:///");
	add_file (model, file, _("File System"), GEDIT_FILE_BOOKMARKS_STORE_IS_ROOT, NULL);
	g_object_unref (file);
	
	check_mount_separator (model, GEDIT_FILE_BOOKMARKS_STORE_IS_ROOT, TRUE);
}

static void
add_mount (GeditFileBookmarksStore * model, GMount * mount,
	   const gchar * name, guint flags, GtkTreeIter * iter)
{
	GdkPixbuf *pixbuf = NULL;
	GIcon *icon;

	icon = g_mount_get_icon (mount);
	
	if (icon) {
		pixbuf = gedit_file_browser_utils_pixbuf_from_icon (icon, GTK_ICON_SIZE_MENU);
		g_object_unref (icon);
	}

	add_node (model, pixbuf, name, mount,
		  flags | GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT, iter);

	if (pixbuf)
		g_object_unref (pixbuf);

	check_mount_separator (model, GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT, TRUE);
}

static void
process_mount (GeditFileBookmarksStore * model, GMount * mount,
		gboolean * isroot)
{
	guint flags = GEDIT_FILE_BOOKMARKS_STORE_NONE;
	GFile * root;
	GFileInfo * info;
	gchar * name;
	gboolean local;

	root = g_mount_get_root (mount);
	
	if (!root)
		return;

	info = g_file_query_info (root, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, G_FILE_QUERY_INFO_NONE, NULL, NULL);

	/* Is this mount actually readable by the user */
	if (info && g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ)) {
		/* Here we set up the flags for the mount */
		local = g_file_is_native (root);
		
		if (local) {
			flags |= GEDIT_FILE_BOOKMARKS_STORE_IS_LOCAL_MOUNT;
		} else {
			flags |= GEDIT_FILE_BOOKMARKS_STORE_IS_REMOTE_MOUNT;
		}
		
		name = g_mount_get_name (mount);
		add_mount (model, mount, name, flags, NULL);
		g_free (name);
	}
	
	if (info)
		g_object_unref (info);
	
	g_object_unref (root);
}

static void
init_mounts (GeditFileBookmarksStore * model)
{
	GList * mounts;
	GList * item;
	gboolean root = FALSE;

	if (model->priv->volume_monitor == NULL) {
		model->priv->volume_monitor = g_volume_monitor_get ();

		/* Connect signals */
		g_signal_connect (model->priv->volume_monitor,
				  "mount-added",
				  G_CALLBACK (on_mount_added), model);
		g_signal_connect (model->priv->volume_monitor,
				  "mount-removed",
				  G_CALLBACK (on_mount_removed), model);
	}

	mounts = g_volume_monitor_get_mounts (model->priv->volume_monitor);

	for (item = mounts; item; item = item->next)
		process_mount (model, G_MOUNT(item->data), &root);

	g_list_foreach (mounts, (GFunc)g_object_unref, NULL);
	g_list_free (mounts);
}

static gboolean
add_bookmark (GeditFileBookmarksStore * model, 
	      gchar const * name,
	      gchar const * uri)
{
	GFile * file;
	gboolean ret;
	guint flags = GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK;
	GtkTreeIter iter;
	GdkPixbuf * pixbuf;
	
	file = g_file_new_for_uri (uri);
	
	if (g_file_is_native (file)) {
		flags |= GEDIT_FILE_BOOKMARKS_STORE_IS_LOCAL_BOOKMARK;
	} else {
		flags |= GEDIT_FILE_BOOKMARKS_STORE_IS_REMOTE_BOOKMARK;
	}

	ret = add_file (model, file, name, flags, &iter);
	
	if (!g_file_is_native (file)) {
		/* Check icon */
		gtk_tree_model_get (GTK_TREE_MODEL (model), 
				    &iter, 
				    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON, 
				    &pixbuf, 
				    -1);

		if (!pixbuf) {
			pixbuf = gedit_file_browser_utils_pixbuf_from_theme ("folder", GTK_ICON_SIZE_MENU);

			if (pixbuf) {			
				gtk_tree_store_set (GTK_TREE_STORE (model), 
						    &iter, 
						    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON,
						    pixbuf,
						    -1);
						    
				g_object_unref (pixbuf);
			}
		} else {
			g_object_unref (pixbuf);
		}
	}
	
	g_object_unref (file);
	
	return ret;
}

static void
init_bookmarks (GeditFileBookmarksStore * model)
{
	gchar *bookmarks;
	GError *error = NULL;
	gchar *contents;
	gchar **lines;
	gchar **line;
	gboolean added = FALSE;
	GFile * file;

	/* Read the bookmarks file */
	bookmarks = g_build_filename (g_get_home_dir (),
				      ".gtk-bookmarks",
				      NULL);

	if (g_file_get_contents (bookmarks, &contents, NULL, &error)) {
		lines = g_strsplit (contents, "\n", 0);

		for (line = lines; *line; ++line) {
			if (**line) {
				gchar *pos;
				gchar *name;

				/* CHECK: is this really utf8? */
				pos = g_utf8_strchr (*line, -1, ' ');

				if (pos != NULL) {
					*pos = '\0';
					name = pos + 1;
				} else {
					name = NULL;
				}

				/* the bookmarks file should contain valid
				 * URIs, but paranoia is good */
				if (gedit_utils_is_valid_uri (*line)) {
					added |= add_bookmark (model, name, *line);
				}
			}
		}

		g_strfreev (lines);
		g_free (contents);

		/* Add a watch */
		if (model->priv->bookmarks_monitor == NULL) {
			file = g_file_new_for_path (bookmarks);
			model->priv->bookmarks_monitor = g_file_monitor_file (file, G_FILE_MONITOR_NONE, NULL, NULL);
			g_signal_connect (model->priv->bookmarks_monitor, 
					  "changed", 
					  (GCallback)on_bookmarks_file_changed, 
					  model);
		}
	} else {
		/* The bookmarks file doesn't exist (which is perfectly fine) */
		g_error_free (error);
	}

	if (added) {
		/* Bookmarks separator */
		add_node (model, NULL, NULL, NULL,
			  GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK |
			  GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR, NULL);
	}

	g_free (bookmarks);
}

static gint flags_order[] = {
	GEDIT_FILE_BOOKMARKS_STORE_IS_HOME,
	GEDIT_FILE_BOOKMARKS_STORE_IS_DESKTOP,
	GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR,
	GEDIT_FILE_BOOKMARKS_STORE_IS_ROOT,
	GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT,
	GEDIT_FILE_BOOKMARKS_STORE_IS_LOCAL_MOUNT,
	GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK,
	-1
};

static gint
utf8_casecmp (gchar const *s1, const gchar * s2)
{
	gchar *n1;
	gchar *n2;
	gint result;

	n1 = g_utf8_casefold (s1, -1);
	n2 = g_utf8_casefold (s2, -1);

	result = g_utf8_collate (n1, n2);

	g_free (n1);
	g_free (n2);

	return result;
}

static gint
bookmarks_compare_names (GtkTreeModel * model, GtkTreeIter * a,
			 GtkTreeIter * b)
{
	gchar *n1;
	gchar *n2;
	gint result;

	gtk_tree_model_get (model, a,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_NAME, &n1,
			    -1);
	gtk_tree_model_get (model, b,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_NAME, &n2,
			    -1);

	if (n1 == NULL && n2 == NULL)
		result = 0;
	else if (n1 == NULL)
		result = -1;
	else if (n2 == NULL)
		result = 1;
	else
		result = utf8_casecmp (n1, n2);

	g_free (n1);
	g_free (n2);

	return result;
}

static gint
bookmarks_compare_flags (GtkTreeModel * model, GtkTreeIter * a,
			 GtkTreeIter * b)
{
	guint f1;
	guint f2;
	gint *flags;
	guint sep;

	sep = GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR;

	gtk_tree_model_get (model, a,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS, &f1,
			    -1);
	gtk_tree_model_get (model, b,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS, &f2,
			    -1);

	for (flags = flags_order; *flags != -1; ++flags) {
		if ((f1 & *flags) != (f2 & *flags)) {
			if (f1 & *flags) {
				return -1;
			} else {
				return 1;
			}
		} else if ((f1 & *flags) && (f1 & sep) != (f2 & sep)) {
			if (f1 & sep)
				return -1;
			else
				return 1;
		}
	}

	return 0;
}

static gint
bookmarks_compare_func (GtkTreeModel * model, GtkTreeIter * a,
			GtkTreeIter * b, gpointer user_data)
{
	gint result;

	result = bookmarks_compare_flags (model, a, b);

	if (result == 0)
		result = bookmarks_compare_names (model, a, b);

	return result;
}

static gboolean
find_with_flags (GtkTreeModel * model, GtkTreeIter * iter, gpointer obj,
		 guint flags, guint notflags)
{
	GtkTreeIter child;
	guint childflags = 0;
 	GObject * childobj;
 	gboolean fequal;
	
	if (!gtk_tree_model_get_iter_first (model, &child))
		return FALSE;

	do {
		gtk_tree_model_get (model, &child,
				    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT,
				    &childobj,
				    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
				    &childflags, -1);
		
		fequal = (obj == childobj);
		
		if (childobj)
			g_object_unref (childobj);
		 
		if ((obj == NULL || fequal) &&
		    (childflags & flags) == flags
		    && !(childflags & notflags)) {
			*iter = child;
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &child));

	return FALSE;
}

static void
remove_node (GtkTreeModel * model, GtkTreeIter * iter)
{
	guint flags;

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (!(flags & GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR)) {
		if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT) {
			check_mount_separator (GEDIT_FILE_BOOKMARKS_STORE (model),
			     		       flags & GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT,
					       FALSE);
		}
	}

	gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
}

static void
remove_bookmarks (GeditFileBookmarksStore * model)
{
	GtkTreeIter iter;

	while (find_with_flags (GTK_TREE_MODEL (model), &iter, NULL,
				GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK,
				0)) {
		remove_node (GTK_TREE_MODEL (model), &iter);
	}
}

static void
initialize_fill (GeditFileBookmarksStore * model)
{
	init_special_directories (model);
	init_mounts (model);
	init_bookmarks (model);
}

/* Public */
GeditFileBookmarksStore *
gedit_file_bookmarks_store_new ()
{
	GeditFileBookmarksStore *model;
	GType column_types[] = {
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_OBJECT,
		G_TYPE_UINT
	};

	model = g_object_new (GEDIT_TYPE_FILE_BOOKMARKS_STORE, NULL);
	gtk_tree_store_set_column_types (GTK_TREE_STORE (model),
					 GEDIT_FILE_BOOKMARKS_STORE_N_COLUMNS,
					 column_types);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
						 bookmarks_compare_func,
						 NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);

	initialize_fill (model);

	return model;
}

gchar *
gedit_file_bookmarks_store_get_uri (GeditFileBookmarksStore * model,
				    GtkTreeIter * iter)
{
	GObject * obj;
	GFile * file = NULL;
	guint flags;
	gchar * ret = NULL;
	
	g_return_val_if_fail (GEDIT_IS_FILE_BOOKMARKS_STORE (model), NULL);
	g_return_val_if_fail (iter != NULL, NULL);

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
			    &flags,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT, &obj,
			    -1);

	if (obj == NULL)
		return NULL;

	if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT) {
		file = g_mount_get_root (G_MOUNT (obj));
	} else {
		file = g_object_ref (obj);
	}
	
	g_object_unref (obj);
	
	if (file) {
		ret = g_file_get_uri (file);
		g_object_unref (file);
	}
	
	return ret;
}

void
gedit_file_bookmarks_store_refresh (GeditFileBookmarksStore * model)
{
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	initialize_fill (model);
}

/* Signal handlers */
static void
on_mount_added (GVolumeMonitor * monitor,
		GMount * mount,
		GeditFileBookmarksStore * model)
{
	process_mount (model, mount, NULL);
}

static void
on_mount_removed (GVolumeMonitor * monitor,
		  GMount * mount,
		  GeditFileBookmarksStore * model)
{
	GtkTreeIter iter;

	/* Find the mount and remove it */
	if (find_with_flags (GTK_TREE_MODEL (model), &iter, mount,
			     GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT,
			     GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR))
		remove_node (GTK_TREE_MODEL (model), &iter);
}

static void
on_bookmarks_file_changed (GFileMonitor * monitor,
			   GFile * file,
			   GFile * other_file,
			   GFileMonitorEvent event_type,
			   GeditFileBookmarksStore * model)
{
	switch (event_type) {
	case G_FILE_MONITOR_EVENT_CHANGED:
	case G_FILE_MONITOR_EVENT_CREATED:
		/* Re-initialize bookmarks */
		remove_bookmarks (model);
		init_bookmarks (model);
		break;
	case G_FILE_MONITOR_EVENT_DELETED: // FIXME: shouldn't we also monitor the directory?
		/* Remove bookmarks */
		remove_bookmarks (model);
		g_object_unref (monitor);
		model->priv->bookmarks_monitor = NULL;
		break;
	default:
		break;
	}
}

// ex:ts=8:noet:
