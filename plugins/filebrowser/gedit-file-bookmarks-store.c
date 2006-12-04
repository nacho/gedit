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

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomeui/libgnomeui.h>
#include <gedit/gedit-plugin.h>
#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-utils.h"

#define GEDIT_FILE_BOOKMARKS_STORE_GET_PRIVATE(object)( \
		G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BOOKMARKS_STORE, \
		GeditFileBookmarksStorePrivate))

struct _GeditFileBookmarksStorePrivate 
{
	GnomeVFSVolumeMonitor *volume_monitor;
	GnomeVFSMonitorHandle *bookmarks_monitor;
};

static void remove_node               (GtkTreeModel * model, 
                                       GtkTreeIter * iter,
                                       gboolean fromtree);

static void on_volume_mounted         (GnomeVFSVolumeMonitor * monitor,
                                       GnomeVFSVolume * volume,
                                       GeditFileBookmarksStore * model);
static void on_volume_unmounted       (GnomeVFSVolumeMonitor * monitor,
                                       GnomeVFSVolume * volume,
                                       GeditFileBookmarksStore * model);
static void on_bookmarks_file_changed (GnomeVFSMonitorHandle * handle,
				       gchar const *monitor_uri,
				       gchar const *info_uri,
				       GnomeVFSMonitorEventType event_type,
				       GeditFileBookmarksStore * model);
static gboolean find_with_flags       (GtkTreeModel * model, 
                                       GtkTreeIter * iter,
                                       gpointer obj, 
                                       guint flags,
                                       guint notflags);

GEDIT_PLUGIN_DEFINE_TYPE(GeditFileBookmarksStore, gedit_file_bookmarks_store,
	       GTK_TYPE_TREE_STORE)

static gboolean
foreach_remove_node (GtkTreeModel * model, GtkTreePath * path,
		     GtkTreeIter * iter, gpointer user_data)
{
	remove_node (model, iter, FALSE);
	return FALSE;
}

static void
gedit_file_bookmarks_store_finalize (GObject * object)
{
	GeditFileBookmarksStore *obj = GEDIT_FILE_BOOKMARKS_STORE (object);

	if (obj->priv->volume_monitor) {
		g_signal_handlers_disconnect_by_func (obj->priv->
						      volume_monitor,
						      on_volume_mounted,
						      obj);
		g_signal_handlers_disconnect_by_func (obj->priv->
						      volume_monitor,
						      on_volume_mounted,
						      obj);
	}

	if (obj->priv->bookmarks_monitor != NULL)
		gnome_vfs_monitor_cancel (obj->priv->bookmarks_monitor);

	gtk_tree_model_foreach (GTK_TREE_MODEL (obj), foreach_remove_node,
				NULL);

	G_OBJECT_CLASS (gedit_file_bookmarks_store_parent_class)->
	    finalize (object);
}

static void
gedit_file_bookmarks_store_class_init (GeditFileBookmarksStoreClass *
				       klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_file_bookmarks_store_finalize;

	g_type_class_add_private (object_class,
				  sizeof (GeditFileBookmarksStorePrivate));
}

static void
gedit_file_bookmarks_store_init (GeditFileBookmarksStore * obj)
{
	obj->priv = GEDIT_FILE_BOOKMARKS_STORE_GET_PRIVATE (obj);
}

/* Private */
static GdkPixbuf *
pixbuf_from_stock (const gchar * stock)
{
	return gedit_file_browser_utils_pixbuf_from_theme(stock, 
	                                                  GTK_ICON_SIZE_MENU);
}

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
add_uri (GeditFileBookmarksStore * model, GnomeVFSURI * uri,
	 gchar * name, guint flags, GtkTreeIter * iter)
{
	GdkPixbuf *pixbuf = NULL;
	gchar *mime;
	gchar *path;
	gboolean free_name = FALSE;

	/* CHECK: how bad is this? */
	if (!gnome_vfs_uri_exists (uri)) {
		gnome_vfs_uri_unref (uri);
		return FALSE;
	}

	if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_HOME)
		pixbuf = pixbuf_from_stock ("gnome-fs-home");
	else if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_DESKTOP)
		pixbuf = pixbuf_from_stock ("gnome-fs-desktop");

	if (pixbuf == NULL) {
		path = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
		mime = gnome_vfs_get_mime_type (path);

		pixbuf = gedit_file_browser_utils_pixbuf_from_mime_type (path, 
		                                                         mime, 
		                                                         GTK_ICON_SIZE_MENU);
		g_free (path);
		g_free (mime);
	}

	if (name == NULL) {
		name = gedit_file_browser_utils_uri_basename (gnome_vfs_uri_get_path (uri));
		free_name = TRUE;
	}

	add_node (model, pixbuf, name, uri, flags, iter);

	if (free_name)
		g_free (name);
	
	return TRUE;
}

