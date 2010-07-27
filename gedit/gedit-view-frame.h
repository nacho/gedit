/*
 * gedit-view-frame.h
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

#ifndef __GEDIT_VIEW_FRAME_H__
#define __GEDIT_VIEW_FRAME_H__

#include <gtk/gtk.h>
#include "gedit-document.h"
#include "gedit-view-interface.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_VIEW_FRAME			(gedit_view_frame_get_type ())
#define GEDIT_VIEW_FRAME(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_FRAME, GeditViewFrame))
#define GEDIT_VIEW_FRAME_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_FRAME, GeditViewFrame const))
#define GEDIT_VIEW_FRAME_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_VIEW_FRAME, GeditViewFrameClass))
#define GEDIT_IS_VIEW_FRAME(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_VIEW_FRAME))
#define GEDIT_IS_VIEW_FRAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_VIEW_FRAME))
#define GEDIT_VIEW_FRAME_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_VIEW_FRAME, GeditViewFrameClass))

typedef struct _GeditViewFrame		GeditViewFrame;
typedef struct _GeditViewFrameClass	GeditViewFrameClass;
typedef struct _GeditViewFramePrivate	GeditViewFramePrivate;

struct _GeditViewFrame
{
	GtkVBox parent;
	
	GeditViewFramePrivate *priv;
};

struct _GeditViewFrameClass
{
	GtkVBoxClass parent_class;
};

GType		 gedit_view_frame_get_type	(void) G_GNUC_CONST;

GeditViewFrame	*gedit_view_frame_new		(void);

GeditDocument	*gedit_view_frame_get_document	(GeditViewFrame *frame);

GeditView	*gedit_view_frame_get_view	(GeditViewFrame *frame);

void		 gedit_view_frame_set_active_view(GeditViewFrame *frame,
						 GeditView      *view);

GSList		*gedit_view_frame_get_views	(GeditViewFrame *frame);

void		 gedit_view_frame_set_text_view	(GeditViewFrame *frame);

void		 gedit_view_frame_set_web_view	(GeditViewFrame *frame);

G_END_DECLS

#endif /* __GEDIT_VIEW_FRAME_H__ */
