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

#ifndef WITHOUT_GNOME
#include <config.h>
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
#include "msgbox.h"
#include "search.h"


#define GE_DATA		1
#define GE_WINDOW	2

#ifdef WITHOUT_GNOME
static void menus_init(gE_window *window, gE_data *data);
static void menus_create(GtkMenuEntry * entries, int nmenu_entries);

static GtkMenuFactory *factory = NULL;
static GtkMenuFactory *subfactory;
static GHashTable *entry_ht = NULL;


static GtkMenuEntry menu_items[] =
{
	{"<Main>/File/New", "<control>N",
		file_new_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Open", "<control>O",
		file_open_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Save", "<control>S",
		file_save_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Save As...", NULL,
		file_save_as_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Print", NULL,
		file_print_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Close", "<control>W",
		file_close_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/Close All", NULL,
		file_close_all_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
	{"<Main>/File/Recent Documents/", NULL, NULL},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
	{"<Main>/File/Quit", "<control>Q",
		file_quit_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/Cut", "<control>X",
		edit_cut_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/Copy", "<control>C",
		edit_copy_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/Paste", "<control>V",
		edit_paste_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Edit/<separator>", NULL, NULL, NULL},
	{"<Main>/Edit/Select All", "<control>A",
		edit_selall_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search...", NULL,
		search_search_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search and Replace...", NULL,
		search_replace_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Search/Search Again", NULL,
		search_again_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Options/Text Font...", NULL,
		prefs_callback, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toggle Autoindent", NULL,
		auto_indent_toggle_callback, (gpointer)GE_DATA, NULL},
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
		tab_top_cback, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/Bottom", NULL,
		tab_bot_cback, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/Left", NULL,
		tab_lef_cback, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/Right", NULL,
		tab_rgt_cback, (gpointer)GE_WINDOW, NULL},
	{"<Main>/Options/Document Tabs/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs/Toggle", NULL,
		tab_toggle_cback, (gpointer)GE_WINDOW, NULL},
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
		file_newwindow_cmd_callback, (gpointer)GE_DATA, NULL},
	{"<Main>/Window/Close Window", NULL,
		file_close_window_cmd_callback, (gpointer)GE_DATA, NULL},
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
	GtkWidget **menubar;
#ifdef GTK_HAVE_FEATURES_1_1_0
	GtkAccelGroup **accel;
#else
	GtkAcceleratorTable **accel;
#endif

	menubar = &window->menubar;
	accel = &window->accel;

	menus_init (window, data);

	if (menubar)
	{
		*menubar = subfactory->widget;
		window->factory = subfactory;
	}
#ifdef GTK_HAVE_FEATURES_1_1_0
	if(accel)
		*accel = subfactory->accel_group;
#else
	if(accel)
		*accel = subfactory->table;
#endif
}


static void
menus_init(gE_window *window, gE_data *data)
{
	int i;
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
	subfactory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
	
	gtk_menu_factory_add_subfactory(factory, subfactory, "<Main>");
	menus_create(menu_items, nmenu_items);

	for (i = 0; i < nmenu_items; i++) {
		if (menu_items[i].callback_data == (gpointer) data)
			menu_items[i].callback_data = (gpointer) GE_DATA;
		if (menu_items[i].callback_data == (gpointer) window)
			menu_items[i].callback_data = (gpointer) GE_WINDOW;
	}

}
	    
static void
menus_create(GtkMenuEntry * entries, int nmenu_entries)
{
	GtkMenuPath *mpath;
	char *accelerator;
	int i;

	if (entry_ht)
		for (i = 0; i < nmenu_entries; i++) {
			accelerator = g_hash_table_lookup(entry_ht, entries[i].path);
			if (accelerator) {
				if (accelerator[0] == '\0')  
					entries[i].accelerator = NULL;
				else
					entries[i].accelerator = accelerator;
			}
		}
	gtk_menu_factory_add_entries(factory, entries, nmenu_entries);

	/*   for (i = 0; i < nmenu_entries; i++)
		 if (entries[i].widget) {
		 gtk_signal_connect(GTK_OBJECT(entries[i].widget), "install_accelerator",
		 (GtkSignalFunc) menus_install_accel,
		 entries[i].path);
		 gtk_signal_connect(GTK_OBJECT(entries[i].widget), "remove_accelerator",
		 (GtkSignalFunc) menus_remove_accel,
		 entries[i].path);
		 }
	 */

	mpath = gtk_menu_factory_find(factory, "<Main>/Help");
	gtk_menu_item_right_justify(GTK_MENU_ITEM(mpath->widget));
} /* menus_create */

