/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-languages-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 - Paolo Maggi 
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
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include "gedit-languages-manager.h"

#include <string.h>

static GtkSourceLanguagesManager *language_manager = NULL;

GtkSourceLanguagesManager *
gedit_get_languages_manager (void)
{
	if (language_manager == NULL)
		language_manager = gtk_source_languages_manager_new ();	

	return language_manager;
}



GtkSourceLanguage *
gedit_languages_manager_get_language_from_name (GtkSourceLanguagesManager *lm,
						const gchar 		  *lang_name)
{
	const GSList *languages;
	g_return_val_if_fail (lang_name != NULL, NULL);

	languages = gtk_source_languages_manager_get_available_languages (lm);

	while (languages != NULL)
	{
		gchar *name;

		GtkSourceLanguage *lang = GTK_SOURCE_LANGUAGE (languages->data);
		
		name = gtk_source_language_get_name (lang);

		if (strcmp (name, lang_name) == 0)
		{	
			g_free (name);	
			return lang;
		}
		
		g_free (name);
		languages = g_slist_next (languages);
	}

	return NULL;
}

