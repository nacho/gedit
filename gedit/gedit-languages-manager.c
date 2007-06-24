/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-languages-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2003-2006 - Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 2003-2006. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#include <string.h>

#include "gedit-languages-manager.h"
#include "gedit-prefs-manager.h"
#include "gedit-utils.h"
#include "gedit-debug.h"

static GtkSourceLanguageManager *language_manager = NULL;
static const GSList *languages_list = NULL;
static GSList *languages_list_sorted = NULL;


GtkSourceLanguageManager *
gedit_get_language_manager (void)
{
	if (language_manager == NULL)
	{
		language_manager = gtk_source_language_manager_new ();
	}

	return language_manager;
}

static gint
language_compare (gconstpointer a, gconstpointer b)
{
	GtkSourceLanguage *lang_a = a;
	GtkSourceLanguage *lang_b = b;
	const gchar *name_a = gtk_source_language_get_name (lang_a);
	const gchar *name_b = gtk_source_language_get_name (lang_b);
	gint res;
	
	res = g_utf8_collate (name_a, name_b);
		
	return res;
}

/* Move the sorting in sourceview as a flag to get_available_langs? */
const GSList *
gedit_language_manager_get_available_languages_sorted (GtkSourceLanguageManager *lm)
{
	const GSList *languages;

	// FIXME This is totally broken also in the old code: checking if
	// languages != languages_list is not a good way to see if no new lang
	// were loaded: elements could have been appended to the list without
	// changing the list head! Should be easy to fix if we make gsv return
	// an array and the array len... or even better if gsv is able to sort

	languages = gtk_source_language_manager_get_available_languages (lm);

	if ((languages_list_sorted == NULL) || (languages != languages_list))
	{
		languages_list = languages;
		languages_list_sorted = g_slist_copy ((GSList *)languages);
		languages_list_sorted = g_slist_sort (languages_list_sorted, (GCompareFunc)language_compare);
	}

	return languages_list_sorted;
}

/* Returns a hash table that is used as a cache of already matched mime-types */
static GHashTable *
get_languages_cache (GtkSourceLanguageManager *lm)
{
	static GQuark id = 0;
	GHashTable *res;

	if (id == 0)
		id = g_quark_from_static_string ("gedit_languages_manager_cache");

	res = (GHashTable *)g_object_get_qdata (G_OBJECT (lm), id);
	if (res == NULL)
	{
		res = g_hash_table_new_full (g_str_hash,
					     g_str_equal,
					     g_free,
					     NULL);

		g_object_set_qdata_full (G_OBJECT (lm), 
					 id,
					 res,
					 (GDestroyNotify)g_hash_table_unref);
	}
	
	return res;
}

static GtkSourceLanguage *
get_language_from_cache (GtkSourceLanguageManager *lm,
			 const gchar              *mime_type)
{
	GHashTable *cache;

	cache = get_languages_cache (lm);

	return g_hash_table_lookup (cache, mime_type);
}

static void
add_language_to_cache (GtkSourceLanguageManager *lm,
		       const gchar              *mime_type,
		       GtkSourceLanguage        *lang)
{
	GHashTable *cache;

	cache = get_languages_cache (lm);
	g_hash_table_replace (cache, g_strdup (mime_type), lang);
}

GtkSourceLanguage *
gedit_language_manager_get_language_from_mime_type (GtkSourceLanguageManager *lm,
						    const gchar              *mime_type)
{
	const GSList *languages;
	const GSList *l;
	GtkSourceLanguage *lang;
	GtkSourceLanguage *parent = NULL;

	g_return_val_if_fail (mime_type != NULL, NULL);

	lang = get_language_from_cache (lm, mime_type);
	if (lang != NULL)
	{
		gedit_debug_message (DEBUG_DOCUMENT,
				     "Cache hit for %s", mime_type);
		return lang;
	}
	gedit_debug_message (DEBUG_DOCUMENT,
			     "Cache miss for %s", mime_type);

	languages = gtk_source_language_manager_get_available_languages (lm);

	for (l = languages; l != NULL; l = l->next)
	{
		gchar **mime_types;
		gchar *found = NULL;
		gint i;

		lang = l->data;

		mime_types = gtk_source_language_get_mime_types (lang);
		if (mime_types == NULL)
			continue;

		for (i = 0; mime_types[i] != NULL; ++i)
		{
			GnomeVFSMimeEquivalence res;

			/* Use gnome_vfs_mime_type_get_equivalence instead of
			 * strcmp, to take care of mime-types inheritance
			 * (see bug #324191) */
			res = gnome_vfs_mime_type_get_equivalence (mime_type, 
								   mime_types[i]);

			if (res == GNOME_VFS_MIME_IDENTICAL)
			{
				/* If the mime-type of lang is identical to "mime-type" then
				   return lang */
				gedit_debug_message (DEBUG_DOCUMENT,
						     "%s is indentical to %s\n",
						     mime_types[i], mime_type);

				found = mime_types[i];

				break;
			}
			else if ((res == GNOME_VFS_MIME_PARENT) && (parent == NULL))
			{
				/* If the mime-type of lang is a parent of "mime-type" then
				   remember it. We will return it if we don't find
				   an exact match. The first "parent" wins */
				parent = lang;

				gedit_debug_message (DEBUG_DOCUMENT,
						     "%s is a parent of %s\n",
						     mime_types[i], mime_type);
			}
		}

		if (found != NULL)
		{
			add_language_to_cache (lm, mime_type, lang);

			g_strfreev (mime_types);

			return lang;
		}

		g_strfreev (mime_types);
	}

	if (parent != NULL)
		add_language_to_cache (lm, mime_type, parent);
	
	return parent;
}

