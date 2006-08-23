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

/* FIXME: Monitor gconf keys */

#include <string.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gtksourceview/gtksourcetag.h>

#include "gedit-languages-manager.h"
#include "gedit-prefs-manager.h"
#include "gedit-utils.h"
#include "gedit-debug.h"

static GtkSourceLanguagesManager *language_manager = NULL;
static GConfClient *gconf_client = NULL;
static const GSList *languages_list = NULL;
static GSList *languages_list_sorted = NULL;


GtkSourceLanguagesManager *
gedit_get_languages_manager (void)
{
	if (language_manager == NULL)
	{
		language_manager = gtk_source_languages_manager_new ();	

		gconf_client = gconf_client_get_default ();
		g_return_val_if_fail (gconf_client != NULL, language_manager);
	}

	return language_manager;
}

GtkSourceLanguage *
gedit_languages_manager_get_language_from_id (GtkSourceLanguagesManager *lm,
					      const gchar               *lang_id)
{
	const GSList *languages;
	
	g_return_val_if_fail (lang_id != NULL, NULL);

	languages = gtk_source_languages_manager_get_available_languages (lm);

	while (languages != NULL)
	{
		gchar *name;

		GtkSourceLanguage *lang = GTK_SOURCE_LANGUAGE (languages->data);
		
		name = gtk_source_language_get_id (lang);

		if (strcmp (name, lang_id) == 0)
		{	
			g_free (name);	
			return lang;
		}
		
		g_free (name);
		languages = g_slist_next (languages);
	}

	return NULL;
}

static gchar*
get_gconf_key (GtkSourceLanguage *language, const gchar *tag_id)
{
	gchar *key;
	gchar *lang_id;

	lang_id = gtk_source_language_get_id (language);
	
	key = g_strconcat (GPM_SYNTAX_HL_DIR, "/", lang_id, "/", tag_id, NULL);

	g_return_val_if_fail (gconf_valid_key (key, NULL), NULL);

	g_free (lang_id);

	return key;
}

static gchar * 
gdk_color_to_string (GdkColor color)
{
	return g_strdup_printf ("#%04x%04x%04x",
				color.red, 
				color.green,
				color.blue);
}

static gchar *
tag_style_to_string (const GtkSourceTagStyle *style)
{
	gchar *res;
	gchar *background;
	gchar *foreground;

	background = gdk_color_to_string (style->background);
	foreground = gdk_color_to_string (style->foreground);

	res = g_strdup_printf ("%d/%s/%s/%d/%d/%d/%d",
			style->mask,
			foreground,
			background,
			style->italic,
			style->bold,
			style->underline,
			style->strikethrough);

	g_free (foreground);
	g_free (background);

	return res;
}

static GtkSourceTagStyle *
string_to_tag_style (const gchar *string)
{
	const int items_len[] = {1, 13, 13, 1, 1, 1, 1};
	guint i;
	gchar**	items;
	GtkSourceTagStyle *style;
	
	style = gtk_source_tag_style_new ();
	items = g_strsplit (string, "/", G_N_ELEMENTS(items_len));

	for (i = 0; i < G_N_ELEMENTS (items_len); ++i)
	{
		if ((items[i] == NULL) || (strlen (items[i]) != items_len[i]))
			goto error;
	}

	style->is_default = FALSE;

	style->mask = items[0][0] - '0';
	if (style->mask < 0 || style->mask > 3)
		goto error;

	if (!gdk_color_parse (items[1], &style->foreground))
		goto error;

	if (!gdk_color_parse (items[2], &style->background))
		goto error;

	style->italic = items[3][0] - '0';
	if (!IS_VALID_BOOLEAN (style->italic))
		goto error;

	style->bold = items[4][0] - '0';
	if (!IS_VALID_BOOLEAN (style->bold))
		goto error;

	style->underline = items[5][0] - '0';
	if (!IS_VALID_BOOLEAN (style->underline))
		goto error;

	style->strikethrough = items[6][0] - '0';
	if (!IS_VALID_BOOLEAN (style->strikethrough))
		goto error;

	g_strfreev (items);
	return style;

error:
	gtk_source_tag_style_free (style);

	g_strfreev (items);
	
	return NULL;
}

void
gedit_language_set_tag_style (GtkSourceLanguage       *language,
			      const gchar             *tag_id,
			      const GtkSourceTagStyle *style)
{
	gchar *key;

	g_return_if_fail (gconf_client != NULL);

	key = get_gconf_key (language, tag_id);
	g_return_if_fail (key != NULL);
	
	if (style == NULL)
	{
		gconf_client_unset (gconf_client, key, NULL);
		
		/* Make the changes locally */
		gtk_source_language_set_tag_style (language, tag_id, NULL);
	}
	else
	{
		gchar *value;

		value = tag_style_to_string (style);
		g_return_if_fail (value != NULL);
		
		gconf_client_set_string (gconf_client, key, value, NULL);
	
		/* Make the changes locally */
		gtk_source_language_set_tag_style (language, tag_id, style);
	}
	
	g_free (key);
}

static GSList *initialized_languages = NULL;

