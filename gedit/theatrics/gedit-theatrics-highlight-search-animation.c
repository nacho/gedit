/*
 * gedit-theatrics-highlight-search-animation.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Mike Krüger <mkrueger@novell.com> work.
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

#include "gedit-theatrics-utils.h"
#include "gedit-theatrics-highlight-search-animation.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <math.h>


#define GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION, GeditTheatricsHighlightSearchAnimationPrivate))

struct _GeditTheatricsHighlightSearchAnimationPrivate
{
	GtkSourceView *view;
	GtkTextIter start;
	GtkTextIter end;
	gdouble percent;

	cairo_surface_t *surface;
};

enum
{
	PROP_0,
	PROP_VIEW
};

static void gedit_theatrics_animation_drawable_iface_init (GeditTheatricsAnimationDrawableInterface *iface);

G_DEFINE_TYPE_EXTENDED (GeditTheatricsHighlightSearchAnimation,
			gedit_theatrics_highlight_search_animation,
			G_TYPE_OBJECT,
			0,
			G_IMPLEMENT_INTERFACE (GEDIT_TYPE_THEATRICS_ANIMATION_DRAWABLE,
					       gedit_theatrics_animation_drawable_iface_init))

static void
gedit_theatrics_highlight_search_animation_dispose (GObject *object)
{
	GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (object);

	if (rpa->priv->surface != NULL)
	{
		cairo_surface_finish (rpa->priv->surface);
		cairo_surface_destroy (rpa->priv->surface);

		rpa->priv->surface = NULL;
	}

	G_OBJECT_CLASS (gedit_theatrics_highlight_search_animation_parent_class)->dispose (object);
}

static void
gedit_theatrics_highlight_search_animation_get_property (GObject    *object,
							 guint       prop_id,
							 GValue     *value,
							 GParamSpec *pspec)
{
	GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (object);

	switch (prop_id)
	{
		case PROP_VIEW:
			g_value_set_object (value, rpa->priv->view);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_highlight_search_animation_set_property (GObject      *object,
							 guint         prop_id,
							 const GValue *value,
							 GParamSpec   *pspec)
{
	GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (object);

	switch (prop_id)
	{
		case PROP_VIEW:
		{
			GtkSourceView *view;

			view = g_value_get_object (value);

			if (view != NULL)
			{
				rpa->priv->view = view;
			}

			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_highlight_search_animation_class_init (GeditTheatricsHighlightSearchAnimationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = gedit_theatrics_highlight_search_animation_dispose;
	object_class->get_property = gedit_theatrics_highlight_search_animation_get_property;
	object_class->set_property = gedit_theatrics_highlight_search_animation_set_property;

	g_object_class_install_property (object_class, PROP_VIEW,
	                                 g_param_spec_object ("view",
	                                                      "View",
	                                                      "The Source View",
	                                                      GTK_TYPE_SOURCE_VIEW,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditTheatricsHighlightSearchAnimationPrivate));
}

static void
gedit_theatrics_highlight_search_animation_init (GeditTheatricsHighlightSearchAnimation *rpa)
{
	rpa->priv = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_GET_PRIVATE (rpa);
}

static GdkRectangle
gedit_theatrics_highlight_search_animation_get_animation_bounds (GeditTheatricsAnimationDrawable *ad)
{
	/*GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (ad); */
	GdkRectangle bounds = {0, }; /* FIXME */

	return bounds;
}

static gdouble
gedit_theatrics_highlight_search_animation_get_percent (GeditTheatricsAnimationDrawable *ad)
{
	GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (ad);

	return rpa->priv->percent;
}

static void
gedit_theatrics_highlight_search_animation_set_percent (GeditTheatricsAnimationDrawable *ad,
						    gdouble                          percent)
{
	GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (ad);

	rpa->priv->percent = percent;
}

static void
get_search_match_colors (GtkSourceBuffer *buffer,
			 gboolean        *foreground_set,
			 GdkColor        *foreground,
			 gboolean        *background_set,
			 GdkColor        *background)
{
	GtkSourceStyleScheme *style_scheme;
	GtkSourceStyle *style;
	gchar *fg, *bg;

	style_scheme = gtk_source_buffer_get_style_scheme (buffer);
	if (style_scheme == NULL)
		goto fallback;

	style = gtk_source_style_scheme_get_style (style_scheme,
						   "search-match");
	if (style == NULL)
		goto fallback;

	g_object_get (style,
		      "foreground-set", foreground_set,
		      "foreground", &fg,
		      "background-set", background_set,
		      "background", &bg,
		      NULL);

	if (*foreground_set)
	{
		if (fg == NULL ||
		    !gdk_color_parse (fg, foreground))
		{
			*foreground_set = FALSE;
		}
	}

	g_free (fg);

	if (*background_set)
	{
		if (bg == NULL ||
		    !gdk_color_parse (bg, background))
		{
			*background_set = FALSE;
		}
	}

	g_free (bg);

	return;

fallback:
	g_warning ("Falling back to hard-coded colors "
		   "for the \"found\" text tag.");

	gdk_color_parse ("#FFFF78", background);
	*background_set = TRUE;

	return;
}

