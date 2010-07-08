/*
 * gedit-view-frame.c
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

#include "gedit-view-frame.h"
#include "gedit-text-view.h"


#define GEDIT_VIEW_FRAME_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_VIEW_FRAME, GeditViewFramePrivate))

struct _GeditViewFramePrivate
{
	GtkWidget     *view;
};

enum
{
	PROP_0,
	PROP_DOCUMENT,
	PROP_VIEW
};

G_DEFINE_TYPE (GeditViewFrame, gedit_view_frame, GTK_TYPE_VBOX)

static void
gedit_view_frame_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_view_frame_parent_class)->finalize (object);
}

static void
gedit_view_frame_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value,
			                    gedit_view_frame_get_document (frame));
			break;
		case PROP_VIEW:
			g_value_set_object (value,
			                    gedit_view_frame_get_view (frame));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_view_frame_class_init (GeditViewFrameClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_view_frame_finalize;
	object_class->get_property = gedit_view_frame_get_property;

	g_object_class_install_property (object_class, PROP_DOCUMENT,
	                                 g_param_spec_object ("document",
	                                                      "Document",
	                                                      "The Document",
	                                                      GEDIT_TYPE_DOCUMENT,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_VIEW,
	                                 g_param_spec_object ("view",
	                                                      "View",
	                                                      "The View",
	                                                      GEDIT_TYPE_VIEW,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditViewFramePrivate));
}

static GMountOperation *
view_frame_mount_operation_factory (GeditDocument   *doc,
				    gpointer         user_data)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (user_data);
	GtkWidget *window;

	window = gtk_widget_get_toplevel (GTK_WIDGET (frame));

	return gtk_mount_operation_new (GTK_WINDOW (window));
}

static void
gedit_view_frame_init (GeditViewFrame *frame)
{
	GeditDocument *doc;
	GtkWidget *sw;

	frame->priv = GEDIT_VIEW_FRAME_GET_PRIVATE (frame);

	doc = gedit_document_new ();

	_gedit_document_set_mount_operation_factory (doc,
						     view_frame_mount_operation_factory,
						     frame);

	frame->priv->view = gedit_text_view_new (doc);
	gtk_widget_show (frame->priv->view);

	g_object_unref (doc);

	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (sw), frame->priv->view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);
	gtk_widget_show (sw);

	gtk_box_pack_start (GTK_BOX (frame), sw, TRUE, TRUE, 0);
}

GeditViewFrame *
gedit_view_frame_new ()
{
	return g_object_new (GEDIT_TYPE_VIEW_FRAME, NULL);
}

GeditDocument *
gedit_view_frame_get_document (GeditViewFrame *frame)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_FRAME (frame), NULL);

	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->priv->view)));
}

GeditView *
gedit_view_frame_get_view (GeditViewFrame *frame)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_FRAME (frame), NULL);

	return GEDIT_VIEW (frame->priv->view);
}
