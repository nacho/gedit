/*
 * gedit-rounded-frame.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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

#ifndef __GEDIT_ROUNDED_FRAME_H__
#define __GEDIT_ROUNDED_FRAME_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_ROUNDED_FRAME		(gedit_rounded_frame_get_type ())
#define GEDIT_ROUNDED_FRAME(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_ROUNDED_FRAME, GeditRoundedFrame))
#define GEDIT_ROUNDED_FRAME_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_ROUNDED_FRAME, GeditRoundedFrame const))
#define GEDIT_ROUNDED_FRAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_ROUNDED_FRAME, GeditRoundedFrameClass))
#define GEDIT_IS_ROUNDED_FRAME(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_ROUNDED_FRAME))
#define GEDIT_IS_ROUNDED_FRAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_ROUNDED_FRAME))
#define GEDIT_ROUNDED_FRAME_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_ROUNDED_FRAME, GeditRoundedFrameClass))

typedef struct _GeditRoundedFrame		GeditRoundedFrame;
typedef struct _GeditRoundedFrameClass		GeditRoundedFrameClass;
typedef struct _GeditRoundedFramePrivate	GeditRoundedFramePrivate;

struct _GeditRoundedFrame
{
	GtkBin parent;

	GeditRoundedFramePrivate *priv;
};

struct _GeditRoundedFrameClass
{
	GtkBinClass parent_class;
};

GType		 gedit_rounded_frame_get_type		(void) G_GNUC_CONST;

GtkWidget	*gedit_rounded_frame_new		(void);

G_END_DECLS

#endif /* __GEDIT_ROUNDED_FRAME_H__ */
