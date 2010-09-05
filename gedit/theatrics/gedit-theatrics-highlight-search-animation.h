/*
 * gedit-theatrics-highlight-search-animation.h
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

#ifndef __GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_H__
#define __GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include "gedit-theatrics-animation-drawable.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION		(gedit_theatrics_highlight_search_animation_get_type ())
#define GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION, GeditTheatricsHighlightSearchAnimation))
#define GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION, GeditTheatricsHighlightSearchAnimation const))
#define GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION, GeditTheatricsHighlightSearchAnimationClass))
#define GEDIT_IS_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION))
#define GEDIT_IS_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION))
#define GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION, GeditTheatricsHighlightSearchAnimationClass))

typedef struct _GeditTheatricsHighlightSearchAnimation		GeditTheatricsHighlightSearchAnimation;
typedef struct _GeditTheatricsHighlightSearchAnimationClass		GeditTheatricsHighlightSearchAnimationClass;
typedef struct _GeditTheatricsHighlightSearchAnimationPrivate	GeditTheatricsHighlightSearchAnimationPrivate;

struct _GeditTheatricsHighlightSearchAnimation
{
	GObject parent;

	GeditTheatricsHighlightSearchAnimationPrivate *priv;
};

struct _GeditTheatricsHighlightSearchAnimationClass
{
	GObjectClass parent_class;
};

GType gedit_theatrics_highlight_search_animation_get_type (void) G_GNUC_CONST;

GeditTheatricsAnimationDrawable *gedit_theatrics_highlight_search_animation_new (GtkSourceView *view,
										 GtkTextIter   *start,
										 GtkTextIter   *end);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_HIGHLIGHT_SEARCH_ANIMATION_H__ */
