/*
 * gedit-style-scheme-generator.c
 * This file is part of gedit
 *
 * Copyright (C) 2007 - Paolo Maggi
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "gedit-style-scheme-generator.h"

#include <glib/gi18n.h>
#include <gedit-utils.h>

G_DEFINE_TYPE (GeditStyleSchemeGenerator, gedit_style_scheme_generator, G_TYPE_OBJECT)

#define STYLE_SCHEME_GENERATOR_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEDIT_TYPE_STYLE_SCHEME_GENERATOR, GeditStyleSchemeGeneratorPrivate))

#define STYLE_TEXT			"text"
#define STYLE_SELECTED			"selection"
#define STYLE_CURRENT_LINE		"current-line"
#define STYLE_SEARCH_MATCH		"search-match"

struct _GeditStyleSchemeGeneratorPrivate
{
	GtkSourceStyleScheme *original_scheme;

	gchar                *id;
	gchar                *name;
	gchar                *description;

	GHashTable           *defined_styles;
};

static void
gedit_style_scheme_generator_finalize (GObject *object)
{
	GeditStyleSchemeGeneratorPrivate *priv = STYLE_SCHEME_GENERATOR_PRIVATE (object);

	if (priv->original_scheme != NULL)
		g_object_unref (priv->original_scheme);

	g_free (priv->id);
	g_free (priv->name);
	g_free (priv->description);

	g_hash_table_destroy (priv->defined_styles);

	G_OBJECT_CLASS (gedit_style_scheme_generator_parent_class)->finalize (object);
}

static void
gedit_style_scheme_generator_class_init (GeditStyleSchemeGeneratorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GeditStyleSchemeGeneratorPrivate));

	object_class->finalize = gedit_style_scheme_generator_finalize;
}

static void
init_defined_styles (GeditStyleSchemeGenerator *generator)
{
	gint i = 0;
	gchar *basic_styles[] = {STYLE_TEXT, STYLE_SELECTED, STYLE_CURRENT_LINE, STYLE_SEARCH_MATCH, NULL};

	g_return_if_fail (generator->priv->original_scheme != NULL);

	while (basic_styles[i] != NULL)
	{
		GtkSourceStyle *style;
		style = gtk_source_style_scheme_get_style (generator->priv->original_scheme, basic_styles[i]);

		if (style != NULL)
			g_hash_table_insert (generator->priv->defined_styles,
					     g_strdup (basic_styles[i]),
					     gtk_source_style_copy (style));

		++i;
	}
}

static void
set_original_scheme (GeditStyleSchemeGenerator *generator,
		     GtkSourceStyleScheme      *scheme)
{
	const gchar *cstr;

	g_return_if_fail (generator->priv->original_scheme == NULL);

	generator->priv->original_scheme = g_object_ref (scheme);

	cstr = gtk_source_style_scheme_get_name (scheme);
	if (cstr == NULL)
		cstr = gtk_source_style_scheme_get_id (scheme);

	g_return_if_fail (cstr != NULL);

	/* TRANSLATORS: %s is a color scheme name. The resulting string represents
	 * the name of a color scheme generated by the color scheme in %s.
	 * So if %s is "Dark", the resulting string should be something like
	 * "My Dark" */
	generator->priv->name = g_strdup_printf (_("My %s"), cstr);

	cstr = gtk_source_style_scheme_get_description (scheme);
	if (cstr != NULL)
		generator->priv->description = g_strdup (cstr);

	init_defined_styles (generator);
}


static void
gedit_style_scheme_generator_init (GeditStyleSchemeGenerator *generator)
{
	generator->priv = STYLE_SCHEME_GENERATOR_PRIVATE (generator);

	generator->priv->defined_styles = g_hash_table_new_full (g_str_hash, g_str_equal,
								 g_free, g_object_unref);
}

GeditStyleSchemeGenerator*
gedit_style_scheme_generator_new (GtkSourceStyleScheme *scheme)
{
	GeditStyleSchemeGenerator *generator;

	g_return_val_if_fail (GTK_IS_SOURCE_STYLE_SCHEME (scheme), NULL);
	
	generator = g_object_new (GEDIT_TYPE_STYLE_SCHEME_GENERATOR, NULL);
	g_return_val_if_fail (generator != NULL, NULL);

	set_original_scheme (generator, scheme);

	return generator;
}

const gchar *
gedit_style_scheme_generator_get_scheme_name (GeditStyleSchemeGenerator *generator)
{
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), NULL);
	g_return_val_if_fail (generator->priv->name != NULL, NULL);

	return generator->priv->name;
}

void
gedit_style_scheme_generator_set_scheme_name (GeditStyleSchemeGenerator *generator,
					      const gchar               *name)
{
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (name != NULL);

	g_free (generator->priv->name);

	generator->priv->name = g_strdup (name);
}

const gchar *
gedit_style_scheme_generator_get_scheme_description (GeditStyleSchemeGenerator *generator)
{
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), NULL);

	return generator->priv->description;
}

void
gedit_style_scheme_generator_set_scheme_description (GeditStyleSchemeGenerator *generator,
						     const gchar               *description)
{
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (description != NULL);

	g_free (generator->priv->description);

	generator->priv->description = g_strdup (description);
}


static gboolean
get_background_color (GtkSourceStyle *style, GdkColor *color)
{
	gboolean bg_set;
	gchar *bg_str;

	g_object_get (style,
		      "background", &bg_str,
		      "background-set", &bg_set,
		      NULL);

	if (!bg_set)
		return FALSE;

	if (color == NULL)
		return TRUE;

	if (gdk_color_parse (bg_str, color))
	{
		g_free (bg_str);

		return TRUE;
	}

	g_free (bg_str);

	return FALSE;
}

