/*
 * gedit-print-job-preview.c
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
 
/* This is a modified version of the file gnome-print-job-preview.f file
 * of the library libgnomeprintui. Here the original copyright assignment.
 */
 
/*
 *  Copyright (C) 2000-2002 Ximian Inc.
 *
 *  Authors: Michael Zucchi <notzed@ximian.com>
 *           Miguel de Icaza (miguel@gnu.org)
 *           Lauris Kaplinski <lauris@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

/* FIXME */
#define WE_ARE_LIBGNOMEPRINT_INTERNALS
#include <libgnomeprint/private/gnome-print-private.h>

#include "gedit-print-job-preview.h"

#define GPMP_ZOOM_IN_FACTOR M_SQRT2
#define GPMP_ZOOM_OUT_FACTOR M_SQRT1_2
#define GPMP_ZOOM_MIN 0.0625
#define GPMP_ZOOM_MAX 16.0

#define GPMP_A4_WIDTH  (210.0 * 72.0 / 25.4)
#define GPMP_A4_HEIGHT (297.0 * 72.0 / 2.54)

#define GPP_COLOR_RGBA(color, ALPHA)              \
                ((guint32) (ALPHA               | \
                (((color).red   / 256) << 24)   | \
                (((color).green / 256) << 16)   | \
                (((color).blue  / 256) << 8)))

#define GEDIT_PRINT_JOB_PREVIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PRINT_JOB_PREVIEW, GeditPrintJobPreviewPrivate))

struct _GeditPrintJobPreviewPrivate
{
	/* Navigation and zoom buttons */
	GtkWidget *bpn, *bpp;
	GtkWidget *bz1, *bzf, *bzi, *bzo;
	/* Zoom factor */
	gdouble zoom;

	/* Physical area dimensions */
	gdouble paw, pah;
	/* Calculated Physical Area -> Layout */
	gdouble pa2ly[6];

	/* State */
	guint dragging : 1;
	gint anchorx, anchory;
	gint offsetx, offsety;

	/* Our GnomePrintJob */
	GnomePrintJob *job;
	/* Our GnomePrintPreview */
	GnomePrintContext *preview;

	GtkWidget *page_entry;
	GtkWidget *scrolled_window;
	GtkWidget *last;
	GnomeCanvas *canvas;
	GnomeCanvasItem *page;

	gint current_page;
	gint pagecount;

	/* Strict theme compliance [#96802] */
	gboolean theme_compliance;

	/* Number of pages displayed together */
	gulong nx, ny;
	gint update_func_id;

	GPtrArray *page_array;
};

enum {
	PROP_0,
	PROP_NX,
	PROP_NY
};

G_DEFINE_TYPE(GeditPrintJobPreview, gedit_print_job_preview, GTK_TYPE_VBOX);

static void gpmp_parse_layout			  (GeditPrintJobPreview *pmp);
static void gedit_print_job_preview_update	  (GeditPrintJobPreview *pmp);
static void gedit_print_job_preview_set_nx_and_ny (GeditPrintJobPreview *pmp,
						   gulong nx, gulong ny);

static gint
goto_page (GeditPrintJobPreview *mp, gint page)
{
	gchar c[32];

	g_return_val_if_fail (mp != NULL, GNOME_PRINT_ERROR_BADVALUE);

	g_return_val_if_fail ((mp->priv->pagecount == 0) || 
			      (page < mp->priv->pagecount),
			      GNOME_PRINT_ERROR_BADVALUE);

	g_snprintf (c, 32, "%d", page + 1);
	gtk_entry_set_text (GTK_ENTRY (mp->priv->page_entry), c);

	gtk_widget_set_sensitive (mp->priv->bpp, (page > 0) &&
					   (mp->priv->pagecount > 1));
	gtk_widget_set_sensitive (mp->priv->bpn, (page != (mp->priv->pagecount - 1)) &&
					   (mp->priv->pagecount > 1));

	if (page != mp->priv->current_page) {
		mp->priv->current_page = page;
		if (mp->priv->pagecount > 0)
			gedit_print_job_preview_update (mp);
	}

	return GNOME_PRINT_OK;
}

static gint
change_page_cmd (GtkEntry *entry, GeditPrintJobPreview *pmp)
{
	const gchar *text;
	gint page;

	text = gtk_entry_get_text (entry);

	page = CLAMP (atoi (text), 1, pmp->priv->pagecount) - 1;

	gtk_widget_grab_focus (GTK_WIDGET (pmp->priv->canvas));
	
	return goto_page (pmp, page);
}

/*
 * Padding in points around the simulated page
 */

#define PAGE_PAD 6
#define CLOSE_ENOUGH(a,b) (fabs (a - b) < 1e-6)

