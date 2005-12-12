/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-languages-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2003-2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2003-2005. See the AUTHORS file for a 
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
	gchar**	items;
	GtkSourceTagStyle *style;
	
	style = gtk_source_tag_style_new ();
	items = g_strsplit (string, "/", 7);

	style->is_default = FALSE;
	
	if (items == NULL)
		goto error;

	if ((items [0] == NULL) || (strlen (items [0]) != 1))
		goto error;

	style->mask = items [0][0] - '0';
	
	if ((style->mask < 0) || (style->mask > 3))
		goto error;

	if ((items [1] == NULL) || (strlen (items [1]) != 13))
		goto error;

	if (!gdk_color_parse (items [1], &style->foreground))
		goto error;

	if ((items [2] == NULL) || (strlen (items [2]) != 13))
		goto error;

	if (!gdk_color_parse (items [2], &style->background))
		goto error;

	if ((items [3] == NULL) || (strlen (items [3]) != 1))
		goto error;

	style->italic = items [3][0] - '0';
	
	if ((style->italic < 0) || (style->italic > 1))
		goto error;
	
	if ((items [4] == NULL) || (strlen (items [4]) != 1))
		goto error;

	style->bold = items [4][0] - '0';
	
	if ((style->bold < 0) || (style->bold > 1))
		goto error;

	if ((items [5] == NULL) || (strlen (items [5]) != 1))
		goto error;

	style->underline = items [5][0] - '0';
	
	if ((style->underline < 0) || (style->underline > 1))
		goto error;

	if ((items [6] == NULL) || (strlen (items [6]) != 1))
		goto error;

	style->strikethrough = items [6][0] - '0';
	
	if ((style->strikethrough < 0) || (style->strikethrough > 1))
		goto error;

	return style;

error:
	gtk_source_tag_style_free (style);

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

