/* vi:set ts=8 sts=0 sw=8:
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
#include "gE_print.h"
#include "gE_files.h"
#include "search.h"
#include "msgbox.h"
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
#include "xpm/tb_exit.xpm"
#endif


#ifdef WITHOUT_GNOME

static toolbar_data_t toolbar_data[] = {
	{ " New ", "Start a new file", "Toolbar/New", tb_new_xpm,
		(GtkSignalFunc)file_new_cb },
	{ " Open ", "Open a file", "Toolbar/Open", tb_open_xpm,
		(GtkSignalFunc)file_open_cb },
	{ " Save ", "Save file", "Toolbar/Save", tb_save_xpm,
		(GtkSignalFunc)file_save_cb },
	{ " Close ", "Close the current file", "Toolbar/Close", cancel_xpm,
		(GtkSignalFunc)file_close_cb },
	{ " Print ", "Print file", "Toolbar/Print", tb_print_xpm,
		(GtkSignalFunc)file_print_cb },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ " Cut ", "Cut text", "Toolbar/Cut", tb_cut_xpm,
		(GtkSignalFunc)edit_cut_cb },
	{ " Copy ", "Copy text", "Toolbar/Copy", tb_copy_xpm,
		(GtkSignalFunc)edit_copy_cb },
	{ " Paste ", "Paste text", "Toolbar/Paste", tb_paste_xpm,
		(GtkSignalFunc)edit_paste_cb },
	{ " Search ", "Search for text", "Toolbar/Search", tb_search_xpm,
		(GtkSignalFunc)search_cb },
	{ NULL, NULL, NULL, NULL, NULL }
};

#else	/* USING GNOME */

static toolbar_data_t toolbar_data[] = {
	{ N_(" New "), N_("Start a new file"), "Toolbar/New",
		GNOME_STOCK_PIXMAP_NEW, (GtkSignalFunc)file_new_cb },
	{ N_(" Open "), N_("Open a file"), "Toolbar/Open",
		GNOME_STOCK_PIXMAP_OPEN, (GtkSignalFunc)file_open_cb },
	{ N_(" Save "), N_("Save file"), "Toolbar/Save",
		GNOME_STOCK_PIXMAP_SAVE, (GtkSignalFunc)file_save_cb },
	{ N_(" Close "), N_("Close the current file"), "Toolbar/Close",
		GNOME_STOCK_PIXMAP_CLOSE, (GtkSignalFunc)file_close_cb },
	{ N_(" Print "), N_("Print file"), "Toolbar/Print",
		GNOME_STOCK_PIXMAP_PRINT, (GtkSignalFunc)file_print_cb },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ N_(" Cut "), N_("Cut text"), "Toolbar/Cut",
		GNOME_STOCK_PIXMAP_CUT, (GtkSignalFunc)edit_cut_cb },
	{ N_(" Copy "), N_("Copy text"), "Toolbar/Copy",
		GNOME_STOCK_PIXMAP_COPY, (GtkSignalFunc)edit_copy_cb },
	{ N_(" Paste "), N_("Paste text"), "Toolbar/Paste",
		GNOME_STOCK_PIXMAP_PASTE, (GtkSignalFunc)edit_paste_cb },
	{ N_(" Search "), N_("Search for text"), "Toolbar/Search",
		GNOME_STOCK_PIXMAP_SEARCH, (GtkSignalFunc)search_cb },
	{ NULL, NULL, NULL, NULL, NULL }
};

#endif	/* #ifdef WITHOUT_GNOME */

