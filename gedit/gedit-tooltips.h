/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GEDIT_TOOLTIPS_H__
#define __GEDIT_TOOLTIPS_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_TOOLTIPS                  (gedit_tooltips_get_type ())
#define GEDIT_TOOLTIPS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_TOOLTIPS, GeditTooltips))
#define GEDIT_TOOLTIPS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_TOOLTIPS, GeditTooltipsClass))
#define GEDIT_IS_TOOLTIPS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_TOOLTIPS))
#define GEDIT_IS_TOOLTIPS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_TOOLTIPS))
#define GEDIT_TOOLTIPS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_TOOLTIPS, GeditTooltipsClass))


typedef struct _GeditTooltips GeditTooltips;
typedef struct _GeditTooltipsClass GeditTooltipsClass;
typedef struct _GeditTooltipsData GeditTooltipsData;

struct _GeditTooltipsData {
	GeditTooltips *tooltips;
	GtkWidget *widget;
	gchar *tip_text;
	gchar *tip_private;
};

struct _GeditTooltips {
	GtkObject parent_instance;

	GtkWidget *tip_window;
	GtkWidget *tip_label;
	GeditTooltipsData *active_tips_data;
	GList *tips_data_list;

	guint delay:30;
	guint enabled:1;
	guint have_grab:1;
	guint use_sticky_delay:1;
	gint timer_tag;
	GTimeVal last_popdown;
};

struct _GeditTooltipsClass {
	GtkObjectClass parent_class;

	/* Padding for future expansion */
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
	void (*_gtk_reserved4) (void);
};

GType gedit_tooltips_get_type (void) G_GNUC_CONST;

GeditTooltips *gedit_tooltips_new (void);

void gedit_tooltips_enable (GeditTooltips * tooltips);
void gedit_tooltips_disable (GeditTooltips * tooltips);

void gedit_tooltips_set_tip (GeditTooltips * tooltips,
			     GtkWidget * widget,
			     const gchar * tip_text,
			     const gchar * tip_private);

GeditTooltipsData *gedit_tooltips_data_get (GtkWidget * widget);
void gedit_tooltips_force_window (GeditTooltips * tooltips);

void _gedit_tooltips_toggle_keyboard_mode (GtkWidget * widget);

G_END_DECLS

#endif				/* __GEDIT_TOOLTIPS_H__ */