static void
gpmp_zoom (GeditPrintJobPreview *mp, gdouble factor)
{
	gdouble	     zoom, xdpi, ydpi;
	int          w, h, w_mm, h_mm;
	GdkScreen   *screen;

	if (factor <= 0.) {
		gint width = GTK_WIDGET (mp->priv->canvas)->allocation.width;
		gint height = GTK_WIDGET (mp->priv->canvas)->allocation.height;
		gdouble zoomx = width  / ((mp->priv->paw + PAGE_PAD * 2) * mp->priv->nx + PAGE_PAD * 2);
		gdouble zoomy = height / ((mp->priv->pah + PAGE_PAD * 2) * mp->priv->ny + PAGE_PAD * 2);

		zoom = MIN (zoomx, zoomy);
	} else
		zoom = mp->priv->zoom * factor;

	mp->priv->zoom = CLAMP (zoom, GPMP_ZOOM_MIN, GPMP_ZOOM_MAX);

	gtk_widget_set_sensitive (mp->priv->bz1, (!CLOSE_ENOUGH (mp->priv->zoom, 1.0)));
	gtk_widget_set_sensitive (mp->priv->bzi, (!CLOSE_ENOUGH (mp->priv->zoom, GPMP_ZOOM_MAX)));
	gtk_widget_set_sensitive (mp->priv->bzo, (!CLOSE_ENOUGH (mp->priv->zoom, GPMP_ZOOM_MIN)));

	screen = gtk_widget_get_screen (GTK_WIDGET (mp->priv->canvas)),
#if 0
       /* there is no per monitor physical size info available for now */
	n = gdk_screen_get_n_monitors (screen);
	for (i = 0; i < n; i++) {
		gdk_screen_get_monitor_geometry (screen, i, &monitor);
		if (x >= monitor.x && x <= monitor.x + monitor.width &&
		    y >= monitor.y && y <= monitor.y + monitor.height) {
			break;
		}
	}
#endif
	/*
	 * From xdpyinfo.c:
	 *
	 * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
	 *
	 *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
	 *         = N pixels / (M inch / 25.4)
	 *         = N * 25.4 pixels / M inch
	 */
	w_mm = gdk_screen_get_width_mm (screen);
	xdpi = -1.;
	if (w_mm > 0) {
		w    = gdk_screen_get_width (screen);
		xdpi = (w * 25.4) / (gdouble) w_mm;
	} 
	if (xdpi < 30. || 600. < xdpi) {
		g_warning ("Invalid the x-resolution for the screen, assuming 96dpi");
		xdpi = 96.;
	}

	h_mm = gdk_screen_get_height_mm (screen);
	ydpi = -1.;
	if (h_mm > 0) {
		h    = gdk_screen_get_height (screen);
		ydpi = (h * 25.4) / (gdouble) h_mm;
	}
	if (ydpi < 30. || 600. < ydpi) {
		g_warning ("Invalid the y-resolution for the screen, assuming 96dpi");
		ydpi = 96.;
	}

	gnome_canvas_set_pixels_per_unit (mp->priv->canvas, mp->priv->zoom * (xdpi + ydpi) / (72. * 2.));
}

/* Button press handler for the print preview canvas */

static gint
preview_canvas_button_press (GtkWidget *widget, GdkEventButton *event, GeditPrintJobPreview *mp)
{
	gint retval;

	retval = FALSE;

	if (event->button == 1) {
		GdkCursor *cursor;
		GdkDisplay *display = gtk_widget_get_display (widget);

		mp->priv->dragging = TRUE;

		gnome_canvas_get_scroll_offsets (GNOME_CANVAS (widget), &mp->priv->offsetx, &mp->priv->offsety);

		mp->priv->anchorx = event->x - mp->priv->offsetx;
		mp->priv->anchory = event->y - mp->priv->offsety;

		cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
		gdk_pointer_grab (widget->window, FALSE,
				  (GDK_POINTER_MOTION_MASK | 
				   GDK_POINTER_MOTION_HINT_MASK | 
				   GDK_BUTTON_RELEASE_MASK),
				  NULL, cursor, event->time);
		gdk_cursor_unref (cursor);

		retval = TRUE;
	}

	return retval;
}

/* Motion notify handler for the print preview canvas */

static gint
preview_canvas_motion (GtkWidget *widget, GdkEventMotion *event, GeditPrintJobPreview *mp)
{
	GdkModifierType mod;
	gint retval;

	retval = FALSE;

	if (mp->priv->dragging) {
		gint x, y, dx, dy;

		if (event->is_hint) {
			gdk_window_get_pointer (widget->window, &x, &y, &mod);
		} else {
			x = event->x;
			y = event->y;
		}

		dx = mp->priv->anchorx - x;
		dy = mp->priv->anchory - y;

		gnome_canvas_scroll_to ( mp->priv->canvas, mp->priv->offsetx + dx, mp->priv->offsety + dy);

		/* Get new anchor and offset */
		mp->priv->anchorx = event->x;
		mp->priv->anchory = event->y;
		gnome_canvas_get_scroll_offsets (GNOME_CANVAS (widget), &mp->priv->offsetx, &mp->priv->offsety);

		retval = TRUE;
	}

	return retval;
}

/* Button release handler for the print preview canvas */

static gint
preview_canvas_button_release (GtkWidget *widget, GdkEventButton *event, GeditPrintJobPreview *mp)
{
	gint retval;

	retval = TRUE;

	if (event->button == 1) {
		mp->priv->dragging = FALSE;
		gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
					    event->time);
		retval = TRUE;
	}

	return retval;
}

static void
preview_close_cmd (gpointer unused, GeditPrintJobPreview *mp)
{
	gtk_widget_destroy (GTK_WIDGET (mp));
}

