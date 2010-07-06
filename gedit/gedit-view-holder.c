/*
 * gedit-view-holder.c
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

#include "gedit-view-holder.h"


#define GEDIT_VIEW_HOLDER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_VIEW_HOLDER, GeditViewHolderPrivate))

struct _GeditViewHolderPrivate
{
	GtkWidget     *view;
	GeditDocument *doc;
};

G_DEFINE_TYPE (GeditViewHolder, gedit_view_holder, GTK_TYPE_VBOX)

static void
gedit_view_holder_dispose (GObject *object)
{
	GeditViewHolder *holder = GEDIT_VIEW_HOLDER (object);

	if (holder->priv->doc != NULL)
	{
		g_object_unref (holder->priv->doc);
		holder->priv->doc = NULL;
	}

	G_OBJECT_CLASS (gedit_view_holder_parent_class)->dispose (object);
}

static void
gedit_view_holder_class_init (GeditViewHolderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_view_holder_dispose;

	g_type_class_add_private (object_class, sizeof (GeditViewHolderPrivate));
}

static GMountOperation *
view_holder_mount_operation_factory (GeditDocument   *doc,
				     gpointer         user_data)
{
	GeditViewHolder *holder = GEDIT_VIEW_HOLDER (user_data);
	GtkWidget *window;

	window = gtk_widget_get_toplevel (GTK_WIDGET (holder));

	return gtk_mount_operation_new (GTK_WINDOW (window));
}

static void
gedit_view_holder_init (GeditViewHolder *holder)
{
	GtkWidget *sw;

	holder->priv = GEDIT_VIEW_HOLDER_GET_PRIVATE (holder);

	holder->priv->doc = gedit_document_new ();

	_gedit_document_set_mount_operation_factory (holder->priv->doc,
						     view_holder_mount_operation_factory,
						     holder);

	holder->priv->view = gedit_view_new (holder->priv->doc);
	gtk_widget_show (holder->priv->view);

	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (sw), holder->priv->view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);
	gtk_widget_show (sw);

	gtk_box_pack_start (GTK_BOX (holder), sw, TRUE, TRUE, 0);
}

GeditViewHolder *
gedit_view_holder_new ()
{
	return g_object_new (GEDIT_TYPE_VIEW_HOLDER, NULL);
}

GeditDocument *
gedit_view_holder_get_document (GeditViewHolder *holder)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_HOLDER (holder), NULL);

	return holder->priv->doc;
}

GeditView *
gedit_view_holder_get_view (GeditViewHolder *holder)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_HOLDER (holder), NULL);

	return GEDIT_VIEW (holder->priv->view);
}