#if 0
static gint menus_install_accel( GtkWidget * widget, gchar * signal_name, gchar key, char modifiers, gchar * path)
{
   char accel[64];
   char *t1, t2[2];
   
   accel[0] = '\0';
   if (modifiers & GDK_CONTROL_MASK)
     strcat(accel, "<control>");
   if (modifiers & GDK_SHIFT_MASK)
     strcat(accel, "<shift>");
   if (modifiers & GDK_MOD1_MASK)
     strcat(accel, "<alt>");
   
   t2[0] = key;
   t2[1] = '\0';
   strcat(accel, t2);
   
   if (entry_ht) {
      t1 = g_hash_table_lookup(entry_ht, path);
      g_free(t1);
   } else
     entry_ht = g_hash_table_new(g_str_hash, g_str_equal);
  
   g_hash_table_insert(entry_ht, path, g_strdup(accel));
   
   return TRUE;
   
}

static void menus_remove_accel(GtkWidget * widget, gchar * signal_name, gchar * path)
{
   char *t;
   
   if (entry_ht) {
      t = g_hash_table_lookup(entry_ht, path);
      g_free(t);
      
      g_hash_table_insert(entry_ht, path, g_strdup(""));
   }
}

void menus_set_sensitive(char *path, int sensitive)
{
   GtkMenuPath *menu_path;
   
   menu_path = gtk_menu_factory_find(factory, path);
   if (menu_path)
     gtk_widget_set_sensitive(menu_path->widget, sensitive);
   else
     g_warning("Unable to set sensitivity for menu which doesn't exist: %s", path);
}
#endif

#else	/* USING GNOME */

void add_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data);
void remove_callback_data (GnomeUIInfo *menu, gE_window *window, gE_data *data);

GnomeUIInfo gedit_program_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("About..."), NULL, gE_about_box, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT },
	{ GNOME_APP_UI_ITEM, N_("Preferences..."), NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF },
	{ GNOME_APP_UI_SEPARATOR },
        { GNOME_APP_UI_ITEM, N_("Quit"),  NULL, file_quit_cmd_callback, (gpointer) GE_DATA, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
          'Q', GDK_CONTROL_MASK, NULL },
        GNOMEUIINFO_END
};

GnomeUIInfo gedit_file_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New"),  NULL, file_new_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
          'N', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Open"),  NULL, file_open_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	  'O', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save"),  NULL, file_save_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	  'S', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save as"),  NULL, file_save_as_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS },
	{ GNOME_APP_UI_ITEM, N_("Print"),  NULL, file_print_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT },	
	{ GNOME_APP_UI_ITEM, N_("Close"),  NULL, file_close_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	  'W', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Close All"), NULL, file_close_all_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE },

	{ GNOME_APP_UI_SEPARATOR },

#if 0
	{ GNOME_APP_UI_ITEM, N_("Quit"),  NULL, file_quit_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	  'Q', GDK_CONTROL_MASK, NULL },
#endif

	GNOMEUIINFO_END
};


GnomeUIInfo gedit_edit_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Cut"),  NULL, edit_cut_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
	  'X', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Copy"),  NULL, edit_copy_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
	  'C', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Paste"),  NULL, edit_paste_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
	  'V', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Select All"),  NULL, edit_selall_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL },
	GNOMEUIINFO_END
};	

GnomeUIInfo gedit_search_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Search"),  NULL, search_search_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_ITEM, N_("Search and Replace"),  NULL, search_replace_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Search Again"),  NULL, search_again_cmd_callback, (gpointer) GE_DATA, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_tab_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Top"),     NULL, tab_top_cback, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Bottom"),  NULL, tab_bot_cback, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Left"),    NULL, tab_lef_cback, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_ITEM, N_("Right"),   NULL, tab_rgt_cback, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle"),   NULL, tab_toggle_cback, (gpointer) GE_WINDOW, NULL },
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
	{ GNOME_APP_UI_ITEM, N_("Text Font..."),  NULL, prefs_callback, (gpointer) GE_WINDOW, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle Autoindent"),  NULL, auto_indent_toggle_callback, (gpointer) GE_DATA, NULL },
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
        { GNOME_APP_UI_ITEM, N_("New Window"), NULL, file_newwindow_cmd_callback, (gpointer) GE_DATA, NULL,
          GNOME_APP_PIXMAP_NONE, NULL },
	{ GNOME_APP_UI_ITEM, N_("Close Window"), NULL, file_close_window_cmd_callback, (gpointer) GE_DATA, NULL,
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
