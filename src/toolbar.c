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
#include "menus.h"
#include "toolbar.h"


/*
 * XXX fixme - should make xpm pathname configurable or at least set
 * in a static location (e.g., /usr/local/lib/gedit/xpm)
 */
 /* ^^^ Proper location would be $INSTALL_DIR/share/pixmaps/gedit ^^^ */
 
#ifdef WITHOUT_GNOME

static toolbar_data_t toolbar_data[] = {
	{ " New ", "Start a new file", "Toolbar/New", "xpm/tb_new.xpm",
		(GtkSignalFunc)file_new_cmd_callback },
	{ " Open ", "Open a file", "Toolbar/Open", "xpm/tb_open.xpm",
		(GtkSignalFunc)file_open_cmd_callback },
	{ " Save ", "Save file", "Toolbar/Save", "xpm/tb_save.xpm",
		(GtkSignalFunc)file_save_cmd_callback },
	{ " Close ", "Close the current file", "Toolbar/Close", "xpm/tb_cancel.xpm",
		(GtkSignalFunc)file_close_cmd_callback },
	{ " Print ", "Print file", "Toolbar/Print", "xpm/tb_print.xpm",
		(GtkSignalFunc)file_print_cmd_callback },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ " Cut ", "Cut text", "Toolbar/Cut", "xpm/tb_cut.xpm",
		(GtkSignalFunc)edit_cut_cmd_callback },
	{ " Copy ", "Copy text", "Toolbar/Copy", "xpm/tb_copy.xpm",
		(GtkSignalFunc)edit_copy_cmd_callback },
	{ " Paste ", "Paste text", "Toolbar/Paste", "xpm/tb_paste.xpm",
		(GtkSignalFunc)edit_paste_cmd_callback },
	{ " Search ", "Search for text", "Toolbar/Search", "xpm/tb_search.xpm",
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
static GtkWidget *new_pixmap(char *fname, GtkWidget *gdkw, GtkWidget *w);
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

	while (tbdp->callback != NULL) {
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

	gw->toolbar = toolbar;
	gtk_widget_show (toolbar);

#ifndef WITHOUT_GNOME
	gnome_app_set_toolbar (GNOME_APP (gw->window), GTK_TOOLBAR(toolbar));
#endif

	gw->have_toolbar = 1;

} /* gE_create_toolbar */


/*
 * PUBLIC: tb_on_cb
 *
 * unhides toolbar
 */
void
tb_on_cb(GtkWidget *w, gE_window *window)
{
	if (!GTK_WIDGET_VISIBLE(window->toolbar))
		gtk_widget_show (window->toolbar);

#ifndef WITHOUT_GNOME
	if (!GTK_WIDGET_VISIBLE (GNOME_APP (window->window)->toolbar->parent))
		gtk_widget_show (GNOME_APP(window->window)->toolbar->parent);
#endif
	window->have_toolbar = 1;
}


/*
 * PUBLIC: tb_off_cb
 *
 * hides toolbar
 */
void
tb_off_cb(GtkWidget *w, gE_window *window)
{
	if (GTK_WIDGET_VISIBLE(window->toolbar))
		gtk_widget_hide (window->toolbar);
		
#ifndef WITHOUT_GNOME
	/*
	 * This is a bit of a hack to get the entire toolbar to disappear
	 * instead of just the buttons
	 */
	if (GTK_WIDGET_VISIBLE (GNOME_APP(window->window)->toolbar->parent))
		gtk_widget_hide (GNOME_APP(window->window)->toolbar->parent);
#endif
	window->have_toolbar = 0;
}


/*
 * PUBLIC: tb_pic_text_cb
 *
 * updates toolbar to show buttons with icons and text
 */
void
tb_pic_text_cb(GtkWidget *w, gE_window *window)
{
	gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_BOTH);
}


/*
 * PUBLIC: tb_pic_only_cb
 *
 * updates toolbar to show buttons with icons only
 */
void
tb_pic_only_cb(GtkWidget *w, gE_window *window)
{
	gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_ICONS);

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
tb_text_only_cb(GtkWidget *w, gE_window *window)
{
	gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_TEXT);
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
tb_tooltips_on_cb(GtkWidget *w, gE_window *window)
{
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), TRUE);
}


/*
 * PUBLIC: tb_tooltips_off_cb
 *
 * turns on tooltips
 */
void
tb_tooltips_off_cb(GtkWidget *w, gE_window *window)
{
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), FALSE);
}


/*
 * PRIVATE: new_pixmap
 *
 * taken from testgtk.c
 */
static GtkWidget*
#ifdef WITHOUT_GNOME
new_pixmap(char *fname, GtkWidget *gtkw, GtkWidget *w)
#else
new_pixmap(char *fname, GtkWidget *w)
#endif
{
	GtkWidget *wpixmap;
#ifdef WITHOUT_GNOME
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	pixmap = gdk_pixmap_create_from_xpm(gtkw->window, &mask,
				&gtkw->style->bg[GTK_STATE_NORMAL], fname);
	wpixmap = gtk_pixmap_new(pixmap, mask);
#else
	wpixmap = gnome_stock_pixmap_widget ((GtkWidget *) w, fname);
#endif

	return wpixmap;
} /* new pixmap */

/* the end */
