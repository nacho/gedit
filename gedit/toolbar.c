/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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
#include <config.h>
#include <gnome.h>

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "commands.h"
#include "gE_print.h"
#include "gE_files.h"
#include "search.h"
#include "toolbar.h"
#include "gE_prefs_box.h"
#include "gE_prefs.h"

GnomeUIInfo toolbar_data[] = {
	{ GNOME_APP_UI_ITEM, N_("New"), N_("Create a new document"), file_new_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW },
	{ GNOME_APP_UI_ITEM, N_("Open"), N_("Open a file"), file_open_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN },
	{ GNOME_APP_UI_ITEM, N_("Save"), N_("Save the current file"), file_save_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE },
	{ GNOME_APP_UI_ITEM, N_("Close"), N_("Close the current file"), file_close_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE },
	{ GNOME_APP_UI_ITEM, N_("Print"), N_("Print the current file"), file_print_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PRINT },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Cut"), N_("Cut the selection"), edit_cut_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT },
	{ GNOME_APP_UI_ITEM, N_("Copy"), N_("Copy the selection"), edit_copy_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_COPY },
	{ GNOME_APP_UI_ITEM, N_("Paste"), N_("Paste the clipboard"), edit_paste_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PASTE },
	{ GNOME_APP_UI_ITEM, N_("Find"), N_("Search for a string"), search_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SEARCH },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Exit"), N_("Exit the program"), file_quit_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_QUIT },

	GNOMEUIINFO_END
};


static GtkWidget *new_pixmap(char *fname, GtkWidget *w);


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

	gnome_app_create_toolbar_with_data (GNOME_APP (gw->window),
                                    toolbar_data,
                                    data );

	settings->have_toolbar = TRUE;

} /* gE_create_toolbar */



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

			_(tbdp->text),
			_(tbdp->tooltip_text),

			tbdp->tooltip_private_text,

			new_pixmap(tbdp->icon, tb),

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
 * PRIVATE: new_pixmap
 *
 * taken from testgtk.c
 */
static GtkWidget*

new_pixmap(char *fname, GtkWidget *w)
{

	return gnome_stock_pixmap_widget ((GtkWidget *) w, fname);

} /* new pixmap */

/* the end */
