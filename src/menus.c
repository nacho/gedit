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
#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif

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
#define GE_TOGGLE_LABEL_SCROLLBALL      N_("Scro_llball")
#define GE_TOGGLE_LABEL_SHOWTABS        N_("_Show tabs")
#define GE_TOGGLE_LABEL_TOOLBAR_RELIEF  N_("Toolbar _relief")
#define GE_TOGGLE_LABEL_TOOLTIPS        N_("_Show tooltips")
#define GE_TOGGLE_LABEL_TOOLBAR_TEXT    N_("Show toolbar _text")
#define GE_TOGGLE_LABEL_TOOLBAR_PIX     N_("Show toolbar _icons")

#ifdef WITHOUT_GNOME

static GtkMenuEntry menu_items[] =
{
	{"<Main>/File/New", "<control>N",
		file_new_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Open", "<control>O",
		file_open_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Save", "<control>S",
		file_save_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Save As...", NULL,
		file_save_as_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Print", NULL,
		file_print_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Close", "<control>W",
		file_close_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Close All", NULL,
		file_close_all_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
/*	{"<Main>/File/Recent Documents/", NULL, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},*/
	{"<Main>/File/Quit", "<control>Q",
		file_quit_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/Cut", "<control>X",
		edit_cut_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/Copy", "<control>C",
		edit_copy_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/Paste", "<control>V",
		edit_paste_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/<separator>", NULL, NULL, NULL},
	{"<Main>/Edit/Select All", "<control>A",
		edit_selall_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search For Text...", NULL,
		search_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search For Line...", NULL,
		goto_line_cb, (gpointer) GE_WINDOW, NULL},
	{"<Main>/Search/Search and Replace...", NULL,
		search_replace_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search Again", NULL,
		search_again_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Options/Preferences...", NULL,
		gE_prefs_dialog, (gpointer)GE_DATA, NULL},
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toggle Autoindent", NULL,
		auto_indent_toggle_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Options/Toggle Statusbar", NULL,
		options_toggle_status_bar_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toggle Wordwrap", NULL,
		options_toggle_word_wrap_cb, (gpointer)GE_WINDOW, NULL},
#ifdef GTK_HAVE_FEATURES_1_1_0	
	{"<Main>/Options/Toggle Linewrap", NULL,
		options_toggle_line_wrap_cb, (gpointer)GE_WINDOW, NULL},
#endif
	{"<Main>/Options/Toggle ReadOnly", NULL,
		options_toggle_read_only_cb, (gpointer)GE_WINDOW, NULL},
#ifdef GTK_HAVE_FEATURES_1_1_0
	{"<Main>/Options/Toggle Split Screen", NULL,
		options_toggle_split_screen_cb, (gpointer)GE_WINDOW, NULL},
#endif
#ifndef WITHOUT_GNOME
	{"<Main>/Options/Toggle Scrollball", NULL,
		options_toggle_scroll_ball_cb, (gpointer) GE_WINDOW, NULL},
#endif
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs/Top", NULL,
		tab_top_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/Bottom", NULL,
		tab_bot_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/Left", NULL,
		tab_lef_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/Right", NULL,
		tab_rgt_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs/Toggle", NULL,
		tab_toggle_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Show Toolbar", NULL,
		tb_on_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Hide Toolbar", NULL,
		tb_off_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/<separator>", NULL, NULL, NULL, NULL},
	{"<Main>/Options/Toolbar/Pictures and Text", NULL,
		tb_pic_text_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Pictures only", NULL,
		tb_pic_only_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Text only", NULL,
		tb_text_only_cb, (gpointer)GE_WINDOW, NULL},
#ifdef GTK_HAVE_FEATURES_1_1_0
	{"<Main>/Options/Toolbar/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toolbar/Toolbar Relief On", NULL,
		tb_relief_on, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Toolbar Relief Off", NULL,
		tb_relief_off, (gpointer)GE_WINDOW, NULL},
#endif
	{"<Main>/Options/Toolbar/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toolbar/Tooltips On", NULL,
		tb_tooltips_on_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Tooltips Off", NULL,
		tb_tooltips_off_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Save Settings", NULL, gE_save_settings, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Plugins/", NULL, NULL, NULL},
	{"<Main>/Window/New Window", NULL,
		window_new_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Window/Close Window", NULL,
		window_close_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Window/<separator>", NULL,
		NULL, NULL },
	{"<Main>/Window/Document List", "<control>L",
		files_list_popup, (gpointer)GE_DATA, NULL},
	{"<Main>/Help/About", "<control>H",
		gE_about_box, NULL, NULL}
};


/*
 * PUBLIC: gE_menus_init
 *
 * this is the only routine that should be global to the world
 */
void
gE_menus_init (gE_window *window, gE_data *data)
{
	int i;
	GtkMenuPath *mpath;
	GtkMenuFactory *factory;
	GtkMenuFactory *subfactory;
	int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

	for (i = 0; i < nmenu_items; i++) {
		switch ((long)(menu_items[i].callback_data)) {
		case GE_DATA :
			menu_items[i].callback_data = (gpointer)data;
			break;
		case GE_WINDOW :
			menu_items[i].callback_data = (gpointer)window;
			break;
		case (long)NULL :	/* do nothing */
			break;
		default:
			printf("unknown callback data specifier: %ld\n",
				(long)(menu_items[i].callback_data));
			exit(-1);
		}
	}

	factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
	window->factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
	subfactory = window->factory;
	gtk_menu_factory_add_subfactory(factory, subfactory, "<Main>");
	gtk_menu_factory_add_entries(factory, menu_items, nmenu_items);

	mpath = gtk_menu_factory_find(factory, "<Main>/Help");
	gtk_menu_item_right_justify(GTK_MENU_ITEM(mpath->widget));

	window->menubar = subfactory->widget;
#ifdef GTK_HAVE_FEATURES_1_1_0
	window->accel = subfactory->accel_group;
#else
	window->accel = subfactory->table;
#endif
	/*
	 * XXX - menu accelerators still don't work correctly (ever since
	 * having multiple windows capability).
	 */
#ifndef GTK_HAVE_FEATURES_1_1_0
	gtk_window_add_accelerator_table(GTK_WINDOW(window->window),
		subfactory->table);
#else
	gtk_window_add_accel_group(GTK_WINDOW(window->window),
		subfactory->accel_group);
#endif
	for (i = 0; i < nmenu_items; i++) {
		if (menu_items[i].callback_data == (gpointer)data)
			menu_items[i].callback_data = (gpointer)GE_DATA;
		if (menu_items[i].callback_data == (gpointer)window)
			menu_items[i].callback_data = (gpointer)GE_WINDOW;
	}

} /* gE_menus_init */
	    
#else	/* USING GNOME */

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

GnomeUIInfo gedit_toolbar_menu []= {
/*	{GNOME_APP_UI_ITEM, N_("Show Toolbar"), NULL, tb_on_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_ITEM, N_("Hide Toolbar"), NULL, tb_off_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_SEPARATOR},
*/
        GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_TOOLBAR_TEXT,
				    N_("Toggle display of toolbar text"),
				    tb_text_toggle_cb, (gpointer) GE_WINDOW,
				    NULL),

        GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_TOOLBAR_PIX,
				    N_("Toggle display of toolbar icons"),
				    tb_pix_toggle_cb, (gpointer) GE_WINDOW,
				    NULL),
