/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit Menus Definition
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
#include "main.h"
#include "commands.h"
#include "gedit-about.h"
#include "gE_files.h"
#include "gE_mdi.h"
#include "gE_print.h"
#include "gE_prefs.h"
#include "gE_prefs_box.h"
#include "gE_view.h"
#include "gE_window.h"
#include "search.h"

/*
#include <gtk/gtk.h>
#include <strings.h>
#include <stdio.h>
#include "toolbar.h"
#include "gedit_prefs.h"
*/


#define GE_DATA		1
#define GE_WINDOW	2

/*
 * The labels for the togglable menu items.  After the settings are
 * loaded, we need to set the states of these items, and so we need a
 * reliable way to reference them when we traverse the menu tree.  And
 * that's what these macros are for.
 */
#define GE_TOGGLE_LABEL_AUTOINDENT      N_("_Autoindent")
#define GE_TOGGLE_LABEL_STATUSBAR       N_("Status_bar")
#define GE_TOGGLE_LABEL_WORDWRAP        N_("_Wordwrap")
#define GE_TOGGLE_LABEL_LINEWRAP        N_("_Linewrap")
#define GE_TOGGLE_LABEL_READONLY        N_("_Readonly")
#define GE_TOGGLE_LABEL_SPLITSCREEN     N_("_Splitscreen")
#define GE_TOGGLE_LABEL_SHOWTABS        N_("_Show tabs")
#define GE_TOGGLE_LABEL_TOOLBAR_RELIEF  N_("Toolbar _relief")
#define GE_TOGGLE_LABEL_TOOLTIPS        N_("_Show tooltips")
#define GE_TOGGLE_LABEL_TOOLBAR_TEXT    N_("Show toolbar _text")
#define GE_TOGGLE_LABEL_TOOLBAR_PIX     N_("Show toolbar _icons")

/* xgettext doesn't find the strings to translate behind a #define, so
   here are they again, in a useless and unused structure.
   a quick and dirty hack, I know -- Pablo Saratxaga <srtxg@chanae.alphanet.ch>
 */
char *just_a_quick_and_dirty_hack[]={
	N_("_Autoindent"),
        N_("Status_bar"),
        N_("_Wordwrap"),
        N_("_Linewrap"),
        N_("_Readonly"),
        N_("_Splitscreen"),
        N_("_Show tabs"),
        N_("Toolbar _relief"),
        N_("_Show tooltips"),
        N_("Show toolbar _text"),
        N_("Show toolbar _icons"),
/* and this one is the default file name */
	N_("Untitled")
};

	
GnomeUIInfo gedit_file_menu [] = {

        GNOMEUIINFO_MENU_NEW_ITEM(N_("_New"), N_("Create a new document"),
				  file_new_cb, NULL),

	GNOMEUIINFO_MENU_OPEN_ITEM(file_open_cb, NULL),

	GNOMEUIINFO_MENU_SAVE_ITEM(file_save_cb, NULL),

	GNOMEUIINFO_ITEM_STOCK (N_("Save All"),N_("Save All Open Files"),
						file_save_all_cb, GNOME_STOCK_MENU_SAVE),

	GNOMEUIINFO_MENU_SAVE_AS_ITEM(file_save_as_cb, NULL),

	GNOMEUIINFO_ITEM_STOCK (N_("Revert"),NULL,file_revert_cb,GNOME_STOCK_MENU_REFRESH),

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_PRINT_ITEM(file_print_cb, (gpointer) GE_DATA),
	GNOMEUIINFO_ITEM(N_("Print preview..."), N_("Preview data to be printed"), file_print_preview_cb, NULL),
	
	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_CLOSE_ITEM(file_close_cb, NULL),

	GNOMEUIINFO_ITEM_STOCK (N_("Close All"),N_("Close All Open Files"),
						file_close_all_cb, GNOME_STOCK_MENU_CLOSE),

	GNOMEUIINFO_MENU_EXIT_ITEM(file_quit_cb, NULL),

	GNOMEUIINFO_END

};