static void
preview_next_page_cmd (void *unused, GeditPrintJobPreview *pmp)
{
	GdkEvent *event;

	event = gtk_get_current_event ();
	
	if (event->button.state & GDK_SHIFT_MASK)
		goto_page (pmp, pmp->priv->pagecount - 1);
	else
		goto_page (pmp, MIN (pmp->priv->current_page + pmp->priv->nx * pmp->priv->ny,
			     	     pmp->priv->pagecount - 1));
		   
	gdk_event_free (event);
}

static void
preview_prev_page_cmd (void *unused, GeditPrintJobPreview *pmp)
{
	GdkEvent *event;

	event = gtk_get_current_event ();
	
	if (event->button.state & GDK_SHIFT_MASK)
		goto_page (pmp, 0);
	else
		goto_page (pmp,
		   MAX ((gint) (pmp->priv->current_page - pmp->priv->nx * pmp->priv->ny), 0));
		   
	gdk_event_free (event);
}

static void
gpmp_zoom_in_cmd (GtkToggleButton *t, GeditPrintJobPreview *pmp)
{
	gpmp_zoom (pmp, GPMP_ZOOM_IN_FACTOR);
}

static void
gpmp_zoom_out_cmd (GtkToggleButton *t, GeditPrintJobPreview *pmp)
{
	gpmp_zoom (pmp, GPMP_ZOOM_OUT_FACTOR);
}

static void
preview_zoom_fit_cmd (GeditPrintJobPreview *mp)
{
	gpmp_zoom (mp, -1.);
}

static void
preview_zoom_100_cmd (GeditPrintJobPreview *mp)
{
	gpmp_zoom (mp,  1. / mp->priv->zoom); /* cheesy way to force 100% */
}

static gint
preview_canvas_key (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GeditPrintJobPreview *pmp;
	gint x,y;
	gint height, width;
	gint domove = 0;

	pmp = (GeditPrintJobPreview *) data;

	gnome_canvas_get_scroll_offsets (pmp->priv->canvas, &x, &y);
	height = GTK_WIDGET (pmp->priv->canvas)->allocation.height;
	width = GTK_WIDGET (pmp->priv->canvas)->allocation.width;

	switch (event->keyval) {
	case '1':
		preview_zoom_100_cmd (pmp);
		break;
	case '+':
	case '=':
	case GDK_KP_Add:
		gpmp_zoom_in_cmd (NULL, pmp);
		break;
	case '-':
	case '_':
	case GDK_KP_Subtract:
		gpmp_zoom_out_cmd (NULL, pmp);
		break;
	case GDK_KP_Right:
	case GDK_Right:
		if (event->state & GDK_SHIFT_MASK)
			x += width;
		else
			x += 10;
		domove = 1;
		break;
	case GDK_KP_Left:
	case GDK_Left:
		if (event->state & GDK_SHIFT_MASK)
			x -= width;
		else
			x -= 10;
		domove = 1;
		break;
	case GDK_KP_Up:
	case GDK_Up:
		if (event->state & GDK_SHIFT_MASK)
			goto page_up;
		y -= 10;
		domove = 1;
		break;
	case GDK_KP_Down:
	case GDK_Down:
		if (event->state & GDK_SHIFT_MASK)
			goto page_down;
		y += 10;
		domove = 1;
		break;
	case GDK_KP_Page_Up:
	case GDK_Page_Up:
	case GDK_Delete:
	case GDK_KP_Delete:
	case GDK_BackSpace:
	page_up:
		if (y <= 0) {
			if (pmp->priv->current_page > 0) {
				goto_page (pmp, pmp->priv->current_page - 1);
				y = GTK_LAYOUT (pmp->priv->canvas)->height - height;
			}
		} else {
			y -= height;
		}
		domove = 1;
		break;
	case GDK_KP_Page_Down:
	case GDK_Page_Down:
	case ' ':
	page_down:
		if (y >= GTK_LAYOUT (pmp->priv->canvas)->height - height) {
			if (pmp->priv->current_page < pmp->priv->pagecount - 1) {
				goto_page (pmp, pmp->priv->current_page + 1);
				y = 0;
			}
		} else {
			y += height;
		}
		domove = 1;
		break;
	case GDK_KP_Home:
	case GDK_Home:
		goto_page (pmp, 0);
		y = 0;
		domove = 1;
		break;
	case GDK_KP_End:
	case GDK_End:
		goto_page (pmp, pmp->priv->pagecount - 1);
		y = 0;
		domove = 1;
		break;
	case GDK_Escape:
		gtk_widget_destroy (GTK_WIDGET (pmp));
		return TRUE;
	        /* break skipped */
	case 'c':
		if (event->state & GDK_MOD1_MASK)
		{
			gtk_widget_destroy (GTK_WIDGET (pmp));
			return TRUE;
		}
		break;
	case 'p':
		if (event->state & GDK_MOD1_MASK)
		{
			gtk_widget_grab_focus (GTK_WIDGET (pmp->priv->page_entry));
			return TRUE;
		}
		break;		
	default:
		return FALSE;
	}

	if (domove)
		gnome_canvas_scroll_to (pmp->priv->canvas, x, y);

	return FALSE;
}