static void
init_special_directories (GeditFileBookmarksStore * model)
{
	gchar const *path;
	gchar *local;
	GnomeVFSURI *uri;

	path = g_get_home_dir ();
	local = gnome_vfs_get_uri_from_local_path (path);
	uri = gnome_vfs_uri_new (local);
	g_free (local);

	add_uri (model, uri, NULL, GEDIT_FILE_BOOKMARKS_STORE_IS_HOME |
		 GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);

	local = g_build_filename (path, "Desktop", NULL);
	uri = gnome_vfs_uri_new (local);
	add_uri (model, uri, NULL, GEDIT_FILE_BOOKMARKS_STORE_IS_DESKTOP |
		 GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);
	g_free (local);

	local = g_build_filename (path, "Documents", NULL);
	uri = gnome_vfs_uri_new (local);

	if (gnome_vfs_uri_is_local (uri)) {
		if (gnome_vfs_uri_exists (uri))
			add_uri (model, uri, NULL,
				 GEDIT_FILE_BOOKMARKS_STORE_IS_DOCUMENTS |
				 GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);
		else
			gnome_vfs_uri_unref (uri);
	} else {
		add_uri (model, uri, NULL,
			 GEDIT_FILE_BOOKMARKS_STORE_IS_DOCUMENTS |
			 GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR, NULL);
	}
		

	g_free (local);
}

static void
check_volume_separator (GeditFileBookmarksStore * model, guint flags,
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
		remove_node (GTK_TREE_MODEL (model), &iter, TRUE);
	}
}

static void
add_volume (GeditFileBookmarksStore * model, GnomeVFSVolume * volume,
	    const gchar * name, guint flags, GtkTreeIter * iter)
{
	GdkPixbuf *pixbuf;
	gchar *icon;

	icon = gnome_vfs_volume_get_icon (volume);
	pixbuf = pixbuf_from_stock (icon);

	add_node (model, pixbuf, name, volume,
		  flags | GEDIT_FILE_BOOKMARKS_STORE_IS_VOLUME, iter);

	g_free (icon);

	flags = flags & (GEDIT_FILE_BOOKMARKS_STORE_IS_DRIVE |
			 GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT |
			 GEDIT_FILE_BOOKMARKS_STORE_IS_REMOTE_MOUNT);

	if (flags)
		check_volume_separator (model, flags, TRUE);
}

static gboolean
process_volume (GeditFileBookmarksStore * model, GnomeVFSVolume * volume,
		gboolean * root)
{
	GnomeVFSVolumeType vtype;
	guint flags;

	vtype = gnome_vfs_volume_get_volume_type (volume);

	if (gnome_vfs_volume_get_device_type (volume) ==
	    GNOME_VFS_DEVICE_TYPE_AUDIO_CD)
		return FALSE;

	if (gnome_vfs_volume_is_user_visible (volume)) {
		gchar *name;

		if (vtype == GNOME_VFS_VOLUME_TYPE_VFS_MOUNT)
			flags = GEDIT_FILE_BOOKMARKS_STORE_IS_DRIVE;
		else if (vtype == GNOME_VFS_VOLUME_TYPE_MOUNTPOINT)
			flags = GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT;
		else
			flags = GEDIT_FILE_BOOKMARKS_STORE_IS_REMOTE_MOUNT;

		name = gnome_vfs_volume_get_display_name (volume);

		add_volume (model, volume, name, flags, NULL);

		g_free (name);
	} else if (root && !*root) {
		gchar *uri;

		uri = gnome_vfs_volume_get_activation_uri (volume);

		if (strcmp (uri, "file:///") == 0) {
			*root = TRUE;

			add_volume (model, volume, "File System",
				    GEDIT_FILE_BOOKMARKS_STORE_IS_ROOT |
				    GEDIT_FILE_BOOKMARKS_STORE_IS_DRIVE,
				    NULL);
		}

		g_free (uri);
	} else {
		return FALSE;
	}

	return TRUE;
}