static void
gedit_theatrics_highlight_search_animation_draw (GeditTheatricsAnimationDrawable *ad,
						 GdkDrawable                     *drawable)
{
	GeditTheatricsHighlightSearchAnimation *rpa = GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION (ad);
	GdkRectangle pos_start, pos_end;
	gint iw, ih;
	gdouble scale;
	cairo_t *cr;
	gint x, y;

	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (rpa->priv->view),
					 &rpa->priv->start, &pos_start);
	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (rpa->priv->view),
					 &rpa->priv->end, &pos_end);

	/* Width and height */
	iw = pos_end.x - pos_start.x;
	ih = MAX (pos_start.height, pos_end.height);

	if (rpa->priv->surface == NULL)
	{
		GtkSourceBuffer *buffer;
		PangoLayout *layout;
		PangoContext *context;
		PangoFontDescription *desc;
		gchar *text;
		gchar *markup;
		gboolean foreground_set, background_set;
		GdkColor fg_search_color, bg_search_color;

		buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (rpa->priv->view)));

		get_search_match_colors (buffer,
					 &foreground_set, &fg_search_color,
					 &background_set, &bg_search_color);

		rpa->priv->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
								 iw,
								 ih);

		cr = cairo_create (rpa->priv->surface);

		gedit_theatrics_utils_draw_round_rectangle (cr, TRUE, TRUE,
							    TRUE, TRUE,
							    0,
							    0,
							    4,
							    iw,
							    ih);

		if (background_set)
		{
			cairo_set_source_rgb (cr,
					      bg_search_color.red / 65535.,
					      bg_search_color.green / 65535.,
					      bg_search_color.blue / 65535.);
		}

		cairo_fill (cr);

		cairo_move_to (cr, 0, 0);

		if (foreground_set)
		{
			cairo_set_source_rgb (cr,
					      fg_search_color.red / 65535.,
					      fg_search_color.green / 65535.,
					      fg_search_color.blue / 65535.);
		}

		context = gtk_widget_get_pango_context (GTK_WIDGET (rpa->priv->view));
		layout = gtk_widget_create_pango_layout (GTK_WIDGET (rpa->priv->view),
							 NULL);

		desc = pango_context_get_font_description (context);
		pango_layout_set_font_description (layout, desc);

		text = gtk_text_iter_get_text (&rpa->priv->start,
					       &rpa->priv->end);
		markup = g_strdup_printf ("<b>%s</b>", text);

		g_free (text);

		pango_layout_set_markup (layout, markup, -1);
		g_free (markup);

		pango_cairo_show_layout (cr, layout);

		cairo_destroy (cr);
	}

	scale = 1 + rpa->priv->percent / 12;
	gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (rpa->priv->view),
					       GTK_TEXT_WINDOW_TEXT,
					       pos_start.x,
					       pos_start.y,
					       &x,
					       &y);

	cr = gdk_cairo_create (drawable);

	cairo_translate (cr, x - scale / 2, y - scale / 2);

	cairo_scale (cr, scale, scale);
	cairo_set_source_surface (cr, rpa->priv->surface, 0, 0);

	cairo_paint (cr);

	cairo_destroy (cr);
}

static void
gedit_theatrics_animation_drawable_iface_init (GeditTheatricsAnimationDrawableInterface *iface)
{
	iface->get_animation_bounds = gedit_theatrics_highlight_search_animation_get_animation_bounds;
	iface->get_percent = gedit_theatrics_highlight_search_animation_get_percent;
	iface->set_percent = gedit_theatrics_highlight_search_animation_set_percent;
	iface->draw = gedit_theatrics_highlight_search_animation_draw;
}

GeditTheatricsAnimationDrawable *
gedit_theatrics_highlight_search_animation_new (GtkSourceView *view,
						GtkTextIter   *start,
						GtkTextIter   *end)
{
	GeditTheatricsHighlightSearchAnimation *rpa;

	g_return_val_if_fail (GTK_IS_SOURCE_VIEW (view), NULL);
	g_return_val_if_fail (start != NULL && end != NULL, NULL);

	rpa = g_object_new (GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION,
			    "view", view, NULL);

	rpa->priv->start = *start;
	rpa->priv->end = *end;

	return GEDIT_THEATRICS_ANIMATION_DRAWABLE (rpa);
}
