/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
 *
 * Toolbar code by Andy Kahn
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "commands.h"
#include "menus.h"
#include "gE_print.h"
#include "toolbar.h"

#ifdef WITHOUT_GNOME
#include "xpm/tb_new.xpm"
#include "xpm/tb_open.xpm"
#include "xpm/tb_save.xpm"
#include "xpm/tb_cancel.xpm"
#include "xpm/tb_print.xpm"
#include "xpm/tb_cut.xpm"
#include "xpm/tb_copy.xpm"
#include "xpm/tb_paste.xpm"
#include "xpm/tb_search.xpm"
#endif


#ifdef WITHOUT_GNOME

static toolbar_data_t toolbar_data[] = {
	{ " New ", "Start a new file", "Toolbar/New", tb_new_xpm,
		(GtkSignalFunc)file_new_cmd_callback },
	{ " Open ", "Open a file", "Toolbar/Open", tb_open_xpm,
		(GtkSignalFunc)file_open_cmd_callback },
	{ " Save ", "Save file", "Toolbar/Save", tb_save_xpm,
		(GtkSignalFunc)file_save_cmd_callback },
	{ " Close ", "Close the current file", "Toolbar/Close", cancel_xpm,
		(GtkSignalFunc)file_close_cmd_callback },
	{ " Print ", "Print file", "Toolbar/Print", tb_print_xpm,
		(GtkSignalFunc)file_print_cmd_callback },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ " Cut ", "Cut text", "Toolbar/Cut", tb_cut_xpm,
		(GtkSignalFunc)edit_cut_cmd_callback },
	{ " Copy ", "Copy text", "Toolbar/Copy", tb_copy_xpm,
		(GtkSignalFunc)edit_copy_cmd_callback },
	{ " Paste ", "Paste text", "Toolbar/Paste", tb_paste_xpm,
		(GtkSignalFunc)edit_paste_cmd_callback },
	{ " Search ", "Search for text", "Toolbar/Search", tb_search_xpm,
		(GtkSignalFunc)search_search_cmd_callback },
	{ NULL, NULL, NULL, NULL, NULL }
};

#else

static toolbar_data_t toolbar_data[] = {
	{ N_(" New "), N_("Start a new file"), "Toolbar/New", GNOME_STOCK_PIXMAP_NEW,
		(GtkSignalFunc)file_new_cmd_callback },
	{ N_(" Open "), N_("Open a file"), "Toolbar/Open", GNOME_STOCK_PIXMAP_OPEN,
		(GtkSignalFunc)file_open_cmd_callback },
	{ N_(" Save "), N_("Save file"), "Toolbar/Save", GNOME_STOCK_PIXMAP_SAVE,
		(GtkSignalFunc)file_save_cmd_callback },
	{ N_(" Close "), N_("Close the current file"), "Toolbar/Close", GNOME_STOCK_PIXMAP_CLOSE,
		(GtkSignalFunc)file_close_cmd_callback },
	{ N_(" Print "), N_("Print file"), "Toolbar/Print", GNOME_STOCK_PIXMAP_PRINT,
		(GtkSignalFunc)file_print_cmd_callback },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ N_(" Cut "), N_("Cut text"), "Toolbar/Cut", GNOME_STOCK_PIXMAP_CUT,
		(GtkSignalFunc)edit_cut_cmd_callback },
	{ N_(" Copy "), N_("Copy text"), "Toolbar/Copy", GNOME_STOCK_PIXMAP_COPY,
		(GtkSignalFunc)edit_copy_cmd_callback },
	{ N_(" Paste "), N_("Paste text"), "Toolbar/Paste", GNOME_STOCK_PIXMAP_PASTE,
		(GtkSignalFunc)edit_paste_cmd_callback },
	{ N_(" Search "), N_("Search for text"), "Toolbar/Search", GNOME_STOCK_PIXMAP_SEARCH,
		(GtkSignalFunc)search_search_cmd_callback },
	{ NULL, NULL, NULL, NULL, NULL }
};

#endif


#ifdef WITHOUT_GNOME
static GtkWidget *new_pixmap(char **icon, GtkWidget *gdkw, GtkWidget *w);
#else
static GtkWidget *new_pixmap(char *fname, GtkWidget *w);
#endif


/*
 * PUBLIC: gE_create_toolbar
 *
 * creates toolbar
 */