#ifdef WITHOUT_GNOME
static toolbar_data_t flw_tb_data[] = {
	{ " Save ", "Save file", "Toolbar/Save", tb_save_xpm,
		(GtkSignalFunc)file_save_cb },
	{ " Close ", "Close the current file", "Toolbar/Close", cancel_xpm,
		(GtkSignalFunc)file_close_cb },
	{ " Print ", "Print file", "Toolbar/Print", tb_print_xpm,
		(GtkSignalFunc)file_print_cb },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ " Ok ", "Close list window", "Ok", exit_xpm,
		(GtkSignalFunc)flw_destroy },
	{ NULL, NULL, NULL, NULL, NULL }
};
#else
static toolbar_data_t flw_tb_data[] = {
	{ N_(" Save "), "Save file", "Toolbar/Save",
		GNOME_STOCK_PIXMAP_SAVE, (GtkSignalFunc)file_save_cb },
	{ N_(" Close "), "Close the current file", "Toolbar/Close",
		GNOME_STOCK_PIXMAP_CLOSE, (GtkSignalFunc)file_close_cb },
	{ N_(" Print "), "Print file", "Toolbar/Print",
		GNOME_STOCK_PIXMAP_PRINT, (GtkSignalFunc)file_print_cb },
	{ " SPACE ", NULL, NULL, NULL, NULL },
	{ N_(" OK "), "Close list window", "Ok",
		GNOME_STOCK_PIXMAP_QUIT, (GtkSignalFunc)flw_destroy },
	{ NULL, NULL, NULL, NULL, NULL }
};
#endif /* #ifdef WITHOUT_GNOME */

#ifdef WITHOUT_GNOME
static GtkWidget *new_pixmap(char **icon, GtkWidget *gdkw, GtkWidget *w);
#else
static GtkWidget *new_pixmap(char *fname, GtkWidget *w);
#endif

static GtkWidget *toolbar_create_common(toolbar_data_t *tbdata, gE_data *data);


/*
 * PUBLIC: gE_create_toolbar
 *
 * creates main toolbar that goes below menu bar
 */
void
gE_create_toolbar(gE_window *gw, gE_data *data)
{
	GtkWidget *toolbar;

#ifdef WITHOUT_GNOME
	gtk_widget_realize(gw->window);
#endif

	toolbar = toolbar_create_common(toolbar_data, data);

#ifdef GTK_HAVE_FEATURES_1_1_0
#ifndef WITHOUT_GNOME
/*	if ( ! gnome_preferences_get_toolbar_relief() ) */
#endif
/*		Dunno if it's me, but i find relief buttons ugly... --Alex
		gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), 
					      GTK_RELIEF_NONE);
*/
#endif /* GTK_HAVE_FEATURES_1_1_0 */

	GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
	gw->toolbar = toolbar;
#ifdef WITHOUT_GNOME	
	gw->toolbar_handle = gtk_handle_box_new();
	gtk_container_add(GTK_CONTAINER(gw->toolbar_handle), toolbar);
#endif
	gtk_widget_show(toolbar);

#ifndef WITHOUT_GNOME
	gnome_app_set_toolbar (GNOME_APP (gw->window), GTK_TOOLBAR(toolbar));
#endif

	gw->have_toolbar = TRUE;

} /* gE_create_toolbar */


/*
 * PUBLIC: gE_create_toolbar_flw
 *
 * creates toolbar for files list window
 */
GtkWidget *
gE_create_toolbar_flw(gE_data *data)
{
	return toolbar_create_common(flw_tb_data, data);
}


/*
 * PRIVATE: toolbar_create_common
 *
 * common routine to create a toolbar.
 *
 * in: toolbar_data_t and pointer to callback data of gE_data type
 * out: GtkWidget *toolbar
 */
static GtkWidget *
toolbar_create_common(toolbar_data_t *tbdata, gE_data *data)
{
	GtkWidget *tb;
	GtkWidget *parent = data->window->window;
	toolbar_data_t *tbdp = tbdata;

	g_assert(tbdp != NULL);
	g_assert(parent != NULL);

	tb = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);

	while (tbdp->text != NULL) {
		gtk_toolbar_append_item(
			GTK_TOOLBAR(tb),
#ifdef WITHOUT_GNOME
			tbdp->text,
			tbdp->tooltip_text,
#else
			_(tbdp->text),
			_(tbdp->tooltip_text),
#endif
			tbdp->tooltip_private_text,
#ifdef WITHOUT_GNOME
			new_pixmap(tbdp->icon, parent, tb),
#else
			new_pixmap(tbdp->icon, tb),
#endif
			(GtkSignalFunc)tbdp->callback,
			(gpointer)data);

		tbdp++;

		if (tbdp->text != NULL && strcmp(tbdp->text, " SPACE ") == 0) {
			gtk_toolbar_append_space(GTK_TOOLBAR(tb));
			tbdp++;
		}
	}

	return tb;
} /* toolbar_create_common */


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
		gtk_widget_show(window->toolbar);