void 
gedit_language_init_tag_styles (GtkSourceLanguage *language)
{
	GSList *l;
	GSList *tags;

	l = initialized_languages;

	while (l != NULL)
	{
		if (l->data == language)
			/* Already initialized */
			return;

		l = g_slist_next (l);
	}

	tags = gtk_source_language_get_tags (language);

	l = tags;

	while (l != NULL)
	{
		GtkSourceTag *tag;
		gchar *id;
		gchar *key;
		gchar *value;
		
		tag = GTK_SOURCE_TAG (l->data);
		
		id = gtk_source_tag_get_id (tag);
		g_return_if_fail (id != NULL);
		
		key = get_gconf_key (language, id);
		g_return_if_fail (key != NULL);
		
		value = gconf_client_get_string (gconf_client, key, NULL);
		
		if (value != NULL)
		{
			GtkSourceTagStyle *style;
	
			style = string_to_tag_style (value);
			if (style != NULL)
			{
				gtk_source_language_set_tag_style (language, id, style);
				
				gtk_source_tag_style_free (style);

			}
			else
			{
				g_warning ("gconf key %s contains an invalid value", key);
			}

			g_free (value);
		}

		g_free (id);
		g_free (key);

		l = g_slist_next (l);
	}

	g_slist_foreach (tags, (GFunc)g_object_unref, NULL);
	g_slist_free (tags);

	initialized_languages =	g_slist_prepend (initialized_languages, language);
}

static gint
language_compare (gconstpointer a, gconstpointer b)
{
	GtkSourceLanguage *lang_a = GTK_SOURCE_LANGUAGE (a);
	GtkSourceLanguage *lang_b = GTK_SOURCE_LANGUAGE (b);
	gchar *name_a = gtk_source_language_get_name (lang_a);
	gchar *name_b = gtk_source_language_get_name (lang_b);
	gint res;
	
	res = g_utf8_collate (name_a, name_b);
		
	g_free (name_a);
	g_free (name_b);	
	
	return res;
}

const GSList*
gedit_languages_manager_get_available_languages_sorted (GtkSourceLanguagesManager *lm)
{
	const GSList *languages;

	languages = gtk_source_languages_manager_get_available_languages (lm);

	if ((languages_list_sorted == NULL) || (languages != languages_list))
	{
		languages_list = languages;
		languages_list_sorted = g_slist_copy ((GSList*)languages);
		languages_list_sorted = g_slist_sort (languages_list_sorted, (GCompareFunc)language_compare);
	}

	return languages_list_sorted;
}

/* Returns a hash table that is used as a cache of already matched mime-types */
static GHashTable *
get_languages_cache (GtkSourceLanguagesManager *lm)
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
get_language_from_cache (GtkSourceLanguagesManager *lm,
			 const gchar               *mime_type)
{
	GHashTable *cache;
	
	cache = get_languages_cache (lm);
	
	return g_hash_table_lookup (cache, mime_type);
}

static void
add_language_to_cache (GtkSourceLanguagesManager *lm,
		       const gchar               *mime_type,
		       GtkSourceLanguage         *lang)
{
	GHashTable *cache;
	
	cache = get_languages_cache (lm);
	g_hash_table_replace (cache, g_strdup (mime_type), lang);
}

/*
 * gedit_languages_manager_get_language_from_mime_type works like
 * gtk_source_languages_manager_get_language_from_mime_type but it uses
 * gnome_vfs_mime_type_get_equivalence instead of strcmp to compare mime-types.
 * In this way it takes care of mime-types inheritance (fixes bug #324191)
 * It also uses a cache of already matched mime-type in order to improve
 * performance for frequently used mime-types.
 */
GtkSourceLanguage *
gedit_languages_manager_get_language_from_mime_type (GtkSourceLanguagesManager 	*lm,
						     const gchar                *mime_type)
{
	const GSList *languages;
	GtkSourceLanguage *lang;
	GtkSourceLanguage *parent = NULL;
	
	g_return_val_if_fail (mime_type != NULL, NULL);

	languages = gtk_source_languages_manager_get_available_languages (lm);
	
	lang = get_language_from_cache (lm, mime_type);
	if (lang != NULL)
	{
		gedit_debug_message (DEBUG_DOCUMENT,
				     "Cache hit for %s", mime_type);
		return lang;
	}
	gedit_debug_message (DEBUG_DOCUMENT,
			     "Cache miss for %s", mime_type);

	while (languages != NULL)
	{
		GSList *mime_types, *tmp;

		lang = GTK_SOURCE_LANGUAGE (languages->data);
		
		tmp = mime_types = gtk_source_language_get_mime_types (lang);

		while (tmp != NULL)
		{
			GnomeVFSMimeEquivalence res;
			res = gnome_vfs_mime_type_get_equivalence (mime_type, 
								   (const gchar*)tmp->data);
									
			if (res == GNOME_VFS_MIME_IDENTICAL)
			{	
				/* If the mime-type of lang is identical to "mime-type" then
				   return lang */
				gedit_debug_message (DEBUG_DOCUMENT,
						     "%s is indentical to %s\n", (const gchar*)tmp->data, mime_type);
				   
				break;
			}
			else if ((res == GNOME_VFS_MIME_PARENT) && (parent == NULL))
			{
				/* If the mime-type of lang is a parent of "mime-type" then
				   remember it. We will return it if we don't find
				   an exact match. The first "parent" wins */
				parent = lang;
				
				gedit_debug_message (DEBUG_DOCUMENT,
						     "%s is a parent of %s\n", (const gchar*)tmp->data, mime_type);
			}

			tmp = g_slist_next (tmp);
		}

		g_slist_foreach (mime_types, (GFunc) g_free, NULL);
		g_slist_free (mime_types);
		
		if (tmp != NULL)
		{
			add_language_to_cache (lm, mime_type, lang);
			return lang;
		}
		
		languages = g_slist_next (languages);
	}

	if (parent != NULL)
		add_language_to_cache (lm, mime_type, parent);
	
	return parent;
}

