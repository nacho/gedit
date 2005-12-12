/*
 * gedit-spinner.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
 * Copyright (C) 2002-2004 Marco Pesenti Gritti
 * Copyright (C) 2004 Christian Persch
 * Copyright (C) 2000 - Eazel, Inc. 
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
 * This widget was originally written by Andy Hertzfeld <andy@eazel.com> for
 * Nautilus. It was then modified by Marco Pesenti Gritti and Christian Persch
 * for Epiphany.
 *
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-spinner.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtksettings.h>

/* Spinner cache implementation */

#define GEDIT_TYPE_SPINNER_CACHE		(gedit_spinner_cache_get_type())
#define GEDIT_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), GEDIT_TYPE_SPINNER_CACHE, GeditSpinnerCache))
#define GEDIT_SPINNER_CACHE_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_SPINNER_CACHE, GeditSpinnerCacheClass))
#define GEDIT_IS_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), GEDIT_TYPE_SPINNER_CACHE))
#define GEDIT_IS_SPINNER_CACHE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GEDIT_TYPE_SPINNER_CACHE))
#define GEDIT_SPINNER_CACHE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_SPINNER_CACHE, GeditSpinnerCacheClass))

typedef struct _GeditSpinnerCache		GeditSpinnerCache;
typedef struct _GeditSpinnerCacheClass		GeditSpinnerCacheClass;
typedef struct _GeditSpinnerCachePrivate	GeditSpinnerCachePrivate;
typedef struct _GeditSpinnerImages		GeditSpinnerImages;

struct _GeditSpinnerImages
{
	GtkIconSize  size;
	gint         width;
	gint         height;
	GdkPixbuf   *quiescent_pixbuf;
	GList       *images;
};

struct _GeditSpinnerCacheClass
{
	GObjectClass parent_class;
};

struct _GeditSpinnerCache
{
	GObject parent_object;

	/*< private >*/
	GeditSpinnerCachePrivate *priv;
};

#define GEDIT_SPINNER_CACHE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SPINNER_CACHE, GeditSpinnerCachePrivate))

struct _GeditSpinnerCachePrivate
{
	GtkIconTheme       *icon_theme;
	GeditSpinnerImages *originals;
	
	/* List of GeditSpinnerImages scaled to different sizes */
	GList              *images;
};

static void gedit_spinner_cache_class_init	(GeditSpinnerCacheClass *klass);
static void gedit_spinner_cache_init		(GeditSpinnerCache      *cache);

static GObjectClass *cache_parent_class = NULL;

static GType
gedit_spinner_cache_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo our_info =
		{
			sizeof (GeditSpinnerCacheClass),
			NULL,
			NULL,
			(GClassInitFunc) gedit_spinner_cache_class_init,
			NULL,
			NULL,
			sizeof (GeditSpinnerCache),
			0,
			(GInstanceInitFunc) gedit_spinner_cache_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GeditSpinnerCache",
					       &our_info, 0);
	}

	return type;
}

static void
gedit_spinner_images_free (GeditSpinnerImages *images)
{
	if (images != NULL)
	{
		g_list_foreach (images->images, (GFunc) g_object_unref, NULL);
		g_object_unref (images->quiescent_pixbuf);

		g_free (images);
	}
}

static GeditSpinnerImages *
gedit_spinner_images_copy (GeditSpinnerImages *images)
{
	GeditSpinnerImages *copy = g_new (GeditSpinnerImages, 1);

	copy->size = images->size;
	copy->width = images->width;
	copy->height = images->height;

	copy->quiescent_pixbuf = g_object_ref (images->quiescent_pixbuf);
	copy->images = g_list_copy (images->images);
	g_list_foreach (copy->images, (GFunc) g_object_ref, NULL);

	return copy;
}

static void
gedit_spinner_cache_unload (GeditSpinnerCache *cache)
{
	g_list_foreach (cache->priv->images, (GFunc) gedit_spinner_images_free, NULL);
	cache->priv->images = NULL;
	cache->priv->originals = NULL;
}