static void
init_volumes (GeditFileBookmarksStore * model)
{
	GList *volumes;
	GList *item;
	GnomeVFSVolume *volume;
	gboolean root = FALSE;

	if (model->priv->volume_monitor == NULL) {
		model->priv->volume_monitor =
		    gnome_vfs_get_volume_monitor ();

		/* Connect signals */
		g_signal_connect (model->priv->volume_monitor,
				  "volume-mounted",
				  G_CALLBACK (on_volume_mounted), model);
		g_signal_connect (model->priv->volume_monitor,
				  "volume-unmounted",
				  G_CALLBACK (on_volume_unmounted), model);
	}

	volumes =
	    gnome_vfs_volume_monitor_get_mounted_volumes (model->priv->
							  volume_monitor);

	for (item = volumes; item; item = item->next) {
		volume = GNOME_VFS_VOLUME (item->data);
		process_volume (model, volume, &root);
	}

	g_list_free (volumes);
}

static void
init_bookmarks (GeditFileBookmarksStore * model)
{
	gchar *bookmarks;
	GError *error = NULL;
	gchar *contents;
	gchar **lines;
	gchar **line;
	gchar *pos;
	gchar *unescape;
	GnomeVFSURI *uri;
	gboolean added = FALSE;

	/* Read the bookmarks file */
	bookmarks =
	    g_build_filename (g_get_home_dir (), ".gtk-bookmarks", NULL);

	if (g_file_get_contents (bookmarks, &contents, NULL, &error)) {
		lines = g_strsplit (contents, "\n", 0);

		for (line = lines; *line; ++line) {
			if (**line) {
				/* Check, is this really utf8? */
				pos = g_utf8_strchr (*line, -1, ' ');

				if (pos == NULL) {
					unescape =
					    gnome_vfs_unescape_string
					    (*line, "");
					uri = gnome_vfs_uri_new (unescape);
					g_free (unescape);

					added = add_uri (model, uri, NULL,
						 GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK,
						 NULL);
				} else {
					*pos = '\0';
					unescape =
					    gnome_vfs_unescape_string
					    (*line, "");
					uri = gnome_vfs_uri_new (unescape);
					g_free (unescape);

					added = add_uri (model, uri, pos + 1,
						 GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK,
						 NULL);
				}
			}
		}

		g_strfreev (lines);
		g_free (contents);

		/* Add a watch */
		if (model->priv->bookmarks_monitor == NULL) {
			gnome_vfs_monitor_add (&model->priv->bookmarks_monitor,
					       bookmarks,
					       GNOME_VFS_MONITOR_FILE,
					       (GnomeVFSMonitorCallback)on_bookmarks_file_changed, 
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
	GEDIT_FILE_BOOKMARKS_STORE_IS_DRIVE,
	GEDIT_FILE_BOOKMARKS_STORE_IS_ROOT,
	GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT,
	GEDIT_FILE_BOOKMARKS_STORE_IS_REMOTE_MOUNT,
	GEDIT_FILE_BOOKMARKS_STORE_IS_VOLUME,
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
	gpointer childobj;

	if (!gtk_tree_model_get_iter_first (model, &child))
		return FALSE;

	do {
		gtk_tree_model_get (model, &child,
				    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT,
				    &childobj,
				    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
				    &childflags, -1);

		if ((obj == NULL || childobj == obj) &&
		    (childflags & flags) == flags
		    && !(childflags & notflags)) {
			*iter = child;
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &child));

	return FALSE;
}

static void
remove_node (GtkTreeModel * model, GtkTreeIter * iter, gboolean fromtree)
{
	gpointer obj;
	guint flags;

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
			    &flags,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT, &obj,
			    -1);

	if (!(flags & GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR)) {
		if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_VOLUME) {
			gnome_vfs_volume_unref (GNOME_VFS_VOLUME (obj));

			if (fromtree)
				check_volume_separator
				    (GEDIT_FILE_BOOKMARKS_STORE (model),
				     flags &
				     (GEDIT_FILE_BOOKMARKS_STORE_IS_DRIVE |
				      GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT),
				     FALSE);
		} else if ((flags &
			   GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR) ||
			   (flags &
			   GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK)) {
			gnome_vfs_uri_unref ((GnomeVFSURI *) obj);
		}
	}

	if (fromtree)
		gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
}

static void
remove_bookmarks (GeditFileBookmarksStore * model)
{
	GtkTreeIter iter;

	while (find_with_flags (GTK_TREE_MODEL (model), &iter, NULL,
				GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK,
				0)) {
		remove_node (GTK_TREE_MODEL (model), &iter, TRUE);
	}
}

static void
initialize_fill (GeditFileBookmarksStore * model)
{
	init_special_directories (model);
	init_volumes (model);
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
		G_TYPE_POINTER,
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
	gpointer obj;
	guint flags;

	g_return_val_if_fail (GEDIT_IS_FILE_BOOKMARKS_STORE (model), NULL);
	g_return_val_if_fail (iter != NULL, NULL);

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS,
			    &flags,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT, &obj,
			    -1);

	if (obj == NULL)
		return NULL;

	if (!(flags & GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR)) {
		if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_VOLUME) {
			return
			    gnome_vfs_volume_get_activation_uri
			    (GNOME_VFS_VOLUME (obj));
		} else
		    if ((flags & GEDIT_FILE_BOOKMARKS_STORE_IS_SPECIAL_DIR)
			|| (flags &
			    GEDIT_FILE_BOOKMARKS_STORE_IS_BOOKMARK)) {
			return gnome_vfs_uri_to_string ((GnomeVFSURI *)
							obj,
							GNOME_VFS_URI_HIDE_NONE);
		}
	}

	return NULL;
}

