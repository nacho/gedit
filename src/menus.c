/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * gedit - Menu definitions
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

#include <config.h>
#include <gnome.h>

#define PLUGIN_TEST 1
#include "window.h"
#include "file.h"
#include "search.h"
#include "print.h"
#include "undo.h"

#include "commands.h"
#include "document.h"
#include "prefs.h"
#include "view.h"

#include "dialogs/dialogs.h"

GnomeUIInfo popup_menu[] =
{
	GNOMEUIINFO_MENU_CUT_ITEM (edit_cut_cb, NULL),
        GNOMEUIINFO_MENU_COPY_ITEM (edit_copy_cb, NULL),
	GNOMEUIINFO_MENU_PASTE_ITEM (edit_paste_cb, NULL),
	GNOMEUIINFO_MENU_SELECT_ALL_ITEM (edit_selall_cb, NULL),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_SAVE_ITEM  (file_save_cb, NULL),
	GNOMEUIINFO_MENU_CLOSE_ITEM (file_close_cb, NULL),
	GNOMEUIINFO_MENU_PRINT_ITEM (file_print_cb, NULL),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Open (swap) .c/.h file"), NULL,
				doc_swaphc_cb, GNOME_STOCK_MENU_REFRESH),
	
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
	GNOMEUIINFO_MENU_REVERT_ITEM (file_revert_cb, NULL),
