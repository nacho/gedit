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

typedef struct
{
	GtkIconSize  size;
	gint         width;
	gint         height;
	GdkPixbuf   *quiescent_pixbuf;
	GList       *images;
} GeditSpinnerImages;

typedef struct
{
	GdkScreen          *screen;
	GtkIconTheme       *icon_theme;
	GeditSpinnerImages *originals;

	/* List of GeditSpinnerImages scaled to different sizes */
	GList              *images;
} GeditSpinnerCacheData;

struct _GeditSpinnerCachePrivate
{
	/* Hash table of GdkScreen -> EphySpinnerCacheData */
	GHashTable *hash;
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
gedit_spinner_cache_data_unload (GeditSpinnerCacheData *data)
{
	g_return_if_fail (data != NULL);

	/* LOG ("GeditSpinnerDataCache unload for screen %p", data->screen); */

	g_list_foreach (data->images, (GFunc) gedit_spinner_images_free, NULL);
	data->images = NULL;
	data->originals = NULL;
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
gedit_spinner_cache_data_load (GeditSpinnerCacheData *data)
{
	GeditSpinnerImages *images;
	GdkPixbuf *icon_pixbuf, *pixbuf;
	GtkIconInfo *icon_info;
	int grid_width, grid_height, x, y, size, h, w;
	const char *icon;

	g_return_if_fail (data != NULL);

	/* LOG ("GeditSpinnerCacheData loading for screen %p", data->screen); */

	gedit_spinner_cache_data_unload (data);

	/* START_PROFILER ("loading spinner animation") */

	/* Load the animation */
	icon_info = gtk_icon_theme_lookup_icon (data->icon_theme,
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
	data->images = g_list_prepend (NULL, images);
	data->originals = images;

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
	icon_info = gtk_icon_theme_lookup_icon (data->icon_theme,
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

static GeditSpinnerCacheData *
gedit_spinner_cache_data_new (GdkScreen *screen)
{
	GeditSpinnerCacheData *data;

	data = g_new0 (GeditSpinnerCacheData, 1);

	data->screen = screen;
	data->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect_swapped (data->icon_theme,
				  "changed",
				  G_CALLBACK (gedit_spinner_cache_data_load),
				  data);

	gedit_spinner_cache_data_load (data);

	return data;
}

static void
gedit_spinner_cache_data_free (GeditSpinnerCacheData *data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (data->icon_theme != NULL);

	g_signal_handlers_disconnect_by_func (data->icon_theme,
					      G_CALLBACK (gedit_spinner_cache_data_load),
					      data);

	gedit_spinner_cache_data_unload (data);

	g_free (data);
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
				GdkScreen         *screen,
				GtkIconSize        size)
{
	GeditSpinnerCachePrivate *priv = cache->priv;
	GeditSpinnerCacheData *data;
	GeditSpinnerImages *images;
	GtkSettings *settings;
	GdkPixbuf *pixbuf, *scaled_pixbuf;
	GList *element, *l;
	int h, w;

	/* LOG ("Getting animation images at size %d for screen %p", size, screen); */

	data = g_hash_table_lookup (priv->hash, screen);
	{
		data = gedit_spinner_cache_data_new (screen);
		g_hash_table_insert (priv->hash, screen, data);
	}

	if (data->images == NULL || data->originals == NULL)
	{
		/* Load failed, but don't try endlessly again! */
		return NULL;
	}

	element = g_list_find_custom (data->images,
				      GINT_TO_POINTER (size),
				      (GCompareFunc) compare_size);
	if (element != NULL)
	{
		return gedit_spinner_images_copy ((GeditSpinnerImages *) element->data);
	}

	settings = gtk_settings_get_for_screen (screen);

	if (!gtk_icon_size_lookup_for_settings (settings, size, &w, &h))
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

	for (l = data->originals->images; l != NULL; l = l->next)
	{
		pixbuf = (GdkPixbuf *) l->data;
		scaled_pixbuf = scale_to_size (pixbuf, w, h);

		images->images = g_list_prepend (images->images, scaled_pixbuf);
	}
	images->images = g_list_reverse (images->images);

	images->quiescent_pixbuf =
		scale_to_size (data->originals->quiescent_pixbuf, w, h);

	/* store in cache */
	data->images = g_list_prepend (data->images, images);

	/* STOP_PROFILER ("scaling spinner animation") */

	return gedit_spinner_images_copy (images);
}

static void
gedit_spinner_cache_init (GeditSpinnerCache *cache)
{
	GeditSpinnerCachePrivate *priv;

	priv = cache->priv = GEDIT_SPINNER_CACHE_GET_PRIVATE (cache);

	/* LOG ("GeditSpinnerCache initialising"); */

	priv->hash = g_hash_table_new_full (g_direct_hash,
					    g_direct_equal,
					    NULL,
					    (GDestroyNotify) gedit_spinner_cache_data_free);
}

static void
gedit_spinner_cache_finalize (GObject *object)
{
	GeditSpinnerCache *cache = GEDIT_SPINNER_CACHE (object); 
	GeditSpinnerCachePrivate *priv = cache->priv;

	/* LOG ("GeditSpinnerCache finalising"); */

	g_hash_table_destroy (priv->hash);

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

#define SPINNER_TIMEOUT 125 /* ms */

#define GEDIT_SPINNER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SPINNER, GeditSpinnerPrivate))

struct _GeditSpinnerPrivate
{
	GtkIconTheme       *icon_theme;
	GeditSpinnerCache  *cache;
	GtkIconSize         size;
	GeditSpinnerImages *images;
	GList              *current_image;
	guint               timeout;
	guint               timer_task;
	guint               spinning : 1;
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
	GeditSpinnerPrivate *priv = spinner->priv;

	if (priv->images == NULL)
	{
		/* START_PROFILER ("gedit_spinner_load_images") */

		priv->images =
			gedit_spinner_cache_get_images (priv->cache,
							gtk_widget_get_screen (GTK_WIDGET (spinner)),
							priv->size);

		if (priv->images != NULL)
		{
			priv->current_image = priv->images->images;
		}

		/* STOP_PROFILER ("gedit_spinner_load_images") */
	}

	return priv->images != NULL;
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
	GeditSpinnerPrivate *priv = spinner->priv;
	GtkWidget *widget = GTK_WIDGET (spinner);

	gtk_widget_set_events (widget,
			       gtk_widget_get_events (widget)
			       | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

	priv = spinner->priv = GEDIT_SPINNER_GET_PRIVATE (spinner);

	priv->cache = gedit_spinner_cache_ref ();
	priv->size = GTK_ICON_SIZE_INVALID;
	priv->spinning = FALSE;
	priv->timeout = SPINNER_TIMEOUT;
}

static GdkPixbuf *
select_spinner_image (GeditSpinner *spinner)
{
	GeditSpinnerPrivate *priv = spinner->priv;
	GeditSpinnerImages *images = priv->images;

	g_return_val_if_fail (images != NULL, NULL);

	if (spinner->priv->timer_task == 0)
	{
		if (images->quiescent_pixbuf != NULL)
		{
			return g_object_ref (priv->images->quiescent_pixbuf);
		}

		return NULL;
	}

	g_return_val_if_fail (priv->current_image != NULL, NULL);

	return g_object_ref (priv->current_image->data);
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
	GeditSpinnerPrivate *priv = spinner->priv;
	GList *frame;

	if (!GTK_WIDGET_DRAWABLE (spinner))
	{
		return TRUE;
	}

	/* This can happen when we've unloaded the images on a theme
	 * change, but haven't been in the queued size request yet.
	 * Just skip this update.
	 */
	if (priv->images == NULL)
		return TRUE;

	frame = priv->current_image;
	/* g_assert (frame != NULL); */

	if (frame->next != NULL)
	{
		frame = frame->next;
	}
	else
	{
		frame = g_list_first (frame);
	}

	priv->current_image = frame;

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
	GeditSpinnerPrivate *priv = spinner->priv;

	priv->spinning = TRUE;

	if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)) &&
			       priv->timer_task == 0 &&
			       gedit_spinner_load_images (spinner))
	{
		if (priv->images != NULL)
		{
			/* reset to first frame */
			priv->current_image = priv->images->images;
		}

		priv->timer_task = g_timeout_add (priv->timeout,
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
	GeditSpinnerPrivate *priv = spinner->priv;

	priv->spinning = FALSE;

	if (spinner->priv->timer_task != 0)
	{
		gedit_spinner_remove_update_callback (spinner);

		if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)))
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

#if 0
/*
* gedit_spinner_set_timeout:
* @spinner: a #GeditSpinner
* @timeout: time delay between updates to the spinner.
*
* Sets the timeout delay for spinner updates.
**/
void
gedit_spinner_set_timeout (GeditSpinner *spinner,
			   guint         timeout)
{
	GeditSpinnerPrivate *priv = spinner->priv;

	if (timeout != priv->timeout)
	{
		gedit_spinner_stop (spinner);

		priv->timeout = timeout;

		if (priv->spinning)
		{
			gedit_spinner_start (spinner);
		}
	}
}
#endif

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
gedit_spinner_map (GtkWidget *widget)
{
	GeditSpinner *spinner = GEDIT_SPINNER (widget);
	GeditSpinnerPrivate *priv = spinner->priv;

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	if (priv->spinning)
	{
		gedit_spinner_start (spinner);
	}
}

static void
gedit_spinner_unmap (GtkWidget *widget)
{
	GeditSpinner *spinner = GEDIT_SPINNER (widget);

	gedit_spinner_remove_update_callback (spinner);

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gedit_spinner_dispose (GObject *object)
{
	GeditSpinner *spinner = GEDIT_SPINNER (object);

	g_signal_handlers_disconnect_by_func
			(spinner->priv->icon_theme,
			 G_CALLBACK (icon_theme_changed_cb), spinner);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gedit_spinner_finalize (GObject *object)
{
	GeditSpinner *spinner = GEDIT_SPINNER (object);

	gedit_spinner_remove_update_callback (spinner);
	gedit_spinner_unload_images (spinner);

	g_object_unref (spinner->priv->cache);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gedit_spinner_screen_changed (GtkWidget *widget,
			      GdkScreen *old_screen)
{
	GeditSpinner *spinner = GEDIT_SPINNER (widget);
	GeditSpinnerPrivate *priv = spinner->priv;
	GdkScreen *screen;

	if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
	{
		GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, old_screen);
	}

	screen = gtk_widget_get_screen (widget);

	/* FIXME: this seems to be happening when then spinner is destroyed!? */
	if (old_screen == screen)
		return;

	/* We'll get mapped again on the new screen, but not unmapped from
	 * the old screen, so remove timeout here.
	 */
	gedit_spinner_remove_update_callback (spinner);

	gedit_spinner_unload_images (spinner);

	if (old_screen != NULL)
	{
		g_signal_handlers_disconnect_by_func
			(gtk_icon_theme_get_for_screen (old_screen),
			 G_CALLBACK (icon_theme_changed_cb), spinner);
	}

	priv->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect (priv->icon_theme, "changed",
			  G_CALLBACK (icon_theme_changed_cb), spinner);
}

static void
gedit_spinner_class_init (GeditSpinnerClass *class)
{
	GObjectClass *object_class =  G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->dispose = gedit_spinner_dispose;
	object_class->finalize = gedit_spinner_finalize;

	widget_class->expose_event = gedit_spinner_expose;
	widget_class->size_request = gedit_spinner_size_request;
	widget_class->map = gedit_spinner_map;
	widget_class->unmap = gedit_spinner_unmap;
	widget_class->screen_changed = gedit_spinner_screen_changed;

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