static GdkPixbuf *
extract_frame (GdkPixbuf *grid_pixbuf,
	       int x,
	       int y,
	       int size)
{
	GdkPixbuf *pixbuf;

	if (x + size > gdk_pixbuf_get_width (grid_pixbuf) ||
	    y + size > gdk_pixbuf_get_height (grid_pixbuf))
	{
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_subpixbuf (grid_pixbuf,
					   x, y,
					   size, size);
	g_return_val_if_fail (pixbuf != NULL, NULL);

	return pixbuf;
}

static void
gedit_spinner_cache_load (GeditSpinnerCache *cache)
{
	GeditSpinnerImages *images;
	GdkPixbuf *icon_pixbuf, *pixbuf;
	GtkIconInfo *icon_info;
	int grid_width, grid_height, x, y, size, h, w;
	const char *icon;

	/* LOG ("GeditSpinnerCache loading"); */

	gedit_spinner_cache_unload (cache);

	/* START_PROFILER ("loading spinner animation") */

	/* Load the animation */
	icon_info = gtk_icon_theme_lookup_icon (cache->priv->icon_theme,
						"gnome-spinner", -1, 0);
	if (icon_info == NULL)
	{
		/* STOP_PROFILER ("loading spinner animation") */

		g_warning ("Throbber animation not found.");
		return;
	}

	size = gtk_icon_info_get_base_size (icon_info);
	icon = gtk_icon_info_get_filename (icon_info);
	g_return_if_fail (icon != NULL);

	icon_pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
	if (icon_pixbuf == NULL)
	{
		/* STOP_PROFILER ("loading spinner animation") */

		g_warning ("Could not load the spinner file.");
		gtk_icon_info_free (icon_info);
		return;
	}

	grid_width = gdk_pixbuf_get_width (icon_pixbuf);
	grid_height = gdk_pixbuf_get_height (icon_pixbuf);

	images = g_new (GeditSpinnerImages, 1);
	cache->priv->images = g_list_prepend (NULL, images);
	cache->priv->originals = images;

	images->size = GTK_ICON_SIZE_INVALID;
	images->width = images->height = size;
	images->images = NULL;
	images->quiescent_pixbuf = NULL;

	for (y = 0; y < grid_height; y += size)
	{
		for (x = 0; x < grid_width ; x += size)
		{
			pixbuf = extract_frame (icon_pixbuf, x, y, size);

			if (pixbuf)
			{
				images->images =
					g_list_prepend (images->images, pixbuf);
			}
			else
			{
				g_warning ("Cannot extract frame from the grid.");
			}
		}
	}
	images->images = g_list_reverse (images->images);

	gtk_icon_info_free (icon_info);
	g_object_unref (icon_pixbuf);

	/* Load the rest icon */
	icon_info = gtk_icon_theme_lookup_icon (cache->priv->icon_theme,
						"gnome-spinner-rest", -1, 0);
	if (icon_info == NULL)
	{
		/* STOP_PROFILER ("loading spinner animation") */

		g_warning ("Throbber rest icon not found.");
		return;
	}

	size = gtk_icon_info_get_base_size (icon_info);
	icon = gtk_icon_info_get_filename (icon_info);
	g_return_if_fail (icon != NULL);

	icon_pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
	gtk_icon_info_free (icon_info);

	if (icon_pixbuf == NULL)
	{
		/* STOP_PROFILER ("loading spinner animation") */

		g_warning ("Could not load spinner rest icon.");
		gedit_spinner_images_free (images);
		return;
	}

	images->quiescent_pixbuf = icon_pixbuf;

	w = gdk_pixbuf_get_width (icon_pixbuf);
	h = gdk_pixbuf_get_height (icon_pixbuf);
	images->width = MAX (images->width, w);
	images->height = MAX (images->height, h);

	/* STOP_PROFILER ("loading spinner animation") */
}

static int
compare_size (gconstpointer images_ptr,
	      gconstpointer size_ptr)
{
	const GeditSpinnerImages *images = (const GeditSpinnerImages *) images_ptr;
	GtkIconSize size = GPOINTER_TO_INT (size_ptr);

	if (images->size == size)
	{
		return 0;
	}

	return -1;
}

static GdkPixbuf *
scale_to_size (GdkPixbuf *pixbuf,
	       int dw,
	       int dh)
{
	GdkPixbuf *result;
	int pw, ph;

	pw = gdk_pixbuf_get_width (pixbuf);
	ph = gdk_pixbuf_get_height (pixbuf);

	if (pw != dw || ph != dh)
	{
		result = gdk_pixbuf_scale_simple (pixbuf, dw, dh,
						  GDK_INTERP_BILINEAR);
	}
	else
	{
		result = g_object_ref (pixbuf);
	}

	return result;
}

static GeditSpinnerImages *
gedit_spinner_cache_get_images (GeditSpinnerCache *cache,
			       GtkIconSize size)
{
	GeditSpinnerImages *images;
	GdkPixbuf *pixbuf, *scaled_pixbuf;
	GList *element, *l;
	int h, w;

	/* LOG ("Getting animation images at size %d", size); */

	if (cache->priv->images == NULL || cache->priv->originals == NULL)
	{
		return NULL;
	}

	element = g_list_find_custom (cache->priv->images,
				      GINT_TO_POINTER (size),
				      (GCompareFunc) compare_size);
	if (element != NULL)
	{
		return gedit_spinner_images_copy ((GeditSpinnerImages *) element->data);
	}

	if (!gtk_icon_size_lookup_for_settings (gtk_settings_get_default (), size, &w, &h))
	{
		g_warning ("Failed to lookup icon size.");
		return NULL;
	}

	images = g_new (GeditSpinnerImages, 1);
	images->size = size;
	images->width = w;
	images->height = h;
	images->images = NULL;

	/* START_PROFILER ("scaling spinner animation") */

	for (l = cache->priv->originals->images; l != NULL; l = l->next)
	{
		pixbuf = (GdkPixbuf *) l->data;
		scaled_pixbuf = scale_to_size (pixbuf, w, h);

		images->images = g_list_prepend (images->images, scaled_pixbuf);
	}
	images->images = g_list_reverse (images->images);

	images->quiescent_pixbuf =
		scale_to_size (cache->priv->originals->quiescent_pixbuf, w, h);

	/* store in cache */
	cache->priv->images = g_list_prepend (cache->priv->images, images);

	/* STOP_PROFILER ("scaling spinner animation") */

	return gedit_spinner_images_copy (images);
}

static void
gedit_spinner_cache_init (GeditSpinnerCache *cache)
{
	cache->priv = GEDIT_SPINNER_CACHE_GET_PRIVATE (cache);

	/* LOG ("GeditSpinnerCache initialising"); */

	/* FIXME: icon theme is per-screen, not global */
	cache->priv->icon_theme = gtk_icon_theme_get_default ();
	g_signal_connect_swapped (cache->priv->icon_theme, "changed",
				  G_CALLBACK (gedit_spinner_cache_load), cache);

	gedit_spinner_cache_load (cache);
}

static void
gedit_spinner_cache_finalize (GObject *object)
{
	GeditSpinnerCache *cache = GEDIT_SPINNER_CACHE (object); 

	/* LOG ("GeditSpinnerCache finalising"); */

	g_signal_handlers_disconnect_by_func
		(cache->priv->icon_theme, G_CALLBACK(gedit_spinner_cache_load), cache);

	gedit_spinner_cache_unload (cache);

	G_OBJECT_CLASS (cache_parent_class)->finalize (object);
}

static void
gedit_spinner_cache_class_init (GeditSpinnerCacheClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	cache_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gedit_spinner_cache_finalize;

	g_type_class_add_private (object_class, sizeof (GeditSpinnerCachePrivate));
}

static GeditSpinnerCache *spinner_cache = NULL;

static GeditSpinnerCache *
gedit_spinner_cache_ref (void)
{
	if (spinner_cache == NULL)
	{
		GeditSpinnerCache **cache_ptr;

		spinner_cache = g_object_new (GEDIT_TYPE_SPINNER_CACHE, NULL);
		cache_ptr = &spinner_cache;
		g_object_add_weak_pointer (G_OBJECT (spinner_cache),
					   (gpointer *) cache_ptr);

		return spinner_cache;
	}
	else
	{
		return g_object_ref (spinner_cache);
	}
}

/* Spinner implementation */

#define SPINNER_TIMEOUT 100	/* Milliseconds Per Frame */

#define GEDIT_SPINNER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SPINNER, GeditSpinnerPrivate))

struct _GeditSpinnerPrivate
{
	GtkIconTheme       *icon_theme;
	GeditSpinnerCache  *cache;
	GtkIconSize         size;
	GeditSpinnerImages *images;
	GList              *current_image;
	guint               timer_task;
};

static void gedit_spinner_class_init	(GeditSpinnerClass *class);
static void gedit_spinner_init		(GeditSpinner      *spinner);

static GObjectClass *parent_class = NULL;

GType
gedit_spinner_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo our_info =
		{
			sizeof (GeditSpinnerClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) gedit_spinner_class_init,
			NULL,
			NULL, /* class_data */
			sizeof (GeditSpinner),
			0, /* n_preallocs */
			(GInstanceInitFunc) gedit_spinner_init
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX,
					       "GeditSpinner",
					       &our_info, 0);
	}

	return type;
}