static void
canvas_style_changed_cb (GtkWidget *canvas, GtkStyle *ps, GeditPrintJobPreview *mp)
{
	GtkStyle *style;
	gint32 border_color;
	gint32 page_color;
	guint i;
	GnomeCanvasItem *page;

	style = gtk_widget_get_style (GTK_WIDGET (canvas));
	if (mp->priv->theme_compliance) {
		page_color = GPP_COLOR_RGBA (style->base [GTK_STATE_NORMAL], 
					     0xff);
		border_color = GPP_COLOR_RGBA (style->text [GTK_STATE_NORMAL], 
					       0xff);
	}
	else {
		page_color = GPP_COLOR_RGBA (style->white, 0xff);
		border_color = GPP_COLOR_RGBA (style->black, 0xff);
	}

	for (i = 0; i < mp->priv->page_array->len; i++) {
		page = g_object_get_data (mp->priv->page_array->pdata[i], "page");
		gnome_canvas_item_set (page,
			       "fill_color_rgba", page_color,
			       "outline_color_rgba", border_color,
			       NULL);
	}

	gedit_print_job_preview_update (mp);
}

static void
entry_insert_text_cb (GtkEditable *editable, const gchar *text, gint length, gint *position)
{
	gunichar c;
	const gchar *p;
 	const gchar *end;

	p = text;
	end = text + length;

	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);

		c = g_utf8_get_char (p);

		if (!g_unichar_isdigit (c)) {
			g_signal_stop_emission_by_name (editable, "insert_text");
			break;
		}

		p = next;
	}
}

static gboolean 
entry_focus_out_event_cb (GtkWidget *widget, GdkEventFocus *event, GeditPrintJobPreview *pjp)
{
	const gchar *text;
	gint page;

	text = gtk_entry_get_text (GTK_ENTRY(widget));
	page = atoi (text) - 1;
	
	/* Reset the page number only if really needed */
	if (page != pjp->priv->current_page) {
		gchar *str;

		str = g_strdup_printf ("%d", pjp->priv->current_page + 1);
		gtk_entry_set_text (GTK_ENTRY (widget), str);
		g_free (str);
	}

	return FALSE;
}

static void
create_preview_canvas (GeditPrintJobPreview *mp)
{
	AtkObject *atko;

	mp->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (mp->priv->scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_widget_push_colormap (gdk_screen_get_rgb_colormap (
  	                     		gtk_widget_get_screen (mp->priv->scrolled_window)));
	mp->priv->canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
	gnome_canvas_set_center_scroll_region (mp->priv->canvas, FALSE);
	gtk_widget_pop_colormap ();
	atko = gtk_widget_get_accessible (GTK_WIDGET (mp->priv->canvas));
	atk_object_set_name (atko, _("Page Preview"));
	atk_object_set_description (atko, _("The preview of a page in the document to be printed"));

	g_signal_connect (G_OBJECT (mp->priv->canvas), "button_press_event",
			  (GCallback) preview_canvas_button_press, mp);
	g_signal_connect (G_OBJECT (mp->priv->canvas), "button_release_event",
			  (GCallback) preview_canvas_button_release, mp);
	g_signal_connect (G_OBJECT (mp->priv->canvas), "motion_notify_event",
			  (GCallback) preview_canvas_motion, mp);
	g_signal_connect (G_OBJECT (mp->priv->canvas), "key_press_event",
			  (GCallback) preview_canvas_key, mp);

	gtk_container_add (GTK_CONTAINER (mp->priv->scrolled_window), GTK_WIDGET (mp->priv->canvas));

	g_signal_connect (G_OBJECT (mp->priv->canvas), "style_set",
			  G_CALLBACK (canvas_style_changed_cb), mp);
	
	gtk_box_pack_end (GTK_BOX (mp), mp->priv->scrolled_window, TRUE, TRUE, 0);

	gtk_widget_show_all (GTK_WIDGET (mp->priv->scrolled_window));
	gtk_widget_grab_focus (GTK_WIDGET (mp->priv->canvas));
}

static void
on_1x1_clicked (GtkMenuItem *i, GeditPrintJobPreview *mp)
{
	gedit_print_job_preview_set_nx_and_ny (mp, 1, 1);
}

static void
on_1x2_clicked (GtkMenuItem *i, GeditPrintJobPreview *mp)
{
	gedit_print_job_preview_set_nx_and_ny (mp, 2, 1);
}

static void
on_2x1_clicked (GtkMenuItem *i, GeditPrintJobPreview *mp)
{
	gedit_print_job_preview_set_nx_and_ny (mp, 1, 2);
}

static void
on_2x2_clicked (GtkMenuItem *i, GeditPrintJobPreview *mp)
{
	gedit_print_job_preview_set_nx_and_ny (mp, 2, 2);
}


#if 0
static void
on_other_clicked (GtkMenuItem *i, GeditPrintJobPreview *mp)
{
	
}
#endif

static void
gpmp_multi_cmd (GtkWidget *w, GeditPrintJobPreview *mp)
{
	GtkWidget *m, *i;

	m = gtk_menu_new ();
	gtk_widget_show (m);
	g_signal_connect (m, "selection_done", G_CALLBACK (gtk_widget_destroy),
			  m);

        i = gtk_menu_item_new_with_label ("1x1");
        gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 0, 1, 0, 1);
        g_signal_connect (i, "activate", G_CALLBACK (on_1x1_clicked), mp);

        i = gtk_menu_item_new_with_label ("2x1");
        gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 0, 1, 1, 2);
        g_signal_connect (i, "activate", G_CALLBACK (on_2x1_clicked), mp);

        i = gtk_menu_item_new_with_label ("1x2");
        gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 1, 2, 0, 1);
        g_signal_connect (i, "activate", G_CALLBACK (on_1x2_clicked), mp);

        i = gtk_menu_item_new_with_label ("2x2");
        gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 1, 2, 1, 2);
        g_signal_connect (i, "activate", G_CALLBACK (on_2x2_clicked), mp);
        
