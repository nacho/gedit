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
#include <gtk/gtk.h>
#include <strings.h>
#include <stdio.h>
#include <config.h>
#include <gnome.h>

#define PLUGIN_TEST 1
#include "main.h"
#include "commands.h"
#include "gE_prefs.h"
#include "gE_files.h"
#include "toolbar.h"
#include "gE_document.h"
#include "gE_about.h"
#include "gE_print.h"
#include "gE_prefs.h"
#include "gE_prefs_box.h"
#include "search.h"


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



void add_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data);
void remove_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data);

GnomeUIInfo gedit_file_menu [] = {
        GNOMEUIINFO_MENU_NEW_ITEM(N_("_New"), N_("Create a new document"),
				  file_new_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_OPEN_ITEM(file_open_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_SAVE_ITEM(file_save_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_SAVE_AS_ITEM(file_save_as_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_PRINT_ITEM(file_print_cb, (gpointer) GE_DATA),
	
	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_CLOSE_ITEM(file_close_cb, (gpointer) GE_DATA),

	{ GNOME_APP_UI_ITEM, N_("Close All"), N_("Close all open files"),
	  file_close_all_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE },

	GNOMEUIINFO_MENU_EXIT_ITEM(file_quit_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_END
};


GnomeUIInfo gedit_edit_menu [] = {
        GNOMEUIINFO_MENU_CUT_ITEM(edit_cut_cb, (gpointer) GE_DATA),

        GNOMEUIINFO_MENU_COPY_ITEM(edit_copy_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_PASTE_ITEM(edit_paste_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_SELECT_ALL_ITEM(edit_selall_cb, (gpointer) GE_DATA),


	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Find _Line..."),
	  N_("Search for a line"),
	  goto_line_cb, (gpointer) GE_WINDOW, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },

	GNOMEUIINFO_MENU_FIND_ITEM(search_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_FIND_AGAIN_ITEM(search_again_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_REPLACE_ITEM(search_replace_cb, (gpointer) GE_DATA),
	
	GNOMEUIINFO_END
};	

GnomeUIInfo gedit_tab_menu []= {
	{ GNOME_APP_UI_ITEM, N_("_Top"),
	  N_("Put the document tabs at the top"),
	  tab_top_cb, (gpointer) GE_WINDOW, NULL },

	{ GNOME_APP_UI_ITEM, N_("_Bottom"),
	  N_("Put the document tabs at the bottom"),
	  tab_bot_cb, (gpointer) GE_WINDOW, NULL },

	{ GNOME_APP_UI_ITEM, N_("_Left"),
	  N_("Put the document tabs on the left"),
	  tab_lef_cb, (gpointer) GE_WINDOW, NULL },

	{ GNOME_APP_UI_ITEM, N_("_Right"),
	  N_("Put the document tabs on the right"),
	  tab_rgt_cb, (gpointer) GE_WINDOW, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SHOWTABS,
			    N_("Toggle the presence of the document tabs"),
				    tab_toggle_cb, (gpointer) GE_WINDOW, NULL),
	
	GNOMEUIINFO_END
};




GnomeUIInfo gedit_settings_menu []= {
        GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_AUTOINDENT,
				    N_("Toggle autoindent"),
				    auto_indent_toggle_cb, (gpointer) GE_DATA,
				    NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_STATUSBAR,
				    N_("Toggle statusbar"),
				    options_toggle_status_bar_cb,
				    (gpointer) GE_WINDOW, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_WORDWRAP,
				    N_("Toggle Wordwrap"),
				    options_toggle_word_wrap_cb,
				    (gpointer) GE_WINDOW, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_LINEWRAP,
				    N_("Toggle Linewrap"),
				    options_toggle_line_wrap_cb,
				    (gpointer) GE_WINDOW, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_READONLY,
				    N_("Toggle Readonly"),
				    options_toggle_read_only_cb,
				    (gpointer) GE_WINDOW, NULL),

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SPLITSCREEN,
				    N_("Toggle Split Screen"),
				    options_toggle_split_screen_cb,
				    (gpointer) GE_WINDOW, NULL),

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_SUBTREE, N_("_Document Tabs"),
	  N_("Change the placement of the document tabs"), &gedit_tab_menu },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Sa_ve Settings"),
	  N_("Save the current settings for future sessions"),
	  gE_save_settings, (gpointer) GE_WINDOW, NULL },

	GNOMEUIINFO_SEPARATOR, 

	GNOMEUIINFO_MENU_PREFERENCES_ITEM(gE_prefs_dialog, (gpointer) GE_DATA),

	GNOMEUIINFO_END
};

GnomeUIInfo gedit_window_menu []={
        GNOMEUIINFO_MENU_NEW_WINDOW_ITEM(window_new_cb, (gpointer) GE_DATA),

        GNOMEUIINFO_MENU_CLOSE_WINDOW_ITEM(window_close_cb,
					   (gpointer) GE_DATA),
	
	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("_Document List"),
	  N_("Display the document list"),
	  files_list_popup, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 'L', GDK_CONTROL_MASK, NULL },

	GNOMEUIINFO_END
};

GnomeUIInfo gedit_help_menu []= {

	GNOMEUIINFO_HELP ("gedit"),

	GNOMEUIINFO_MENU_ABOUT_ITEM(gE_about_box, NULL),

	GNOMEUIINFO_END
	
};


#if PLUGIN_TEST
GnomeUIInfo gedit_plugins_menu []= {
  { GNOME_APP_UI_ENDOFINFO}
};
#endif

GnomeUIInfo gedit_menu [] = {
        GNOMEUIINFO_MENU_FILE_TREE(gedit_file_menu),

	GNOMEUIINFO_MENU_EDIT_TREE(gedit_edit_menu),


#if PLUGIN_TEST
	{ GNOME_APP_UI_SUBTREE, N_("_Plugins"), NULL, &gedit_plugins_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#endif

	GNOMEUIINFO_MENU_SETTINGS_TREE(gedit_settings_menu),

	GNOMEUIINFO_MENU_WINDOWS_TREE(gedit_window_menu),

	GNOMEUIINFO_MENU_HELP_TREE(gedit_help_menu),

	GNOMEUIINFO_END
};

GnomeUIInfo * gE_menus_init (gE_window *window, gE_data *data)
{
	add_callback_data (gedit_file_menu, window, data);
	add_callback_data (gedit_edit_menu, window, data);
	add_callback_data (gedit_tab_menu, window, data);
	add_callback_data (gedit_settings_menu, window, data);
	add_callback_data (gedit_window_menu, window, data);
	add_callback_data (gedit_help_menu, window, data);

	gnome_app_create_menus (GNOME_APP (window->window), gedit_menu);

	remove_callback_data (gedit_file_menu, window, data);
	remove_callback_data (gedit_edit_menu, window, data);
	remove_callback_data (gedit_tab_menu, window, data);
	remove_callback_data (gedit_settings_menu, window, data);
	remove_callback_data (gedit_window_menu, window, data);
	remove_callback_data (gedit_help_menu, window, data);

	return (GnomeUIInfo *) gedit_menu;
}

/*
 * This function initializes the toggle menu items to the proper states.
 * It is called from gE_window_nwe after the settings have been loaded.
 */
void
gE_set_menu_toggle_states(gE_window *w)
{
  gE_document *doc = gE_document_current (w);
  int i;

#define GE_SET_TOGGLE_STATE(item, l, boolean)                            \
  if (!strcmp(item.label, l))                                            \
    GTK_CHECK_MENU_ITEM (item.widget)->active = boolean

  /*
   * Initialize the states of the document tabs menu...
   */
  for (i = 0; gedit_tab_menu[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      if (gedit_tab_menu[i].label)
	{
	  GE_SET_TOGGLE_STATE(gedit_tab_menu[i], GE_TOGGLE_LABEL_SHOWTABS,
			      w->show_tabs);
	  
	}
    }


  /*
   * The settings menu...
   */
  for (i = 0; gedit_settings_menu[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      if (gedit_settings_menu[i].label)
	{
	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_AUTOINDENT,
			      w->auto_indent);

	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_STATUSBAR,
			      w->show_status);

	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_WORDWRAP,
			      doc->word_wrap);

	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_LINEWRAP,
			      doc->line_wrap);

	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_READONLY,
			      doc->read_only);

	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_SPLITSCREEN,
			      doc->window->splitscreen);

	}
    }
}

void add_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data)
{
	int i = 0;

	while (menu[i].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (menu[i].user_data == (gpointer)GE_DATA)
			menu[i].user_data = data;
		if (menu[i].user_data == (gpointer)GE_WINDOW)
			menu[i].user_data = window;
		i++;
	}
}

void remove_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data)
{
	int i = 0;
	while (menu[i].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (menu[i].user_data == data)
			menu[i].user_data = (gpointer) GE_DATA;
		if (menu[i].user_data == window)
			menu[i].user_data = (gpointer) GE_WINDOW;
		i++;
	}
}