GnomeUIInfo gedit_tab_menu [] = {

	{ GNOME_APP_UI_ITEM, N_("_Top"),
	  N_("Put the document tabs at the top"),
	  tab_top_cb, NULL },

	{ GNOME_APP_UI_ITEM, N_("_Bottom"),
	  N_("Put the document tabs at the bottom"),
	  tab_bot_cb, NULL },

	{ GNOME_APP_UI_ITEM, N_("_Left"),
	  N_("Put the document tabs on the left"),
	  tab_lef_cb, NULL },

	{ GNOME_APP_UI_ITEM, N_("_Right"),
	  N_("Put the document tabs on the right"),
	  tab_rgt_cb, NULL },

/*	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SHOWTABS,
			    N_("Toggle the presence of the document tabs"),
				    tab_toggle_cb, (gpointer) GE_WINDOW, NULL),*/
	
	GNOMEUIINFO_END

};


GnomeUIInfo gedit_settings_menu [] = {
/* -- These settings are in the Preferences Box, and only need to be
      there.. Readonly and Splitscreen are on a per-document basis
      
        GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_AUTOINDENT,
				    N_("Toggle autoindent"),
				    auto_indent_toggle_cb, (gpointer) GE_DATA,
				    NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_STATUSBAR,
				    N_("Toggle statusbar"),
				    options_toggle_status_bar_cb,
				    NULL, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_WORDWRAP,
				    N_("Toggle Wordwrap"),
				    options_toggle_word_wrap_cb,
				    NULL, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_LINEWRAP,
				    N_("Toggle Linewrap"),
				    options_toggle_line_wrap_cb,
				    NULL, NULL),
*/
	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_READONLY,
				    N_("Toggle Readonly"),
				    options_toggle_read_only_cb,
				    NULL, NULL),

/*	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SPLITSCREEN,
				    N_("Toggle Split Screen"),
				    options_toggle_split_screen_cb,
				    NULL, NULL),
*/
	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_SUBTREE, N_("_Document Tabs"),
	  N_("Change the placement of the document tabs"), &gedit_tab_menu },
	  
	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Sa_ve Settings"),
	  N_("Save the current settings for future sessions"),
	  gedit_save_settings, (gpointer) GE_WINDOW, NULL },

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_PREFERENCES_ITEM(gedit_prefs_dialog, (gpointer) GE_DATA),

	GNOMEUIINFO_END

};

GnomeUIInfo gedit_window_menu [] = {

 	GNOMEUIINFO_MENU_NEW_WINDOW_ITEM(window_new_cb, (gpointer) GE_DATA),

/*FIXME        GNOMEUIINFO_MENU_CLOSE_WINDOW_ITEM(window_close_cb,
					   (gpointer) GE_DATA),*/
	
/*	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("_Document List"),
	  N_("Display the document list"),
	  files_list_popup, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 'L', GDK_CONTROL_MASK, NULL },*/

	GNOMEUIINFO_END

};

GnomeUIInfo gedit_docs_menu[] = {
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_help_menu []= {

	GNOMEUIINFO_HELP ("gedit"),

	GNOMEUIINFO_MENU_ABOUT_ITEM (gedit_about, NULL),

	GNOMEUIINFO_END
};


GnomeUIInfo gedit_plugins_menu []= {
	GNOMEUIINFO_END
};


GnomeUIInfo gedit_menu [] = {

        GNOMEUIINFO_MENU_FILE_TREE(gedit_file_menu),

	{ GNOME_APP_UI_SUBTREE, N_("_Plugins"), NULL, &gedit_plugins_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },

	GNOMEUIINFO_MENU_SETTINGS_TREE(gedit_settings_menu),
	GNOMEUIINFO_MENU_WINDOWS_TREE(gedit_window_menu),
	GNOMEUIINFO_MENU_FILES_TREE(gedit_docs_menu),
	GNOMEUIINFO_MENU_HELP_TREE(gedit_help_menu),

	GNOMEUIINFO_END

};

GnomeUIInfo *
gedit_menus_init (gedit_window *window, gedit_data *data)
{

	gnome_app_create_menus (GNOME_APP (mdi->active_window), gedit_menu);

	return (GnomeUIInfo *) gedit_menu;
}