#if 0
        i = gtk_menu_item_new_with_label (_("Other"));
        gtk_widget_show (i);
	gtk_menu_attach (GTK_MENU (m), i, 0, 2, 2, 3);
        gtk_widget_set_sensitive (i, FALSE);
        g_signal_connect (i, "activate", G_CALLBACK (on_other_clicked), mp);
#endif

	gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, mp, 0,
			GDK_CURRENT_TIME);
}

static void
create_toplevel (GeditPrintJobPreview *mp)
{
	GtkWidget *tb;
	GtkToolItem *i;
	AtkObject *atko;
	GtkWidget *status;
	
	tb = gtk_toolbar_new ();
	gtk_toolbar_set_style (GTK_TOOLBAR (tb), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_widget_show (tb);
	gtk_box_pack_start (GTK_BOX (mp), tb, FALSE, FALSE, 0);

	i = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (i), "P_revious Page");
	mp->priv->bpp = GTK_WIDGET(i);
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (i), TRUE);

	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Show the previous page"), "");
	g_signal_connect (i, "clicked",
			  G_CALLBACK (preview_prev_page_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);

	i = gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD);
	mp->priv->bpn = GTK_WIDGET (i);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (i), "_Next Page");
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (i), TRUE);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Show the next page"), "");
	g_signal_connect (i, "clicked",
			  G_CALLBACK (preview_next_page_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);

	i = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);

	status = gtk_hbox_new (FALSE, 4);
	mp->priv->page_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (mp->priv->page_entry), 3);
	gtk_entry_set_max_length (GTK_ENTRY (mp->priv->page_entry), 6);
	gtk_tooltips_set_tip (GTK_TOOLBAR (tb)->tooltips,
	                      mp->priv->page_entry,
	                      _("Current page (Alt+P)"),
	                      NULL);
	                      
	g_signal_connect (G_OBJECT (mp->priv->page_entry), "activate", 
			  G_CALLBACK (change_page_cmd), mp);
	g_signal_connect (G_OBJECT (mp->priv->page_entry), "insert_text",
			  G_CALLBACK (entry_insert_text_cb), NULL);
	g_signal_connect (G_OBJECT (mp->priv->page_entry), "focus_out_event",
			  G_CALLBACK (entry_focus_out_event_cb), mp);

	gtk_box_pack_start (GTK_BOX (status), mp->priv->page_entry, FALSE, FALSE, 0);
	/* gtk_label_set_mnemonic_widget ((GtkLabel *) l, mp->priv->page_entry); */

	/* We are displaying 'XXX of XXX'. */
	gtk_box_pack_start (GTK_BOX (status), gtk_label_new (_("of")),
			    FALSE, FALSE, 0);

	mp->priv->last = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (status), mp->priv->last, FALSE, FALSE, 0);
	atko = gtk_widget_get_accessible (mp->priv->last);
	atk_object_set_name (atko, _("Page total"));
	atk_object_set_description (atko, _("The total number of pages in the document"));

	gtk_widget_show_all (status);

	i = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (i), status);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);

	i = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);
	
	i = gtk_tool_button_new_from_stock (GTK_STOCK_DND_MULTIPLE);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (i), "_Show Multiple Pages");
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (i), TRUE);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Show multiple pages"), "");
	g_signal_connect (i, "clicked", G_CALLBACK (gpmp_multi_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);	
		
	i = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);
	
	i = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_100);
	mp->priv->bz1 = GTK_WIDGET (i);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Zoom 1:1"), "");
	g_signal_connect_swapped (i, "clicked",
		G_CALLBACK (preview_zoom_100_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);
	i = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
	mp->priv->bzf = GTK_WIDGET (i);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Zoom to fit the whole page"), "");
	g_signal_connect_swapped (i, "clicked",
		G_CALLBACK (preview_zoom_fit_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);
	
	i = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_IN);
	mp->priv->bzi = GTK_WIDGET (i);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Zoom the page in"), "");
	g_signal_connect (i, "clicked",
			  G_CALLBACK (gpmp_zoom_in_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);
	i = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_OUT);
	mp->priv->bzo = GTK_WIDGET (i);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips,
				   _("Zoom the page out"), "");
	g_signal_connect (i, "clicked",
			  G_CALLBACK (gpmp_zoom_out_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);

	i = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);

	i = gtk_tool_button_new (NULL, _("_Close Preview"));
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (i), TRUE);
	gtk_tool_item_set_is_important (i, TRUE);
	gtk_tool_item_set_tooltip (i, GTK_TOOLBAR (tb)->tooltips, 
				   _("Close print preview"), "");
	g_signal_connect (i, "clicked",
			  G_CALLBACK (preview_close_cmd), mp);
	gtk_widget_show (GTK_WIDGET (i));
	gtk_toolbar_insert (GTK_TOOLBAR (tb), i, -1);
}