void
gedit_file_bookmarks_store_refresh (GeditFileBookmarksStore * model)
{
	gtk_tree_model_foreach (GTK_TREE_MODEL (model), foreach_remove_node,
				NULL);

	gtk_tree_store_clear (GTK_TREE_STORE (model));
	initialize_fill (model);
}

/* Signal handlers */
static void
on_volume_mounted (GnomeVFSVolumeMonitor * monitor,
		   GnomeVFSVolume * volume,
		   GeditFileBookmarksStore * model)
{
	if (process_volume (model, volume, NULL))
		gnome_vfs_volume_ref (volume);
}

static void
on_volume_unmounted (GnomeVFSVolumeMonitor * monitor,
		     GnomeVFSVolume * volume,
		     GeditFileBookmarksStore * model)
{
	GtkTreeIter iter;

	/* Find the volume and remove it */
	if (find_with_flags (GTK_TREE_MODEL (model), &iter, volume,
			     GEDIT_FILE_BOOKMARKS_STORE_IS_VOLUME,
			     GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR))
		remove_node (GTK_TREE_MODEL (model), &iter, TRUE);
}

static void
on_bookmarks_file_changed (GnomeVFSMonitorHandle * handle,
			   gchar const *monitor_uri, gchar const *info_uri,
			   GnomeVFSMonitorEventType event_type,
			   GeditFileBookmarksStore * model)
{
	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
	case GNOME_VFS_MONITOR_EVENT_CREATED:
		remove_bookmarks (model);
		init_bookmarks (model);
		break;
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		remove_bookmarks (model);
		gnome_vfs_monitor_cancel (handle);
		model->priv->bookmarks_monitor = NULL;
		break;
	default:
		break;
	}
}

// ex:ts=8:noet:
