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

#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#define WITH_FOOT
#include "xpm/foot.xpm"
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
#include "msgbox.h"
#include "search.h"


#define GE_DATA		1
#define GE_WINDOW	2

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
	{"<Main>/File/Recent Documents/", NULL, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
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
	{"<Main>/Search/Search...", NULL,
		search_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search and Replace...", NULL,
		search_replace_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search Again", NULL,
		search_again_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Options/Text Font...", NULL,
		prefs_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toggle Autoindent", NULL,
		auto_indent_toggle_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Options/Toggle Statusbar", NULL,
		gE_window_toggle_statusbar, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toggle Wordwrap", NULL,
		gE_document_toggle_wordwrap, (gpointer)GE_WINDOW, NULL},
#ifdef GTK_HAVE_FEATURES_1_1_0
	{"<Main>/Options/Toggle Split Screen", NULL,
		options_toggle_split_screen, (gpointer)GE_WINDOW, NULL},
#endif
#ifndef WITHOUT_GNOME
	{"<Main>/Options/Toggle Scrollball", NULL,
		gE_document_toggle_scrollball, (gpointer) GE_WINDOW, NULL},
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
	{"<Main>/Options/Toolbar/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toolbar/Tooltips On", NULL,
		tb_tooltips_on_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Toolbar/Tooltips Off", NULL,
		tb_tooltips_off_cb, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Plugins/", NULL, NULL, NULL},
	{"<Main>/Window/New Window", NULL,
		window_new_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Window/Close Window", NULL,
		window_close_cb, (gpointer)GE_DATA, NULL},
	{"<Main>/Window/<separator>", NULL,
		NULL, NULL },
	{"<Main>/Window/Document List", "<control>L",
		files_list_popup, (gpointer)GE_DATA, NULL},
	{"<Main>/Window/Message Box", NULL,
		msgbox_show, NULL, NULL},
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

GnomeUIInfo gedit_program_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("About..."), NULL, gE_about_box, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT },
	{ GNOME_APP_UI_ITEM, N_("Preferences..."), NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF },
	{ GNOME_APP_UI_SEPARATOR },
        { GNOME_APP_UI_ITEM, N_("Quit"),  NULL, file_quit_cb, (gpointer) GE_DATA, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
          'Q', GDK_CONTROL_MASK, NULL },
        GNOMEUIINFO_END
};

GnomeUIInfo gedit_file_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New"),  NULL, file_new_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
          'N', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Open"),  NULL, file_open_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	  'O', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save"),  NULL, file_save_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	  'S', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save as"),  NULL, file_save_as_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS },
	{ GNOME_APP_UI_ITEM, N_("Print"),  NULL, file_print_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT },	
	{ GNOME_APP_UI_ITEM, N_("Close"),  NULL, file_close_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	  'W', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Close All"), NULL, file_close_all_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE },

	{ GNOME_APP_UI_SEPARATOR },

#if 0
	{ GNOME_APP_UI_ITEM, N_("Quit"),  NULL, file_quit_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	  'Q', GDK_CONTROL_MASK, NULL },
#endif

	GNOMEUIINFO_END
};


GnomeUIInfo gedit_edit_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Cut"),  NULL, edit_cut_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
	  'X', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Copy"),  NULL, edit_copy_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
	  'C', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Paste"),  NULL, edit_paste_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
	  'V', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Select All"),  NULL, edit_selall_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL },
	GNOMEUIINFO_END
};	

GnomeUIInfo gedit_search_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Search"),  NULL, search_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_ITEM, N_("Search and Replace"),  NULL, search_replace_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Search Again"),  NULL, search_again_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_tab_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Top"),     NULL, tab_top_cb, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Bottom"),  NULL, tab_bot_cb, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Left"),    NULL, tab_lef_cb, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Right"),   NULL, tab_rgt_cb, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle"),   NULL, tab_toggle_cb, (gpointer) GE_WINDOW, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_toolbar_menu []= {
	{GNOME_APP_UI_ITEM, N_("Show Toolbar"), NULL, tb_on_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_ITEM, N_("Hide Toolbar"), NULL, tb_off_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Pictures and Text"), NULL, tb_pic_text_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_ITEM, N_("Pictures only"), NULL, tb_pic_only_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_ITEM, N_("Text only"), NULL, tb_text_only_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Tooltips On"), NULL, tb_tooltips_on_cb, (gpointer) GE_WINDOW, NULL },
	{GNOME_APP_UI_ITEM, N_("Tooltips Off"), NULL, tb_tooltips_off_cb, (gpointer) GE_WINDOW, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_options_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Text Font..."),  NULL, prefs_cb, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle Autoindent"),  NULL, auto_indent_toggle_cb, (gpointer) GE_DATA, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Statusbar"),  NULL, gE_window_toggle_statusbar, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Wordwrap"),  NULL, gE_document_toggle_wordwrap, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Split Screen"), NULL, options_toggle_split_screen, (gpointer) GE_WINDOW, NULL },
