/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-source-style-manager.c
 *
 * Copyright (C) 2007 - Paolo Borelli
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
 *
 * $Id: gedit-source-style-manager.c 5598 2007-04-15 13:16:24Z pborelli $
 */

#include <string.h>

#include "gedit-source-style-manager.h"
#include "gedit-prefs-manager.h"

static GtkSourceStyleManager *style_manager = NULL;
static GtkSourceStyleScheme  *def_style = NULL;

#define GEDIT_STYLES_DIR ".gnome2/gedit/styles"

static void
add_gedit_styles_path (GtkSourceStyleManager *mgr)
{
	const gchar *home;

	home = g_get_home_dir ();
	if (home != NULL)
	{
		gchar *dir;
		
		dir = g_build_filename (home, GEDIT_STYLES_DIR, NULL);

		gtk_source_style_manager_append_search_path (mgr, dir);
		g_free (dir);
	}
}

GtkSourceStyleManager *
gedit_get_source_style_manager (void)
{
	if (style_manager == NULL)
	{
		style_manager = gtk_source_style_manager_new ();
		add_gedit_styles_path (style_manager);
	}

	return style_manager;
}

GtkSourceStyleScheme *
gedit_source_style_manager_get_default_scheme (GtkSourceStyleManager *manager)
{
	g_return_val_if_fail (GTK_IS_SOURCE_STYLE_MANAGER (manager), def_style);

	if (def_style == NULL)
	{
		gchar *scheme_id;

		scheme_id = gedit_prefs_manager_get_source_style_scheme ();
		def_style = gtk_source_style_manager_get_scheme (manager,
							         scheme_id);
		if (def_style == NULL)
		{
			g_warning ("Style Scheme %s not found", scheme_id);
		}

		g_free (scheme_id);
	}

	return def_style;
}

/* Return TRUE if style scheme changed */
gboolean
_gedit_source_style_manager_set_default_scheme (GtkSourceStyleManager *manager,
						const gchar           *scheme_id)
{
	GtkSourceStyleScheme *new_style;

	g_return_val_if_fail (GTK_IS_SOURCE_STYLE_MANAGER (manager), FALSE);
	g_return_val_if_fail (scheme_id != NULL, FALSE);

	if (def_style != NULL &&
	    strcmp (scheme_id, gtk_source_style_scheme_get_id (def_style)) == 0)
	{
		return FALSE;
	}

	new_style = gtk_source_style_manager_get_scheme (manager,
						         scheme_id);

	if (new_style == NULL)
	{
		g_warning ("Style Scheme %s not found", scheme_id);
		return FALSE;
	}

	/* manager owns the ref to the old style, we don't need to unref it */

	def_style = new_style;

	return TRUE;
}

gboolean
_gedit_source_style_manager_scheme_is_gedit_user_scheme (GtkSourceStyleManager *manager,
							 const gchar           *scheme_id)
{
	GtkSourceStyleScheme *scheme;
	const gchar *filename;
	const gchar *home;
	gchar *dir;
	gboolean res = FALSE;

	scheme = gtk_source_style_manager_get_scheme (manager, scheme_id);
	if (scheme == NULL)
		return FALSE;

	filename = gtk_source_style_scheme_get_filename (scheme);
	if (filename == NULL)
		return FALSE;

	home = g_get_home_dir ();
	if (home == NULL)
		return FALSE;

	dir = g_strdup_printf ("%s/%s",
			       home,
			       GEDIT_STYLES_DIR);

	res = g_str_has_prefix (filename, dir);

	g_free (dir);

	return res;
}

