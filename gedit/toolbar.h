/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998, 1999 Alex Roberts and Evan Lawrence
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

#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include <gtk/gtk.h>
#include "main.h"
#include "gedit-print.h"
#include "search.h"
#include "gE_undo.h"

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

	{ GNOME_APP_UI_ITEM, N_("Undo"), N_("Undo last operation"), gedit_undo_do,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO },
	{ GNOME_APP_UI_ITEM, N_("Redo"), N_("Redo last operation"), gedit_undo_redo,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_REDO },

	{ GNOME_APP_UI_ITEM, N_("Cut"), N_("Cut the selection"), edit_cut_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT },
	{ GNOME_APP_UI_ITEM, N_("Copy"), N_("Copy the selection"), edit_copy_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_COPY },
	{ GNOME_APP_UI_ITEM, N_("Paste"), N_("Paste the clipboard"), edit_paste_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PASTE },
	{ GNOME_APP_UI_ITEM, N_("Find"), N_("Search for a string"), search_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SEARCH },
	{ GNOME_APP_UI_ITEM, N_("Line"), N_("Get the current and total lines"), count_lines_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_INDEX },


	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Exit"), N_("Exit the program"), file_quit_cb,
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_QUIT },

	GNOMEUIINFO_END
};



#endif

/* the end */
