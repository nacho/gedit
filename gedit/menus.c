/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "view.h"
#include "file.h"
#include "search.h"
#include "print.h"
#include "undo.h"

#include "commands.h"
#include "document.h"
#include "dialogs/dialogs.h"

#define GEDIT_DISABLE_VIEW_MENU /* FIXME: This is not working correctly. Chema */


GnomeUIInfo popup_menu[] =
{
	GNOMEUIINFO_MENU_CUT_ITEM (edit_cut_cb, NULL),
        GNOMEUIINFO_MENU_COPY_ITEM (edit_copy_cb, NULL),
	GNOMEUIINFO_MENU_PASTE_ITEM (edit_paste_cb, NULL),
	GNOMEUIINFO_MENU_SELECT_ALL_ITEM (edit_select_all_cb, NULL),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_SAVE_ITEM  (file_save_cb, NULL),
	GNOMEUIINFO_MENU_CLOSE_ITEM (file_close_cb, NULL),
	GNOMEUIINFO_MENU_PRINT_ITEM (gedit_print_cb, NULL),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Open (swap) .c/.h file"), NULL,
				gedit_document_swap_hc_cb, GNOME_STOCK_MENU_REFRESH),
	
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_file_menu[] =
{
        GNOMEUIINFO_MENU_NEW_ITEM (N_("_New"),
				   N_("Create a new document"),
				   file_new_cb, NULL),

	GNOMEUIINFO_MENU_OPEN_ITEM (file_open_cb, NULL),
	GNOMEUIINFO_MENU_SAVE_ITEM (file_save_cb, NULL),
	GNOMEUIINFO_ITEM_STOCK (N_("Save All"),
				N_("Save All Open Files"),
				file_save_all_cb,
				GNOME_STOCK_MENU_SAVE),

	GNOMEUIINFO_MENU_SAVE_AS_ITEM (file_save_as_cb, NULL),
	GNOMEUIINFO_ITEM_STOCK (N_("Revert"),
				NULL,
				file_revert_cb, 
				GNOME_STOCK_MENU_REFRESH),

	GNOMEUIINFO_SEPARATOR,
	/* Recent files ... */
	GNOMEUIINFO_SEPARATOR,
	
	GNOMEUIINFO_ITEM_STOCK (N_("Open from _URI..."),
			  N_("Open a file from a specified URI"),
			  uri_open_cb,
			  GNOME_STOCK_MENU_OPEN),
/*	
	GNOMEUIINFO_ITEM_STOCK (N_("Save _to URI..."),
			  N_("Save the current file to a specified URI"),
			  NULL,
			  GNOME_STOCK_MENU_SAVE_AS),
*/	
	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_PRINT_ITEM (gedit_print_cb, NULL),
#if 0	
	GNOMEUIINFO_ITEM_DATA (
		N_("Print pre_view"), N_("Print preview"),
		gedit_print_preview_cb, NULL, NULL),
/*		file_print_preview_cb, NULL, preview_xpm),*/
#else	
	GNOMEUIINFO_ITEM (N_("Print preview..."),
			  N_("Preview data to be printed"),
			  gedit_print_preview_cb, NULL),
#endif	

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_CLOSE_ITEM (file_close_cb, NULL),
	GNOMEUIINFO_ITEM_STOCK (N_("Close All"),
				N_("Close All Open Files"),
				file_close_all_cb,
				GNOME_STOCK_MENU_CLOSE),

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_EXIT_ITEM (file_quit_cb, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo gedit_edit_menu[] =
{	
	GNOMEUIINFO_MENU_UNDO_ITEM (gedit_undo_undo, NULL),
	GNOMEUIINFO_MENU_REDO_ITEM (gedit_undo_redo, NULL),

	GNOMEUIINFO_SEPARATOR,

        GNOMEUIINFO_MENU_CUT_ITEM (edit_cut_cb, NULL),
        GNOMEUIINFO_MENU_COPY_ITEM (edit_copy_cb, NULL),
	GNOMEUIINFO_MENU_PASTE_ITEM (edit_paste_cb, NULL),
	GNOMEUIINFO_MENU_SELECT_ALL_ITEM (edit_select_all_cb, NULL),

	GNOMEUIINFO_SEPARATOR,
	
	{
		GNOME_APP_UI_ITEM,
		N_("Goto _Line..."),
		N_("Go to a specific line number"),
		gedit_goto_line_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_JUMP_TO
	},

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_FIND_ITEM       (gedit_find_cb, NULL),
	GNOMEUIINFO_MENU_FIND_AGAIN_ITEM (gedit_find_again_cb, NULL),
	GNOMEUIINFO_MENU_REPLACE_ITEM    (gedit_replace_cb, NULL),

	GNOMEUIINFO_SEPARATOR,

	{
		GNOME_APP_UI_ITEM,
		N_("File _Info"),
		N_("Get the basic file info"),
		gedit_file_info_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX
	},


	GNOMEUIINFO_END
};

#ifndef GEDIT_DISABLE_VIEW_MENU
GnomeUIInfo view_menu[] =
{
	GNOMEUIINFO_ITEM_NONE (N_("_Add View"),
			       N_("Add a new view of the document"),
			       gedit_view_add_cb),
	GNOMEUIINFO_ITEM_NONE (N_("_Remove View"),
			       N_("Remove view of the document"),
			       gedit_view_remove_cb),
	GNOMEUIINFO_END
};
#endif

GnomeUIInfo doc_menu[] =
{
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_settings_menu[] =
{
	GNOMEUIINFO_MENU_PREFERENCES_ITEM (gedit_dialog_prefs, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo gedit_docs_menu[] =
{
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_help_menu[] =
{
	GNOMEUIINFO_HELP ("gedit"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (gedit_dialog_about, NULL),
	GNOMEUIINFO_END
};


GnomeUIInfo gedit_plugins_menu[] =
{
	GNOMEUIINFO_END
};


GnomeUIInfo gedit_menu[] =
{
        GNOMEUIINFO_MENU_FILE_TREE (gedit_file_menu),
	GNOMEUIINFO_MENU_EDIT_TREE (gedit_edit_menu),
	{
		GNOME_APP_UI_SUBTREE,
		N_("_Plugins"),
		NULL,
		&gedit_plugins_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
#ifndef GEDIT_DISABLE_VIEW_MENU
	GNOMEUIINFO_MENU_VIEW_TREE (view_menu),
#endif	
	GNOMEUIINFO_MENU_SETTINGS_TREE (gedit_settings_menu),
	{
		GNOME_APP_UI_SUBTREE,
		N_("_Documents"),
		NULL,
		&gedit_docs_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
	GNOMEUIINFO_MENU_HELP_TREE (gedit_help_menu),

	GNOMEUIINFO_END
};

GnomeUIInfo toolbar_data[] =
{
	{
		GNOME_APP_UI_ITEM, N_("New"), N_("Create a new document"), file_new_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW
	},
	{
		GNOME_APP_UI_ITEM, N_("Open"), N_("Open a file"), file_open_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN
	},
	{
		GNOME_APP_UI_ITEM, N_("Save"), N_("Save the current file"), file_save_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE
	},
	{
		GNOME_APP_UI_ITEM, N_("Close"), N_("Close the current file"), file_close_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE
	},
	{
		GNOME_APP_UI_ITEM, N_("Print"), N_("Print the current file"), gedit_print_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PRINT
	},
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM, N_("Undo"), N_("Undo last operation"), gedit_undo_undo,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO
	},
	{
		GNOME_APP_UI_ITEM, N_("Redo"), N_("Redo last operation"), gedit_undo_redo,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_REDO
	},
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM, N_("Cut"), N_("Cut the selection"), edit_cut_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT
	},
	{
		GNOME_APP_UI_ITEM, N_("Copy"), N_("Copy the selection"), edit_copy_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_COPY
	},
	{
		GNOME_APP_UI_ITEM, N_("Paste"), N_("Paste the clipboard"), edit_paste_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PASTE
	},
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM, N_("Find"), N_("Search for a string"), gedit_find_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SEARCH
	},
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM, N_("Exit"), N_("Exit the program"), file_quit_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_QUIT
	},
	GNOMEUIINFO_END
};