static void
gedit_print_job_preview_destroy (GtkObject *object)
{
	GeditPrintJobPreview *pmp = GEDIT_PRINT_JOB_PREVIEW (object);

	if (pmp->priv->page_array != NULL) {
		g_ptr_array_free (pmp->priv->page_array, TRUE);
		pmp->priv->page_array = NULL;
	}

	if (pmp->priv->job != NULL) {
		g_object_unref (G_OBJECT (pmp->priv->job));
		pmp->priv->job = NULL;
	}

	if (pmp->priv->update_func_id) {
		g_source_remove (pmp->priv->update_func_id);
		pmp->priv->update_func_id = 0;
	}

	if (GTK_OBJECT_CLASS (gedit_print_job_preview_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (gedit_print_job_preview_parent_class)->destroy) (object);
}

static void
gedit_print_job_preview_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (gedit_print_job_preview_parent_class)->finalize)
		(* G_OBJECT_CLASS (gedit_print_job_preview_parent_class)->finalize) (object);
}

static gboolean
update_func (gpointer data)
{
	GeditPrintJobPreview *pmp = GEDIT_PRINT_JOB_PREVIEW (data);

	gedit_print_job_preview_update (pmp);
	pmp->priv->update_func_id = 0;
	
	return FALSE;
}

static void
gedit_print_job_preview_set_nx_and_ny (GeditPrintJobPreview *pmp,
				       gulong nx, gulong ny)
{
	GPtrArray *a;
	GnomeCanvasItem *group, *item;
	guint i, col, row;

	g_return_if_fail (GEDIT_IS_PRINT_JOB_PREVIEW (pmp));
	g_return_if_fail (nx > 0);	
	g_return_if_fail (ny > 0);	
	
	pmp->priv->nx = nx;
	pmp->priv->ny = ny; 
		
	/* Remove unnecessary pages */
	a = pmp->priv->page_array;
	while (pmp->priv->page_array->len > MIN (nx * ny, pmp->priv->pagecount)) {
		gtk_object_destroy (GTK_OBJECT (a->pdata[a->len - 1]));
		g_ptr_array_remove_index (a, a->len - 1);
	}

	/* Create pages if needed */
	if (pmp->priv->page_array->len < MIN (nx * ny, pmp->priv->pagecount)) {
		gdouble transform[6];

		transform[0] =  1.; transform[1] =  0.;
		transform[2] =  0.; transform[3] = -1.;
		transform[4] =  0.; transform[5] =  pmp->priv->pah;
		art_affine_multiply (transform, pmp->priv->pa2ly, transform);

		while (pmp->priv->page_array->len < MIN (nx * ny, pmp->priv->pagecount)) {
			GnomePrintPreview *preview;

			group = gnome_canvas_item_new (
				gnome_canvas_root (pmp->priv->canvas),
				GNOME_TYPE_CANVAS_GROUP, "x", 0., "y", 0., NULL);
			g_ptr_array_add (a, group);
			item = gnome_canvas_item_new (GNOME_CANVAS_GROUP (group),
				GNOME_TYPE_CANVAS_RECT, "x1", 0.0, "y1", 0.0,
				"x2", (gdouble) pmp->priv->paw, "y2", (gdouble) pmp->priv->pah,
				"fill_color", "white", "outline_color", "black",
				"width_pixels", 1, NULL);
			gnome_canvas_item_lower_to_bottom (item);
			g_object_set_data (G_OBJECT (group), "page", item);
			item = gnome_canvas_item_new (GNOME_CANVAS_GROUP (group),
				GNOME_TYPE_CANVAS_RECT, "x1", 3.0, "y1", 3.0,
				"x2", (gdouble) pmp->priv->paw + 3,
				"y2", (gdouble) pmp->priv->pah + 3,
				"fill_color", "black", NULL);
			gnome_canvas_item_lower_to_bottom (item);

			/* Create the group that holds the preview. */
			item = gnome_canvas_item_new (
				GNOME_CANVAS_GROUP (group),
				GNOME_TYPE_CANVAS_GROUP,
				"x", 0., "y", 0., NULL);
			gnome_canvas_item_affine_absolute (item, transform); 
			preview = g_object_new (GNOME_TYPE_PRINT_PREVIEW,
				"group", item,
				"theme_compliance", pmp->priv->theme_compliance,
				NULL);
			/* CHECK: changing this I have fixed a crash, but I'm not sure
			 * of the fix since I may have included a mem leak */
			/*
			g_object_set_data_full (G_OBJECT (group), "preview", preview,
				(GDestroyNotify) g_object_unref);
			*/
			g_object_set_data (G_OBJECT (group), "preview", preview);
		}
	}

	/* Position the pages */
	for (i = 0; i < a->len; i++) {
		col = i % pmp->priv->nx;
		row = i / pmp->priv->nx;
		g_object_set (a->pdata[i],
			"x", (gdouble) col * (pmp->priv->paw + PAGE_PAD * 2),
			"y", (gdouble) row * (pmp->priv->pah + PAGE_PAD * 2), NULL);
	}

	preview_zoom_fit_cmd (pmp);

	if (!pmp->priv->update_func_id)
		pmp->priv->update_func_id = g_idle_add_full (
			G_PRIORITY_DEFAULT_IDLE, update_func, pmp, NULL);
}