#ifdef WITHOUT_GNOME
	if (!GTK_WIDGET_VISIBLE(window->toolbar_handle))
		gtk_widget_show(window->toolbar_handle);
#endif

/*	if (window->files_list_window_toolbar &&
		!GTK_WIDGET_VISIBLE(window->files_list_window_toolbar))
		gtk_widget_show(window->files_list_window_toolbar);*/

#ifndef WITHOUT_GNOME
	/*if (!GTK_WIDGET_VISIBLE (GNOME_APP(window)->toolbar))*/
	/*if (window-*/
		gtk_widget_show (GNOME_APP(window->window)->toolbar->parent);
#endif

	window->have_toolbar = TRUE;
	printf("..WORK	damn you!\n");
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
		gtk_widget_hide(window->toolbar);

#ifdef WITHOUT_GNOME
	if (GTK_WIDGET_VISIBLE(window->toolbar_handle))
		gtk_widget_hide(window->toolbar_handle);
#endif

/*	if (window->files_list_window_toolbar &&
		GTK_WIDGET_VISIBLE(window->files_list_window_toolbar))
		gtk_widget_hide(window->files_list_window_toolbar);*/

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
/*	if (window->files_list_window_toolbar) {
		gtk_toolbar_set_style(
			GTK_TOOLBAR(window->files_list_window_toolbar),
			GTK_TOOLBAR_BOTH);
		if (GTK_WIDGET_VISIBLE(window->files_list_window_toolbar)) {
			gtk_widget_hide(window->files_list_window_toolbar);
			gtk_widget_show(window->files_list_window_toolbar);
		}
	}*/
	window->have_tb_text = TRUE;
	window->have_tb_pix = TRUE;
	if (GTK_WIDGET_VISIBLE(window->toolbar)) {
		gtk_widget_hide(window->toolbar);
		gtk_widget_show(window->toolbar);
	}
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
/*	if (window->files_list_window_toolbar) {
		gtk_toolbar_set_style(
			GTK_TOOLBAR(window->files_list_window_toolbar),
			GTK_TOOLBAR_ICONS);
		if (GTK_WIDGET_VISIBLE(window->files_list_window_toolbar)) {
			gtk_widget_hide(window->files_list_window_toolbar);
			gtk_widget_show(window->files_list_window_toolbar);
		}
	}*/
	window->have_tb_text = FALSE;
	window->have_tb_pix = TRUE;
	
	/*
	 * forces the gnome toolbar to resize itself.. slows it down some,
	 * but not much..
	 */
	if (GTK_WIDGET_VISIBLE(window->toolbar)) {
		gtk_widget_hide(window->toolbar);
		gtk_widget_show(window->toolbar);
	}
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
/*	if (window->files_list_window_toolbar) {
		gtk_toolbar_set_style(
			GTK_TOOLBAR(window->files_list_window_toolbar),
			GTK_TOOLBAR_TEXT);
		if (GTK_WIDGET_VISIBLE(window->files_list_window_toolbar)) {
			gtk_widget_hide(window->files_list_window_toolbar);
			gtk_widget_show(window->files_list_window_toolbar);
		}
	}*/
	window->have_tb_text = TRUE;
	window->have_tb_pix = FALSE;
	if (GTK_WIDGET_VISIBLE(window->toolbar)) {
		gtk_widget_hide(window->toolbar);
		gtk_widget_show(window->toolbar);
	}
}


/*
 * PUBLIC: tb_tooltips_on_cb
 *
 * turns ON tooltips
 */
void
tb_tooltips_on_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), TRUE);
/*	if (window->files_list_window_toolbar)
		gtk_toolbar_set_tooltips(
			GTK_TOOLBAR(window->files_list_window_toolbar), TRUE);*/
	window->show_tooltips = TRUE;
}


/*
 * PUBLIC: tb_tooltips_off_cb
 *
 * turns OFF tooltips
 */
void
tb_tooltips_off_cb(GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), FALSE);
/*	if (window->files_list_window_toolbar)
		gtk_toolbar_set_tooltips(
			GTK_TOOLBAR(window->files_list_window_toolbar), FALSE);*/
	window->show_tooltips = FALSE;
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