#ifdef GTK_HAVE_FEATURES_1_1_0
	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_TOOLBAR_RELIEF,
					 N_("Toggle toolbar relief"),
					 tb_relief_toggle_cb,
					 (gpointer) GE_DATA, NULL),
#endif
	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_TOOLTIPS,
					 N_("Toggle tooltips"),
					 tb_tooltips_toggle_cb,
					 (gpointer) GE_WINDOW, NULL),
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

	GNOMEUIINFO_TOGGLEITEM_DATA(GE_TOGGLE_LABEL_SCROLLBALL,
				    N_("Toggle scrollball"),
				    options_toggle_scroll_ball_cb,
				    (gpointer) GE_WINDOW, NULL),

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_SUBTREE, N_("_Document Tabs"),
	  N_("Change the placement of the document tabs"), &gedit_tab_menu },
/*
	{ GNOME_APP_UI_SUBTREE, N_("_Toolbar"),
	  N_("Customize the toolbar"), &gedit_toolbar_menu },
*/
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
	add_callback_data (gedit_toolbar_menu, window, data);
	add_callback_data (gedit_settings_menu, window, data);
	add_callback_data (gedit_window_menu, window, data);
	add_callback_data (gedit_help_menu, window, data);

	gnome_app_create_menus (GNOME_APP (window->window), gedit_menu);

	remove_callback_data (gedit_file_menu, window, data);
	remove_callback_data (gedit_edit_menu, window, data);
	remove_callback_data (gedit_tab_menu, window, data);
	remove_callback_data (gedit_toolbar_menu, window, data);
	remove_callback_data (gedit_settings_menu, window, data);
	remove_callback_data (gedit_window_menu, window, data);
	remove_callback_data (gedit_help_menu, window, data);

	/* Decrease the padding along the menubar */
	/*gtk_container_border_width (GTK_CONTAINER (
		GTK_WIDGET (GTK_WIDGET (GNOME_APP (window->window)->menubar)->parent)->parent), 0);*/

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
   * The toolbar menu..
   */
/*  for (i = 0; gedit_toolbar_menu[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      if (gedit_toolbar_menu[i].label)
	{
	  GE_SET_TOGGLE_STATE(gedit_toolbar_menu[i],
			      GE_TOGGLE_LABEL_TOOLBAR_RELIEF,
			      w->use_relief_toolbar);

	  GE_SET_TOGGLE_STATE(gedit_toolbar_menu[i], GE_TOGGLE_LABEL_TOOLTIPS,
			      w->show_tooltips);

	  GE_SET_TOGGLE_STATE(gedit_toolbar_menu[i],
			      GE_TOGGLE_LABEL_TOOLBAR_TEXT,
			      w->have_tb_text);

	  GE_SET_TOGGLE_STATE(gedit_toolbar_menu[i],
			      GE_TOGGLE_LABEL_TOOLBAR_PIX,
			      w->have_tb_pix);
	}
    }
*/
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

	  GE_SET_TOGGLE_STATE(gedit_settings_menu[i],
			      GE_TOGGLE_LABEL_SCROLLBALL,
			      doc->window->scrollball);

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

#endif	/* #ifdef WITHOUT_GNOME */