static void
gedit_print_job_preview_update (GeditPrintJobPreview *pmp)
{
	guint col, row, i, page;
	GPtrArray *a;

	g_return_if_fail (GEDIT_IS_PRINT_JOB_PREVIEW (pmp));

	a = pmp->priv->page_array;

	/* Show something on the pages */
	for (i = 0; i < a->len; i++) {
		GnomeCanvasItem *group = a->pdata[i];
		GnomePrintPreview *preview = g_object_get_data (
						G_OBJECT (group), "preview");

		col = i % pmp->priv->nx;
		row = i / pmp->priv->nx;
		page = (guint) (pmp->priv->current_page / (pmp->priv->nx * pmp->priv->ny))
				* (pmp->priv->nx * pmp->priv->ny) + i;

		/* Do we need to show this page? */
		if (page < pmp->priv->pagecount)
			gnome_canvas_item_show (group);
		else {
			gnome_canvas_item_hide (group);
			continue;
		}
		gnome_print_job_render_page (pmp->priv->job,
				GNOME_PRINT_CONTEXT (preview), page, TRUE);
	}

	gnome_canvas_set_scroll_region (pmp->priv->canvas,
		0 - PAGE_PAD, 0 - PAGE_PAD,
		(pmp->priv->paw + PAGE_PAD * 2) * pmp->priv->nx + PAGE_PAD,
		(pmp->priv->pah + PAGE_PAD * 2) * pmp->priv->ny + PAGE_PAD);
}

static void
gedit_print_job_preview_get_property (GObject *object, guint n, GValue *v, GParamSpec *pspec)
{
	GeditPrintJobPreview *pmp = GEDIT_PRINT_JOB_PREVIEW (object);

	switch (n) {
	case PROP_NX:
		g_value_set_ulong (v, pmp->priv->nx);
		break;
	case PROP_NY:
		g_value_set_ulong (v, pmp->priv->ny);
		break;
	default:
		break;
	}
}

static void
gedit_print_job_preview_set_property (GObject *object, guint n,
				      const GValue *v, GParamSpec *pspec)
{
	GeditPrintJobPreview *pmp = GEDIT_PRINT_JOB_PREVIEW (object);

	switch (n) {
	case PROP_NX:
		gedit_print_job_preview_set_nx_and_ny (pmp,
				g_value_get_ulong (v), pmp->priv->ny);
		break;
	case PROP_NY:
		gedit_print_job_preview_set_nx_and_ny (pmp,
				pmp->priv->nx, g_value_get_ulong (v));
		break;
	default:
		break;
	}
}

static void
grab_focus (GtkWidget *w)
{
	GeditPrintJobPreview *pmp;
	
	pmp = GEDIT_PRINT_JOB_PREVIEW (w);
	
	gtk_widget_grab_focus (GTK_WIDGET (pmp->priv->canvas));
}

