/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   James Willcox <jwillcox@cs.indiana.edu>
 */


#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include "egg-recent-item.h"



EggRecentItem *
egg_recent_item_new (void)
{
	EggRecentItem *item;

	item = g_new (EggRecentItem, 1);

	item->groups = NULL;
	item->private = FALSE;
	item->uri = NULL;
	item->mime_type = NULL;

	item->refcount = 1;

	return item;
}

static void
egg_recent_item_free (EggRecentItem *item)
{
	if (item->uri)
		g_free (item->uri);

	if (item->mime_type)
		g_free (item->mime_type);

	if (item->groups) {
		g_list_foreach (item->groups, (GFunc)g_free, NULL);
		g_list_free (item->groups);
		item->groups = NULL;
	}

	g_free (item);
}

void
egg_recent_item_ref (EggRecentItem *item)
{
	item->refcount++;
}

void
egg_recent_item_unref (EggRecentItem *item)
{
	item->refcount--;

	if (item->refcount == 0) {
		egg_recent_item_free (item);
	}
}


EggRecentItem * 
egg_recent_item_new_from_uri (const gchar *uri)
{
	EggRecentItem *item;

	g_return_val_if_fail (uri != NULL, NULL);

	item = egg_recent_item_new ();

	egg_recent_item_set_uri (item ,uri);
	
	item->mime_type = gnome_vfs_get_mime_type (item->uri);

	if (!item->mime_type)
		item->mime_type = g_strdup (GNOME_VFS_MIME_TYPE_UNKNOWN);

	return item;
}

/*
static GList *
egg_recent_item_copy_groups (const GList *list)
{
	GList *newlist = NULL;

	while (list) {
		gchar *group = (gchar *)list->data;

		newlist = g_list_prepend (newlist, g_strdup (group));

		list = list->next;
	}

	return newlist;
}


EggRecentItem *
egg_recent_item_copy (const EggRecentItem *item)
{
	EggRecentItem *newitem;

	newitem = egg_recent_item_new ();
	newitem->uri = g_strdup (item->uri);
	if (item->mime_type)
		newitem->mime_type = g_strdup (item->mime_type);
	newitem->timestamp = item->timestamp;
	newitem->private = item->private;
	newitem->groups = egg_recent_item_copy_groups (item->groups);

	return newitem;
}
*/

/*
EggRecentItem *
egg_recent_item_new_valist (const gchar *uri, va_list args)
{
	EggRecentItem *item;
	EggRecentArg arg;
	gchar *str1;
	gchar *str2;
	gboolean priv;

	item = egg_recent_item_new ();

	arg = va_arg (args, EggRecentArg);

	while (arg != EGG_RECENT_ARG_NONE) {
		switch (arg) {
			case EGG_RECENT_ARG_MIME_TYPE:
				str1 = va_arg (args, gchar*);

				egg_recent_item_set_mime_type (item, str1);
			break;
			case EGG_RECENT_ARG_GROUP:
				str1 = va_arg (args, gchar*);

				egg_recent_item_add_group (item, str1);
			break;
			case EGG_RECENT_ARG_PRIVATE:
				priv = va_arg (args, gboolean);

				egg_recent_item_set_private (item, priv);
			break;
			default:
			break;
		}

		arg = va_arg (args, EggRecentArg);
	}

	return item;
}
*/

void 
egg_recent_item_set_uri (EggRecentItem *item, const gchar *uri)
{
	gchar *ascii_uri;

	ascii_uri = g_filename_from_utf8 (uri, -1, NULL, NULL, NULL);

	g_return_if_fail (ascii_uri != NULL);
	
	item->uri = ascii_uri;
}

gchar * 
egg_recent_item_get_uri (const EggRecentItem *item)
{
	return g_strdup (item->uri);
}

gchar *
egg_recent_item_get_uri_utf8 (const EggRecentItem *item)
{
	return g_filename_to_utf8 (item->uri, -1, NULL, NULL, NULL);
}

void 
egg_recent_item_set_mime_type (EggRecentItem *item, const gchar *mime)
{
	item->mime_type = g_strdup (mime);
}

gchar * 
egg_recent_item_get_mime_type (const EggRecentItem *item)
{
	return g_strdup (item->mime_type);
}

void 
egg_recent_item_set_timestamp (EggRecentItem *item, time_t timestamp)
{
	item->timestamp = timestamp;
}

time_t 
egg_recent_item_get_timestamp (const EggRecentItem *item)
{
	return item->timestamp;
}

const GList *
egg_recent_item_get_groups (const EggRecentItem *item)
{
	return item->groups;
}

gboolean
egg_recent_item_in_group (const EggRecentItem *item, const gchar *group_name)
{
	GList *tmp;

	tmp = item->groups;
	while (tmp != NULL) {
		gchar *val = (gchar *)tmp->data;
		
		if (strcmp (group_name, val) == 0)
			return TRUE;

		tmp = tmp->next;
	}
	
	return FALSE;
}

void
egg_recent_item_add_group (EggRecentItem *item, const gchar *group_name)
{
	g_return_if_fail (group_name != NULL);

	if (!egg_recent_item_in_group (item, group_name))
		item->groups = g_list_append (item->groups, g_strdup (group_name));
}

void
egg_recent_item_remove_group (EggRecentItem *item, const gchar *group_name)
{
	GList *tmp;

	g_return_if_fail (group_name != NULL);

	tmp = item->groups;
	while (tmp != NULL) {
		gchar *val = (gchar *)tmp->data;
		
		if (strcmp (group_name, val) == 0) {
			item->groups = g_list_remove (item->groups,
						      val);
			g_free (val);
			break;
		}

		tmp = tmp->next;
	}
}

void
egg_recent_item_set_private (EggRecentItem *item, gboolean priv)
{
	item->private = priv;
}

gboolean
egg_recent_item_get_private (const EggRecentItem *item)
{
	return item->private;
}

GType
egg_recent_item_get_type (void)
{
	static GType boxed_type = 0;
	
	if (!boxed_type) {
		boxed_type = g_boxed_type_register_static ("EggRecentItem",
					(GBoxedCopyFunc)egg_recent_item_ref,
					(GBoxedFreeFunc)egg_recent_item_unref);
	}
	
	return boxed_type;
}