#ifndef WITHOUT_GNOME
	{ GNOME_APP_UI_ITEM, N_("Toggle Scrollball"), NULL, gE_document_toggle_scrollball, (gpointer) GE_WINDOW, NULL },
#endif
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_SUBTREE, N_("Document Tabs"), NULL, &gedit_tab_menu },
	{ GNOME_APP_UI_SUBTREE, N_("Toolbar"), NULL, &gedit_toolbar_menu },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Save Settings"),  NULL, gE_save_settings, (gpointer) GE_WINDOW, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_window_menu []={
        { GNOME_APP_UI_ITEM, N_("New Window"), NULL, window_new_cb, (gpointer) GE_DATA, NULL,
          GNOME_APP_PIXMAP_NONE, NULL },
	{ GNOME_APP_UI_ITEM, N_("Close Window"), NULL, window_close_cb, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Document List"), NULL, files_list_popup, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL,
	  'L', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Message Box"), NULL, msgbox_show, NULL, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

#if 0
GnomeUIInfo gedit_help_menu []= {
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}, 
	
	{GNOME_APP_UI_ITEM, N_("About..."), NULL, gE_about_box, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
	
};

#endif

#if PLUGIN_TEST
GnomeUIInfo gedit_plugins_menu []= {
  { GNOME_APP_UI_ENDOFINFO}
};
#endif

GnomeUIInfo gedit_menu [] = {
	#ifdef WITH_FOOT
	{ GNOME_APP_UI_SUBTREE, "", NULL, &gedit_program_menu, NULL, NULL,
		GNOME_APP_PIXMAP_DATA, foot_xpm, 0, 0, NULL },
	#else
	{ GNOME_APP_UI_SUBTREE, N_("Program"), NULL, &gedit_program_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	#endif

	{ GNOME_APP_UI_SUBTREE, N_("File"), NULL, &gedit_file_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Edit"), NULL, &gedit_edit_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Search"), NULL, &gedit_search_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Options"), NULL, &gedit_options_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#if PLUGIN_TEST
	{ GNOME_APP_UI_SUBTREE, N_("Plugins"), NULL, &gedit_plugins_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#endif
	{ GNOME_APP_UI_SUBTREE, N_("Window"), NULL, &gedit_window_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#if 0
	{ GNOME_APP_UI_SUBTREE, N_("Help"), NULL, &gedit_help_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
#endif
	GNOMEUIINFO_END
};

void gE_menus_init (gE_window *window, gE_data *data)
{
	add_callback_data (gedit_program_menu, window, data);
	add_callback_data (gedit_file_menu, window, data);
	add_callback_data (gedit_edit_menu, window, data);
	add_callback_data (gedit_search_menu, window, data);
	add_callback_data (gedit_tab_menu, window, data);
	add_callback_data (gedit_toolbar_menu, window, data);
	add_callback_data (gedit_options_menu, window, data);
	add_callback_data (gedit_window_menu, window, data);
#if 0
	add_callback_data (gedit_help_menu, window, data);
#endif

	gnome_app_create_menus (GNOME_APP (window->window), gedit_menu);

	remove_callback_data (gedit_program_menu, window, data);
	remove_callback_data (gedit_file_menu, window, data);
	remove_callback_data (gedit_edit_menu, window, data);
	remove_callback_data (gedit_search_menu, window, data);
	remove_callback_data (gedit_tab_menu, window, data);
	remove_callback_data (gedit_toolbar_menu, window, data);
	remove_callback_data (gedit_options_menu, window, data);
	remove_callback_data (gedit_window_menu, window, data);
#if 0
	remove_callback_data (gedit_help_menu, window, data);
#endif

	/* Decrease the padding along the menubar */
	gtk_container_border_width (GTK_CONTAINER (
		GTK_WIDGET (GTK_WIDGET (GNOME_APP (window->window)->menubar)->parent)->parent), 0);
}


void add_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data)
{
	int i = 0;

	while (menu[i].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (menu[i].type == GNOME_APP_UI_ITEM)
		{
			if (menu[i].user_data == (gpointer)GE_DATA)
				menu[i].user_data = data;
			if (menu[i].user_data == (gpointer)GE_WINDOW)
				menu[i].user_data = window;
		}
		i++;
	}
}

void remove_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data)
{
	int i = 0;
	while (menu[i].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (menu[i].type == GNOME_APP_UI_ITEM)
		{
			if (menu[i].user_data == data)
				menu[i].user_data = (gpointer) GE_DATA;
			if (menu[i].user_data == window)
				menu[i].user_data = (gpointer) GE_WINDOW;
		}
		i++;
	}
}

#endif	/* #ifdef WITHOUT_GNOME */
