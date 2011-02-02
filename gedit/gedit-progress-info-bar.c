/*
 * gedit-progress-info-bar.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */
 
 /* TODO: add properties */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gedit-progress-info-bar.h"

enum {
	PROP_0,
	PROP_HAS_CANCEL_BUTTON
};


#define GEDIT_PROGRESS_INFO_BAR_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PROGRESS_INFO_BAR, GeditProgressInfoBarPrivate))

struct _GeditProgressInfoBarPrivate
{
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *progress;
};

G_DEFINE_TYPE(GeditProgressInfoBar, gedit_progress_info_bar, GTK_TYPE_INFO_BAR)

static void
gedit_progress_info_bar_set_has_cancel_button (GeditProgressInfoBar *bar,
					       gboolean              has_button)
{
	if (has_button)
		gtk_info_bar_add_button (GTK_INFO_BAR (bar),
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL);

	g_object_notify (G_OBJECT (bar), "has-cancel-button");
}

static void
gedit_progress_info_bar_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
	GeditProgressInfoBar *bar;

	bar = GEDIT_PROGRESS_INFO_BAR (object);

	switch (prop_id)
	{
		case PROP_HAS_CANCEL_BUTTON:
			gedit_progress_info_bar_set_has_cancel_button (bar,
								       g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void 
gedit_progress_info_bar_class_init (GeditProgressInfoBarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = gedit_progress_info_bar_set_property;

	g_object_class_install_property (gobject_class,
					 PROP_HAS_CANCEL_BUTTON,
					 g_param_spec_boolean ("has-cancel-button",
							       "Has Cancel Button",
							       "If the message bar has a cancel button",
							       TRUE,
							       G_PARAM_WRITABLE |
							       G_PARAM_CONSTRUCT_ONLY |
							       G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (gobject_class, sizeof (GeditProgressInfoBarPrivate));
}

static void
gedit_progress_info_bar_init (GeditProgressInfoBar *bar)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *content;
	
	bar->priv = GEDIT_PROGRESS_INFO_BAR_GET_PRIVATE (bar);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	bar->priv->image = gtk_image_new_from_icon_name (GTK_STOCK_MISSING_IMAGE, 
							 GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show (bar->priv->image);
	gtk_misc_set_alignment (GTK_MISC (bar->priv->image), 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), bar->priv->image, FALSE, FALSE, 4);
	
	bar->priv->label = gtk_label_new ("");
	gtk_widget_show (bar->priv->label);
	gtk_box_pack_start (GTK_BOX (hbox), bar->priv->label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (bar->priv->label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0, 0.5);
	gtk_label_set_ellipsize (GTK_LABEL (bar->priv->label), 
				 PANGO_ELLIPSIZE_END);

	bar->priv->progress = gtk_progress_bar_new ();
	gtk_widget_show (bar->priv->progress);
	gtk_box_pack_start (GTK_BOX (vbox), bar->priv->progress, TRUE, FALSE, 0);
	gtk_widget_set_size_request (bar->priv->progress, -1, 15);
	
	content = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));
	gtk_container_add (GTK_CONTAINER (content), vbox);
}

GtkWidget *
gedit_progress_info_bar_new (const gchar *stock_id,
			     const gchar *markup,
			     gboolean     has_cancel)
{
	GeditProgressInfoBar *bar;

	g_return_val_if_fail (stock_id != NULL, NULL);
	g_return_val_if_fail (markup != NULL, NULL);

	bar = GEDIT_PROGRESS_INFO_BAR (g_object_new (GEDIT_TYPE_PROGRESS_INFO_BAR,
						     "has-cancel-button", has_cancel,
						     NULL));

	gedit_progress_info_bar_set_stock_image (bar, stock_id);
	gedit_progress_info_bar_set_markup (bar, markup);

	return GTK_WIDGET (bar);	
}

void
gedit_progress_info_bar_set_stock_image (GeditProgressInfoBar *bar,
					 const gchar          *stock_id)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));
	g_return_if_fail (stock_id != NULL);
	
	gtk_image_set_from_stock (GTK_IMAGE (bar->priv->image),
				  stock_id,
				  GTK_ICON_SIZE_SMALL_TOOLBAR);
}

void
gedit_progress_info_bar_set_markup (GeditProgressInfoBar *bar,
				    const gchar          *markup)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));
	g_return_if_fail (markup != NULL);

	gtk_label_set_markup (GTK_LABEL (bar->priv->label),
			      markup);
}

void
gedit_progress_info_bar_set_text (GeditProgressInfoBar *bar,
				  const gchar          *text)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));
	g_return_if_fail (text != NULL);

	gtk_label_set_text (GTK_LABEL (bar->priv->label),
			    text);
}

void
gedit_progress_info_bar_set_fraction (GeditProgressInfoBar *bar,
				      gdouble               fraction)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress),
				       fraction);
}

void
gedit_progress_info_bar_pulse (GeditProgressInfoBar *bar)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));

	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar->priv->progress));
}

/* ex:set ts=8 noet: */
