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
	{ " Find ", "Find text", "Toolbar/Find", "xpm/tb_search.xpm",
		(GtkSignalFunc)search_search_cmd_callback },
	{ " SPACE ", NULL, NULL, NULL, NULL },

	{ NULL, NULL, NULL, NULL, NULL }
};


static GtkWidget *new_pixmap(char *fname, GdkWindow *gdkw, GdkColor *bgcolor);


/*
 * PUBLIC: gE_create_toolbar
 *
 * creates toolbar
 */
void
gE_create_toolbar(gE_window *gw)
{
	GtkWidget *toolbar;
	GtkWidget *w = gw->window;
	toolbar_data_t *tbdp = toolbar_data;

	gtk_widget_realize(w);
	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);

	while (tbdp->callback != NULL) {
		gtk_toolbar_append_item(
			GTK_TOOLBAR(toolbar),
			tbdp->text,
			tbdp->tooltip_text,
			tbdp->tooltip_private_text,
			new_pixmap(tbdp->icon, w->window, &w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)tbdp->callback,
			NULL);

		tbdp++;

		if (tbdp->text != NULL && strcmp(tbdp->text, " SPACE ") == 0) {
			gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
			tbdp++;
		}
	}

	gw->toolbar = toolbar;

} /* gE_create_toolbar */


/*
 * PUBLIC: tb_on_cb
 *
 * unhides toolbar
 */
void
tb_on_cb(GtkWidget *w, gpointer data)
{
	if (!GTK_WIDGET_VISIBLE(main_window->toolbar))
		gtk_widget_show (main_window->toolbar);
	main_window->have_toolbar = 1;
}


/*
 * PUBLIC: tb_off_cb
 *
 * hides toolbar
 */
void
tb_off_cb(GtkWidget *w, gpointer data)
{
	if (GTK_WIDGET_VISIBLE(main_window->toolbar))
		gtk_widget_hide (main_window->toolbar);
	main_window->have_toolbar = 0;
}


/*
 * PUBLIC: tb_pic_text_cb
 *
 * updates toolbar to show buttons with icons and text
 */
void
tb_pic_text_cb(GtkWidget *w, gpointer data)
{
	gtk_toolbar_set_style(GTK_TOOLBAR(main_window->toolbar), GTK_TOOLBAR_BOTH);
}


/*
 * PUBLIC: tb_pic_only_cb
 *
 * updates toolbar to show buttons with icons only
 */
void
tb_pic_only_cb(GtkWidget *w, gpointer data)
{
	gtk_toolbar_set_style(GTK_TOOLBAR(main_window->toolbar), GTK_TOOLBAR_ICONS);
}


/*
 * PUBLIC: tb_text_only_cb
 *
 * updates toolbar to show buttons with text only
 */
void
tb_text_only_cb(GtkWidget *w, gpointer data)
{
	gtk_toolbar_set_style(GTK_TOOLBAR(main_window->toolbar), GTK_TOOLBAR_TEXT);
}


/*
 * PUBLIC: tb_tooltips_on_cb
 *
 * turns off tooltips
 */
void
tb_tooltips_on_cb(GtkWidget *w, gpointer data)
{
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(main_window->toolbar), TRUE);
}


/*
 * PUBLIC: tb_tooltips_off_cb
 *
 * turns on tooltips
 */
void
tb_tooltips_off_cb(GtkWidget *w, gpointer data)
{
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(main_window->toolbar), FALSE);
}


/*
 * PRIVATE: new_pixmap
 *
 * taken from testgtk.c
 */
static GtkWidget*
new_pixmap(char *fname, GdkWindow *gdkw, GdkColor *bgcolor)
{
	GtkWidget *wpixmap;
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	pixmap = gdk_pixmap_create_from_xpm(gdkw, &mask, bgcolor, fname);
	wpixmap = gtk_pixmap_new(pixmap, mask);

	return wpixmap;
} /* new pixmap */

/* the end */