static gboolean
gedit_spinner_load_images (GeditSpinner *spinner)
{
	GeditSpinnerPrivate *details = spinner->priv;

	if (details->images == NULL)
	{
		/* START_PROFILER ("gedit_spinner_load_images") */

		details->images =
			gedit_spinner_cache_get_images (details->cache, details->size);

		if (details->images != NULL)
		{
			details->current_image = details->images->images;
		}

		/* STOP_PROFILER ("gedit_spinner_load_images") */
	}

	return details->images != NULL;
}

static void
gedit_spinner_unload_images (GeditSpinner *spinner)
{
	gedit_spinner_images_free (spinner->priv->images);
	spinner->priv->images = NULL;
	spinner->priv->current_image = NULL;
}

static void
icon_theme_changed_cb (GtkIconTheme *icon_theme,
		       GeditSpinner *spinner)
{
	gedit_spinner_unload_images (spinner);
	gtk_widget_queue_resize (GTK_WIDGET (spinner));
}

static void
gedit_spinner_init (GeditSpinner *spinner)
{
	GtkWidget *widget = GTK_WIDGET (spinner);

	gtk_widget_set_events (widget,
			       gtk_widget_get_events (widget)
			       | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

	spinner->priv = GEDIT_SPINNER_GET_PRIVATE (spinner);

	spinner->priv->cache = gedit_spinner_cache_ref ();
	spinner->priv->size = GTK_ICON_SIZE_INVALID;

	/* FIXME: icon theme is per-screen, not global */
	spinner->priv->icon_theme = gtk_icon_theme_get_default ();
	g_signal_connect (spinner->priv->icon_theme, "changed",
			  G_CALLBACK (icon_theme_changed_cb), spinner);

}

static GdkPixbuf *
select_spinner_image (GeditSpinner *spinner)
{
	GeditSpinnerPrivate *details = spinner->priv;
	GeditSpinnerImages *images = details->images;

	g_return_val_if_fail (images != NULL, NULL);

	if (spinner->priv->timer_task == 0)
	{
		if (images->quiescent_pixbuf != NULL)
		{
			return g_object_ref (details->images->quiescent_pixbuf);
		}

		return NULL;
	}

	g_return_val_if_fail (details->current_image != NULL, NULL);

	return g_object_ref (details->current_image->data);
}

static int
gedit_spinner_expose (GtkWidget *widget,
		     GdkEventExpose *event)
{
	GeditSpinner *spinner = GEDIT_SPINNER (widget);
	GdkPixbuf *pixbuf;
	GdkGC *gc;
	int x_offset, y_offset, width, height;
	GdkRectangle pix_area, dest;

	if (!GTK_WIDGET_DRAWABLE (spinner))
	{
		return TRUE;
	}

	if (!gedit_spinner_load_images (spinner))
	{
		return TRUE;
	}

	pixbuf = select_spinner_image (spinner);
	if (pixbuf == NULL)
	{
		return FALSE;
	}

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	/* Compute the offsets for the image centered on our allocation */
	x_offset = (widget->allocation.width - width) / 2;
	y_offset = (widget->allocation.height - height) / 2;

	pix_area.x = x_offset + widget->allocation.x;
	pix_area.y = y_offset + widget->allocation.y;
	pix_area.width = width;
	pix_area.height = height;

	if (!gdk_rectangle_intersect (&event->area, &pix_area, &dest))
	{
		g_object_unref (pixbuf);
		return FALSE;
	}

	gc = gdk_gc_new (widget->window);
	gdk_draw_pixbuf (widget->window, gc, pixbuf,
			 dest.x - x_offset - widget->allocation.x,
			 dest.y - y_offset - widget->allocation.y,
			 dest.x, dest.y,
			 dest.width, dest.height,
			 GDK_RGB_DITHER_MAX, 0, 0);
	g_object_unref (gc);

	g_object_unref (pixbuf);

	return FALSE;
}

static gboolean
bump_spinner_frame_cb (GeditSpinner *spinner)
{
	GList *frame;

	if (!GTK_WIDGET_DRAWABLE (spinner))
	{
		return TRUE;
	}

	frame = spinner->priv->current_image;

	if (g_list_next (frame) != NULL)
	{
		frame = frame->next;
	}
	else
	{
		frame = g_list_first (frame);
	}

	spinner->priv->current_image = frame;

	gtk_widget_queue_draw (GTK_WIDGET (spinner));

	/* run again */
	return TRUE;
}

/**
 * gedit_spinner_start:
 * @spinner: a #GeditSpinner
 *
 * Start the spinner animation.
 **/
void
gedit_spinner_start (GeditSpinner *spinner)
{
	if (spinner->priv->timer_task == 0)
	{

		if (spinner->priv->images != NULL)
		{
			/* reset to first frame */
			spinner->priv->current_image =
				spinner->priv->images->images;
		}

		spinner->priv->timer_task =
			g_timeout_add (SPINNER_TIMEOUT,
				       (GSourceFunc) bump_spinner_frame_cb,
				       spinner);
	}
}

static void
gedit_spinner_remove_update_callback (GeditSpinner *spinner)
{
	if (spinner->priv->timer_task != 0)
	{
		g_source_remove (spinner->priv->timer_task);
		spinner->priv->timer_task = 0;
	}
}

/**
 * gedit_spinner_stop:
 * @spinner: a #GeditSpinner
 *
 * Stop the spinner animation.
 **/
void
gedit_spinner_stop (GeditSpinner *spinner)
{
	if (spinner->priv->timer_task != 0)
	{
		gedit_spinner_remove_update_callback (spinner);
		gtk_widget_queue_draw (GTK_WIDGET (spinner));
	}
}

/*
 * gedit_spinner_set_size:
 * @spinner: a #GeditSpinner
 * @size: the size of type %GtkIconSize
 *
 * Set the size of the spinner. Use %GTK_ICON_SIZE_INVALID to use the
 * native size of the icons.
 **/
void
gedit_spinner_set_size (GeditSpinner *spinner,
		       GtkIconSize size)
{
	if (size != spinner->priv->size)
	{
		gedit_spinner_unload_images (spinner);

		spinner->priv->size = size;

		gtk_widget_queue_resize (GTK_WIDGET (spinner));
	}
}

static void
gedit_spinner_size_request (GtkWidget *widget,
			   GtkRequisition *requisition)
{
	GeditSpinner *spinner = GEDIT_SPINNER (widget);

	if (!gedit_spinner_load_images (spinner))
	{
		requisition->width = requisition->height = 0;
		gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
						   spinner->priv->size,
						   &requisition->width,
					           &requisition->height);
		return;
	}

	requisition->width = spinner->priv->images->width;
	requisition->height = spinner->priv->images->height;

	/* allocate some extra margin so we don't butt up against toolbar edges */
	if (spinner->priv->size != GTK_ICON_SIZE_MENU)
	{
		requisition->width += 4;
		requisition->height += 4;
	}
}

static void
gedit_spinner_finalize (GObject *object)
{
	GeditSpinner *spinner = GEDIT_SPINNER (object);

	g_signal_handlers_disconnect_by_func
		(spinner->priv->icon_theme,
		 G_CALLBACK (icon_theme_changed_cb), spinner);

	gedit_spinner_remove_update_callback (spinner);
	gedit_spinner_unload_images (spinner);

	g_object_unref (spinner->priv->cache);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gedit_spinner_class_init (GeditSpinnerClass *class)
{
	GObjectClass *object_class =  G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->finalize = gedit_spinner_finalize;

	widget_class->expose_event = gedit_spinner_expose;
	widget_class->size_request = gedit_spinner_size_request;

	g_type_class_add_private (object_class, sizeof (GeditSpinnerPrivate));
}

/*
 * gedit_spinner_new:
 *
 * Create a new #GeditSpinner. The spinner is a widget
 * that gives the user feedback about network status with
 * an animated image.
 *
 * Return Value: the spinner #GtkWidget
 **/
GtkWidget *
gedit_spinner_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_SPINNER,
					 "visible-window", FALSE,
					 NULL));
}