void
gE_create_toolbar(gE_window *gw, gE_data *data)
{
	GtkWidget *toolbar;

	toolbar_data_t *tbdp = toolbar_data;

#ifdef WITHOUT_GNOME
	gtk_widget_realize(gw->window);
#endif
	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);

	while (tbdp->text != NULL) {
		gtk_toolbar_append_item(
			GTK_TOOLBAR(toolbar),
#ifdef WITHOUT_GNOME
			tbdp->text,
			tbdp->tooltip_text,
#else
			_(tbdp->text),
			_(tbdp->tooltip_text),
#endif
			tbdp->tooltip_private_text,
#ifdef WITHOUT_GNOME
			new_pixmap(tbdp->icon, gw->window, toolbar),
#else
			new_pixmap(tbdp->icon, toolbar),
#endif
			(GtkSignalFunc)tbdp->callback,
			data);

		tbdp++;

		if (tbdp->text != NULL && strcmp(tbdp->text, " SPACE ") == 0) {
			gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
			tbdp++;
		}
	}

	GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
	gw->toolbar = toolbar;
	gw->toolbar_handle = gtk_handle_box_new();
	gtk_container_add(GTK_CONTAINER(gw->toolbar_handle), toolbar);
	gtk_widget_show(toolbar);

#ifndef WITHOUT_GNOME
	gnome_app_set_toolbar (GNOME_APP (gw->window), GTK_TOOLBAR(toolbar));
#endif

	gw->have_toolbar = TRUE;

} /* gE_create_toolbar */


/*
 * PUBLIC: tb_on_cb
 *
 * unhides toolbar
 */
void
tb_on_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	if (!GTK_WIDGET_VISIBLE(window->toolbar))
		gtk_widget_show (window->toolbar);
	
	if (!GTK_WIDGET_VISIBLE(window->toolbar_handle))
		gtk_widget_show (window->toolbar_handle);

#ifndef WITHOUT_GNOME
	if (!GTK_WIDGET_VISIBLE (GNOME_APP (window->window)->toolbar->parent))
		gtk_widget_show (GNOME_APP(window->window)->toolbar->parent);
#endif
	window->have_toolbar = TRUE;
}


/*
 * PUBLIC: tb_off_cb
 *
 * hides toolbar
 */
void
tb_off_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	if (GTK_WIDGET_VISIBLE(window->toolbar))
		gtk_widget_hide (window->toolbar);
		
	if (GTK_WIDGET_VISIBLE(window->toolbar_handle))
		gtk_widget_hide (window->toolbar_handle);
		
#ifndef WITHOUT_GNOME
	/*
	 * This is a bit of a hack to get the entire toolbar to disappear
	 * instead of just the buttons
	 */
	if (GTK_WIDGET_VISIBLE (GNOME_APP(window->window)->toolbar->parent))
		gtk_widget_hide (GNOME_APP(window->window)->toolbar->parent);
#endif
	window->have_toolbar = FALSE;
}


/*
 * PUBLIC: tb_pic_text_cb
 *
 * updates toolbar to show buttons with icons and text
 */
void
tb_pic_text_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_BOTH);
	window->have_tb_text = 1;
	window->have_tb_pix = 1;
}


/*
 * PUBLIC: tb_pic_only_cb
 *
 * updates toolbar to show buttons with icons only
 */
void
tb_pic_only_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_ICONS);
	window->have_tb_text = 0;
	window->have_tb_pix = 1;
	
#ifndef WITHOUT_GNOME
	/*
	 * forces the gnome toolbar to resize itself.. slows it down some,
	 * but not much..
	 */
	gtk_widget_hide (window->toolbar);
	gtk_widget_show (window->toolbar);
#endif
}


/*
 * PUBLIC: tb_text_only_cb
 *
 * updates toolbar to show buttons with text only
 */
void
tb_text_only_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_TEXT);
	window->have_tb_text = 1;
	window->have_tb_pix = 0;
#ifndef WITHOUT_GNOME
	gtk_widget_hide (window->toolbar);
	gtk_widget_show (window->toolbar);
#endif
}


/*
 * PUBLIC: tb_tooltips_on_cb
 *
 * turns off tooltips
 */
void
tb_tooltips_on_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), TRUE);
}


/*
 * PUBLIC: tb_tooltips_off_cb
 *
 * turns on tooltips
 */
void
tb_tooltips_off_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), FALSE);
}


/*
 * PRIVATE: new_pixmap
 *
 * taken from testgtk.c
 */
static GtkWidget*
#ifdef WITHOUT_GNOME
new_pixmap(char **icon, GtkWidget *gtkw, GtkWidget *w)
#else
new_pixmap(char *fname, GtkWidget *w)
#endif
{
#ifdef WITHOUT_GNOME
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	pixmap = gdk_pixmap_create_from_xpm_d(gtkw->window, &mask,
				&gtkw->style->bg[GTK_STATE_NORMAL], icon);
	return gtk_pixmap_new(pixmap, mask);
#else
	return gnome_stock_pixmap_widget ((GtkWidget *) w, fname);
#endif
} /* new pixmap */

/* the end */