/*
	GNOMEUIINFO_ITEM_STOCK (N_("Revert"),
				NULL,
				file_revert_cb, 
				GNOME_STOCK_MENU_REFRESH),
*/
	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_PRINT_ITEM (file_print_cb, NULL),
	GNOMEUIINFO_ITEM (N_("Print preview..."),
			  N_("Preview data to be printed"),
			  file_print_preview_cb, NULL),

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_CLOSE_ITEM (file_close_cb, NULL),
	GNOMEUIINFO_ITEM_STOCK (N_("Close All"),
				N_("Close All Open Files"),
				file_close_all_cb,
				GNOME_STOCK_MENU_CLOSE),
	GNOMEUIINFO_MENU_EXIT_ITEM (file_quit_cb, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo gedit_edit_menu[] =
{
	GNOMEUIINFO_MENU_UNDO_ITEM (gedit_undo_do, NULL),
	GNOMEUIINFO_MENU_REDO_ITEM (gedit_undo_redo, NULL),
	GNOMEUIINFO_SEPARATOR,
        GNOMEUIINFO_MENU_CUT_ITEM (edit_cut_cb, NULL),
        GNOMEUIINFO_MENU_COPY_ITEM (edit_copy_cb, NULL),
	GNOMEUIINFO_MENU_PASTE_ITEM (edit_paste_cb, NULL),
	GNOMEUIINFO_MENU_SELECT_ALL_ITEM (edit_selall_cb, NULL),

/*	Simplify the interface. Find in files should be a plugin
	Find in all the files open is something that will be used
	very very little times. We should have it as a plugin, tho
	{
		GNOME_APP_UI_ITEM,
		N_("Find _In Files..."),
		N_("Find text in all open files"),
		find_in_files_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH
	},*/

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_FIND_ITEM (search_cb, NULL),
	GNOMEUIINFO_MENU_REPLACE_ITEM (replace_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM,
		N_("Goto _Line..."),
		N_("Go to a specific line number"),
		find_line_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH
	},
	GNOMEUIINFO_END
};

GnomeUIInfo view_menu[] =
{
	GNOMEUIINFO_ITEM_NONE (N_("_Add View"),
			       N_("Add a new view of the document"),
			       gedit_add_view),
	GNOMEUIINFO_ITEM_NONE (N_("_Remove View"),
			       N_("Remove view of the document"),
			       gedit_remove_view),
	GNOMEUIINFO_END
};

GnomeUIInfo doc_menu[] =
{
	GNOMEUIINFO_MENU_EDIT_TREE (gedit_edit_menu),
	GNOMEUIINFO_MENU_VIEW_TREE (view_menu), 
	GNOMEUIINFO_END
};

/* We need to simplify the menus !!.
   A newbiew will probably never want to change this
   option, disable it as a menu option but add it as
   a preference item.
   Chema. 
GnomeUIInfo gedit_tab_menu[] =
{
	{
		GNOME_APP_UI_ITEM,
		N_("_Top"),
		N_("Put the document tabs at the top"),
		tab_top_cb, NULL
	},
	{
		GNOME_APP_UI_ITEM,
		N_("_Bottom"),
		N_("Put the document tabs at the bottom"),
		tab_bot_cb, NULL
	},
	{
		GNOME_APP_UI_ITEM,
		N_("_Left"),
		N_("Put the document tabs on the left"),
		tab_lef_cb, NULL
	},
	{
		GNOME_APP_UI_ITEM,
		N_("_Right"),
		N_("Put the document tabs on the right"),
		tab_rgt_cb, NULL
	},


	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SHOWTABS,
			            N_("Toggle the presence of the document tabs"),
				    tab_toggle_cb, (gpointer) GE_WINDOW, NULL),
	
				    GNOMEUIINFO_END 
}; */


/* These are the customizable checkboxes in the Settings menu,
   Readonly and Splitscreen are on a per-document basis */
/* We need to simplify the interface. Why would the user
   want to change the ReadOnly flag ???. Chema */
/* The toggle status bar should be in preferneces.
   Same thing here, who will togle the status bar,
   simpify the interface... Chema */

GnomeUIInfo gedit_settings_menu[] =
{
/*      
        GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_AUTOINDENT,
				    N_("Toggle autoindent"),
				    auto_indent_toggle_cb, (gpointer) GE_DATA,
				    NULL),
*/

	GNOMEUIINFO_TOGGLEITEM_DATA(N_("Show Statusbar"),
				    N_("Enable or disable the statusbar at the bottom of this application window."),
				    options_toggle_status_bar_cb,
				    NULL, NULL),

/*
	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_WORDWRAP,
				    N_("Toggle Wordwrap"),
				    options_toggle_word_wrap_cb,
				    NULL, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_LINEWRAP,
				    N_("Toggle Linewrap"),
				    options_toggle_line_wrap_cb,
				    NULL, NULL),
*/
/*
	GNOMEUIINFO_TOGGLEITEM_DATA (N_("_Readonly"),
				     N_("Toggle Readonly"),
				     options_toggle_read_only_cb,
				     NULL, NULL),
*/

/*
	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SPLITSCREEN,
				    N_("Toggle Split Screen"),
				    options_toggle_split_screen_cb,
				    NULL, NULL),
*/
/*	GNOMEUIINFO_SEPARATOR, */

	/*
	{
		GNOME_APP_UI_SUBTREE,
		N_("_Document Tabs"),
		N_("Change the placement of the document tabs"),
		&gedit_tab_menu
	},
		
	GNOMEUIINFO_SEPARATOR,*/
/* Simplify the menus, save setting should be in the preferences window,
   not as a menu option. Chema 
	{
		GNOME_APP_UI_ITEM,
		N_("Sa_ve Settings"),
		N_("Save the current settings for future sessions"),
		gedit_save_settings, NULL,
		NULL
		},*/

/* 	GNOMEUIINFO_SEPARATOR, */ 

	GNOMEUIINFO_MENU_PREFERENCES_ITEM (dialog_prefs, NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_window_menu[] =
{
 	GNOMEUIINFO_MENU_NEW_WINDOW_ITEM (window_new_cb, NULL),

/*FIXME        GNOMEUIINFO_MENU_CLOSE_WINDOW_ITEM(window_close_cb,
					   (gpointer) GE_DATA),*/
	
/*	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("_Document List"),
	  N_("Display the document list"),
	  files_list_popup, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 'L', GDK_CONTROL_MASK, NULL },*/

	GNOMEUIINFO_END
};

GnomeUIInfo gedit_docs_menu[] =
{
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_help_menu[] =
{
	GNOMEUIINFO_HELP ("gedit"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (dialog_about, NULL),
	GNOMEUIINFO_END
};


GnomeUIInfo gedit_plugins_menu[] =
{
	GNOMEUIINFO_END
};


GnomeUIInfo gedit_menu[] =
{
        GNOMEUIINFO_MENU_FILE_TREE (gedit_file_menu),
	{
		GNOME_APP_UI_SUBTREE,
		N_("_Plugins"),
		NULL,
		&gedit_plugins_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL
	},
	GNOMEUIINFO_MENU_SETTINGS_TREE (gedit_settings_menu),
/*	GNOMEUIINFO_MENU_WINDOWS_TREE (gedit_window_menu), disabled by Chema */
	GNOMEUIINFO_MENU_FILES_TREE (gedit_docs_menu),
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
		GNOME_APP_UI_ITEM, N_("Print"), N_("Print the current file"), file_print_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PRINT
	},
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM, N_("Undo"), N_("Undo last operation"), gedit_undo_do,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO
	},
	{
		GNOME_APP_UI_ITEM, N_("Redo"), N_("Redo last operation"), gedit_undo_redo,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_REDO
	},
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
	{
		GNOME_APP_UI_ITEM, N_("Find"), N_("Search for a string"), search_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SEARCH
	},
	{
		GNOME_APP_UI_ITEM, N_("Line"), N_("Get the current and total lines"), count_lines_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_INDEX
	},
	GNOMEUIINFO_SEPARATOR,
	{
		GNOME_APP_UI_ITEM, N_("Exit"), N_("Exit the program"), file_quit_cb,
		NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_QUIT
	},
	GNOMEUIINFO_END
};

void find_line_cb (GtkWidget *widget, gpointer data);
void replace_cb (GtkWidget *widget, gpointer data);
void about_cb (GtkWidget *widget, gpointer data);

#if 0
/**
 * gedit_menus_init:
 * @window:
 * @data: 
 * 
 **/
GnomeUIInfo *
gedit_menus_init (Window *window)
{

	g_return_val_if_fail (window != NULL, NULL);

	gnome_app_create_menus (GNOME_APP (mdi->active_window), gedit_menu);

	return (GnomeUIInfo *) gedit_menu;
}
#endif

void
find_line_cb (GtkWidget *widget, gpointer data)
{
	dialog_find_line ();
}

void
replace_cb (GtkWidget *widget, gpointer data)
{
	dialog_replace ();
}

void
about_cb (GtkWidget *widget, gpointer data)
{
	dialog_about ();
}