static gboolean
get_foreground_color (GtkSourceStyle *style, GdkColor *color)
{
	gboolean fg_set;
	gchar *fg_str;

	g_object_get (style,
		      "foreground", &fg_str,
		      "foreground-set", &fg_set,
		      NULL);

	if (!fg_set)
		return FALSE;

	if (color == NULL)
		return TRUE;
		
	if (gdk_color_parse (fg_str, color))
	{
		g_free (fg_str);

		return TRUE;
	}

	g_free (fg_str);

	return FALSE;
}

static void
set_background_color (GtkSourceStyle *style, const GdkColor *color)
{
	gchar *color_str;
	
	color_str = gedit_gdk_color_to_string (*color);
	
	g_object_set (style,
		      "background", color_str,
		      "background-set", TRUE,
		      NULL);
		      
	g_free (color_str);
}

static void
set_foreground_color (GtkSourceStyle *style, const GdkColor *color)
{
	gchar *color_str;
	
	color_str = gedit_gdk_color_to_string (*color);
	
	g_object_set (style,
		      "foreground", color_str,
		      "foreground-set", TRUE,
		      NULL);

	g_free (color_str);
}

static GtkSourceStyle *
ensure_style (GeditStyleSchemeGenerator *generator, const gchar *style_id)
{
	GtkSourceStyle *style;
	
	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_TEXT);
	if (style != NULL)
		return style;
		
	return GTK_SOURCE_STYLE (g_object_new (GTK_TYPE_SOURCE_STYLE, NULL));
}

gboolean
gedit_style_scheme_generator_get_text_color (GeditStyleSchemeGenerator *generator,
					     GdkColor                  *color)
{
	GtkSourceStyle *style;
	
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), FALSE);

	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_TEXT);
	if (style == NULL)
		return FALSE;
		
	return get_foreground_color (style, color);
}


void
gedit_style_scheme_generator_set_text_color (GeditStyleSchemeGenerator *generator,
					     const GdkColor            *color)
{
	GtkSourceStyle *style;
	
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (color != NULL);
	
	style = ensure_style (generator, STYLE_TEXT);

	set_foreground_color (style, color);
}

gboolean
gedit_style_scheme_generator_get_background_color (GeditStyleSchemeGenerator *generator,
						   GdkColor                  *color)
{
	GtkSourceStyle *style;
	
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), FALSE);

	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_TEXT);
	if (style == NULL)
		return FALSE;
		
	return get_background_color (style, color);
}

void
gedit_style_scheme_generator_set_background_color (GeditStyleSchemeGenerator *generator,
						   const GdkColor            *color)
{
	GtkSourceStyle *style;
	
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (color != NULL);
	
	style = ensure_style (generator, STYLE_TEXT);

	set_background_color (style, color);
}

gboolean
gedit_style_scheme_generator_get_selected_text_color	(GeditStyleSchemeGenerator *generator,
							 GdkColor                  *color)
{
	GtkSourceStyle *style;
	
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), FALSE);

	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_SELECTED);
	if (style == NULL)
		return FALSE;
		
	return get_foreground_color (style, color);
}

void
gedit_style_scheme_generator_set_selected_text_color (GeditStyleSchemeGenerator *generator,
						      const GdkColor            *color)
{
	GtkSourceStyle *style;
	
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (color != NULL);
	
	style = ensure_style (generator, STYLE_SELECTED);

	set_foreground_color (style, color);
}

gboolean
gedit_style_scheme_generator_get_selection_color (GeditStyleSchemeGenerator *generator,
						  GdkColor                  *color)
{
	GtkSourceStyle *style;
	
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), FALSE);

	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_SELECTED);
	if (style == NULL)
		return FALSE;
		
	return get_background_color (style, color);
}
						 
void
gedit_style_scheme_generator_set_selection_color (GeditStyleSchemeGenerator *generator,
						  const GdkColor            *color)
{
	GtkSourceStyle *style;
	
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (color != NULL);
	
	style = ensure_style (generator, STYLE_SELECTED);

	set_background_color (style, color);
}

gboolean
gedit_style_scheme_generator_get_current_line_color (GeditStyleSchemeGenerator *generator,
						     GdkColor                  *color)
{
	GtkSourceStyle *style;
	
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), FALSE);

	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_CURRENT_LINE);
	if (style == NULL)
		return FALSE;
		
	return get_background_color (style, color);	
}
void
gedit_style_scheme_generator_set_current_line_color (GeditStyleSchemeGenerator *generator,
						     const GdkColor            *color)
{
	GtkSourceStyle *style;
	
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (color != NULL);
	
	style = ensure_style (generator, STYLE_CURRENT_LINE);

	set_background_color (style, color);
}

gboolean
gedit_style_scheme_generator_get_search_hl_text_color (GeditStyleSchemeGenerator *generator,
						       GdkColor                  *color)
{
	GtkSourceStyle *style;
	
	g_return_val_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator), FALSE);

	style = g_hash_table_lookup (generator->priv->defined_styles, STYLE_SEARCH_MATCH);
	if (style == NULL)
		return FALSE;
		
	return get_background_color (style, color);	
}

void
gedit_style_scheme_generator_set_search_hl_text_color (GeditStyleSchemeGenerator *generator,
						       const GdkColor            *color)
{
	GtkSourceStyle *style;
	
	g_return_if_fail (GEDIT_IS_STYLE_SCHEME_GENERATOR (generator));
	g_return_if_fail (color != NULL);
	
	style = ensure_style (generator, STYLE_SEARCH_MATCH);

	set_background_color (style, color);
}