static void
gedit_print_job_preview_class_init (GeditPrintJobPreviewClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	object_class = (GtkObjectClass *) klass;

	object_class->destroy = gedit_print_job_preview_destroy;
	
	widget_class->grab_focus = grab_focus;
	
	gobject_class->finalize     = gedit_print_job_preview_finalize;
	gobject_class->set_property = gedit_print_job_preview_set_property;
	gobject_class->get_property = gedit_print_job_preview_get_property;

	g_object_class_install_property (gobject_class, PROP_NX,
		g_param_spec_ulong ("nx", _("Number of pages horizontally"),
			_("Number of pages horizontally"), 1, 0xffff, 1,
			G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, PROP_NY,
		g_param_spec_ulong ("ny", _("Number of pages vertically"),
			_("Number of pages vertically"), 1, 0xffff, 1,
			G_PARAM_READWRITE));

	g_type_class_add_private(klass, sizeof(GeditPrintJobPreviewPrivate));
}

static void
gedit_print_job_preview_init (GeditPrintJobPreview *mp)
{
	const gchar *env_theme_variable;
	
	mp->priv = GEDIT_PRINT_JOB_PREVIEW_GET_PRIVATE(mp);
	mp->priv->current_page = -1;

	mp->priv->theme_compliance = FALSE;
	env_theme_variable = g_getenv("GP_PREVIEW_STRICT_THEME");
	if (env_theme_variable && env_theme_variable [0])
		mp->priv->theme_compliance = TRUE;

	mp->priv->zoom = 1.0;

	mp->priv->nx = mp->priv->ny = 1;
	mp->priv->page_array = g_ptr_array_new ();
}

static void
realized (GtkWidget *widget, GeditPrintJobPreview *gpmp)
{
	preview_zoom_100_cmd (gpmp);
}

GtkWidget *
gedit_print_job_preview_new (GnomePrintJob *gpm)
{
	GeditPrintJobPreview *gpmp;
	gchar *text;

	g_return_val_if_fail (gpm != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (gpm), NULL);

	gpmp = g_object_new (GEDIT_TYPE_PRINT_JOB_PREVIEW, NULL);

	gpmp->priv->job = gpm;
	g_object_ref (G_OBJECT (gpm));

	gpmp_parse_layout (gpmp);
	create_toplevel (gpmp);
	create_preview_canvas (gpmp);

	/* this zooms to fit, once we know how big the window actually is */
	g_signal_connect_after (gpmp->priv->canvas, "realize",
		(GCallback) realized, gpmp);

	gpmp->priv->pagecount = gnome_print_job_get_pages (gpm);
	goto_page (gpmp, 0);
	
	if (gpmp->priv->pagecount == 0) {
		gpmp->priv->pagecount = 1;
		text = g_strdup_printf ("<markup>%d   "
					"<span foreground=\"red\" "
					"weight=\"ultrabold\" "
					"background=\"white\">"
					"%s</span></markup>",
					1, 
					_("No visible output was created."));
	} else 
		text =  g_strdup_printf ("%d", gpmp->priv->pagecount);
	gtk_label_set_markup_with_mnemonic (GTK_LABEL (gpmp->priv->last), text);
	g_free (text);

	on_1x1_clicked (NULL, gpmp);
	return (GtkWidget *) gpmp;
}

#ifdef GPMP_VERBOSE
#define PRINT_2(s,a,b) g_print ("GPMP %s %g %g\n", s, (a), (b))
#define PRINT_DRECT(s,a) g_print ("GPMP %s %g %g %g %g\n", (s), (a)->x0, (a)->y0, (a)->x1, (a)->y1)
#define PRINT_AFFINE(s,a) g_print ("GPMP %s %g %g %g %g %g %g\n", (s), *(a), *((a) + 1), *((a) + 2), *((a) + 3), *((a) + 4), *((a) + 5))
#else
#define PRINT_2(s,a,b)
#define PRINT_DRECT(s,a)
#define PRINT_AFFINE(s,a)
#endif

static void
gpmp_parse_layout (GeditPrintJobPreview *mp)
{
	GnomePrintConfig *config;
	GnomePrintLayoutData *lyd;

	/* Calculate layout-compensated page dimensions */
	mp->priv->paw = GPMP_A4_WIDTH;
	mp->priv->pah = GPMP_A4_HEIGHT;
	art_affine_identity (mp->priv->pa2ly);
	config = gnome_print_job_get_config (mp->priv->job);
	lyd = gnome_print_config_get_layout_data (config, NULL, NULL, NULL, NULL);
	gnome_print_config_unref (config);
	if (lyd) {
		GnomePrintLayout *ly;
		ly = gnome_print_layout_new_from_data (lyd);
		if (ly) {
			gdouble pp2lyI[6], pa2pp[6];
			gdouble expansion;
			ArtDRect pp, ap, tp;
			/* Find paper -> layout transformation */
			art_affine_invert (pp2lyI, ly->LYP[0].matrix);
			PRINT_AFFINE ("pp2ly:", &pp2lyI[0]);
			/* Find out, what the page dimensions should be */
			expansion = art_affine_expansion (pp2lyI);
			if (expansion > 1e-6) {
				/* Normalize */
				pp2lyI[0] /= expansion;
				pp2lyI[1] /= expansion;
				pp2lyI[2] /= expansion;
				pp2lyI[3] /= expansion;
				pp2lyI[4] = 0.0;
				pp2lyI[5] = 0.0;
				PRINT_AFFINE ("pp2lyI:", &pp2lyI[0]);
				/* Find page dimensions relative to layout */
				pp.x0 = 0.0;
				pp.y0 = 0.0;
				pp.x1 = lyd->pw;
				pp.y1 = lyd->ph;
				art_drect_affine_transform (&tp, &pp, pp2lyI);
				/* Compensate with expansion */
				mp->priv->paw = tp.x1 - tp.x0;
				mp->priv->pah = tp.y1 - tp.y0;
				PRINT_2 ("Width & Height", mp->priv->paw, mp->priv->pah);
			}
			/* Now compensate with feed orientation */
			art_affine_invert (pa2pp, ly->PP2PA);
			PRINT_AFFINE ("pa2pp:", &pa2pp[0]);
			art_affine_multiply (mp->priv->pa2ly, pa2pp, pp2lyI);
			PRINT_AFFINE ("pa2ly:", &mp->priv->pa2ly[0]);
			/* Finally we need translation factors */
			/* Page box in normalized layout */
			pp.x0 = 0.0;
			pp.y0 = 0.0;
			pp.x1 = lyd->pw;
			pp.y1 = lyd->ph;
			art_drect_affine_transform (&ap, &pp, ly->PP2PA);
			art_drect_affine_transform (&tp, &ap, mp->priv->pa2ly);
			PRINT_DRECT ("RRR:", &tp);
			mp->priv->pa2ly[4] -= tp.x0;
			mp->priv->pa2ly[5] -= tp.y0;
			PRINT_AFFINE ("pa2ly:", &mp->priv->pa2ly[0]);
			/* Now, if job does PA2LY LY2PA concat it ends with scaled identity */
			gnome_print_layout_free (ly);
		}
		gnome_print_layout_data_free (lyd);
	}
}
