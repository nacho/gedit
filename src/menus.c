/* gEdit Menus Definition */


#include <gtk/gtk.h>
#include <strings.h>

#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif

#define PLUGIN_TEST
#include "main.h"
#include "toolbar.h"

static void menus_remove_accel(GtkWidget * widget, gchar * signal_name, gchar * path);
static gint menus_install_accel(GtkWidget * widget, gchar * signal_name, gchar key, gchar modifiers, gchar * path);
void menus_init(void);
void menus_create(GtkMenuEntry * entries, int nmenu_entries);

static int initialize = TRUE;
static GtkMenuFactory *factory = NULL;
static GtkMenuFactory *subfactory[1];
static GHashTable *entry_ht = NULL;

void gE_menus_init (gE_window *window, gE_data *data)
{


#ifndef WITHOUT_GNOME
GnomeUIInfo gedit_file_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New"),  NULL, file_new_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
          'N', GDK_CONTROL_MASK, NULL },
        { GNOME_APP_UI_ITEM, N_("New Window"), NULL, file_newwindow_cmd_callback, data, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW },
	{ GNOME_APP_UI_ITEM, N_("Open"),  NULL, file_open_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	  'O', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save"),  NULL, file_save_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	  'S', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Save as"),  NULL, file_save_as_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS },
	{ GNOME_APP_UI_ITEM, N_("Print"),  NULL, file_print_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT },	
	{ GNOME_APP_UI_ITEM, N_("Close"),  NULL, file_close_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	  'W', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Close Window"), NULL, file_close_window_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Quit"),  NULL, file_quit_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
	  'Q', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_edit_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Cut"),  NULL, edit_cut_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
	  'X', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Copy"),  NULL, edit_copy_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
	  'C', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Paste"),  NULL, edit_paste_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
	  'V', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Select All"),  NULL, edit_selall_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK },
	GNOMEUIINFO_END
};	

GnomeUIInfo gedit_search_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Search"),  NULL, search_search_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_ITEM, N_("Search and Replace"),  NULL, search_replace_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Search Again"),  NULL, search_again_cmd_callback, data, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_tab_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Top"),     NULL, tab_top_cback, window, NULL },
	{ GNOME_APP_UI_ITEM, N_("Bottom"),  NULL, tab_bot_cback, window, NULL },
	{ GNOME_APP_UI_ITEM, N_("Left"),    NULL, tab_lef_cback, window, NULL },
	{ GNOME_APP_UI_ITEM, N_("Right"),   NULL, tab_rgt_cback, window, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle"),   NULL, tab_toggle_cback, window, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_toolbar_menu []= {
	{GNOME_APP_UI_ITEM, N_("Show Toolbar"), NULL, tb_on_cb, window, NULL },
	{GNOME_APP_UI_ITEM, N_("Hide Toolbar"), NULL, tb_off_cb, window, NULL },
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Pictures and Text"), NULL, tb_pic_text_cb, window, NULL },
	{GNOME_APP_UI_ITEM, N_("Pictures only"), NULL, tb_pic_only_cb, window, NULL },
	{GNOME_APP_UI_ITEM, N_("Text only"), NULL, tb_text_only_cb, window, NULL },
	{GNOME_APP_UI_SEPARATOR},
	{GNOME_APP_UI_ITEM, N_("Tooltips On"), NULL, tb_tooltips_on_cb, window, NULL },
	{GNOME_APP_UI_ITEM, N_("Tooltips Off"), NULL, tb_tooltips_off_cb, window, NULL },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_options_menu []= {
	{ GNOME_APP_UI_ITEM, N_("Text Font..."),  NULL, prefs_callback, window, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Toggle Autoindent"),  NULL, auto_indent_toggle_callback, data, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Statusbar"),  NULL, gE_window_toggle_statusbar, window, NULL },
	{ GNOME_APP_UI_ITEM, N_("Toggle Wordwrap"),  NULL, gE_document_toggle_wordwrap, window, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_SUBTREE, N_("Document Tabs"), NULL, &gedit_tab_menu },
	{ GNOME_APP_UI_SUBTREE, N_("Toolbar"), NULL, &gedit_toolbar_menu },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Save Settings"),  NULL, gE_save_settings },
	GNOMEUIINFO_END
};

GnomeUIInfo gedit_help_menu []= {
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}, 
	
	{GNOME_APP_UI_ITEM, N_("About..."), NULL, gE_about_box, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
	
	{GNOME_APP_UI_ENDOFINFO}
	
	/*
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}, 
		
	{ GNOME_APP_UI_ITEM, N_("About"), NULL, gE_about_box, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT },
	{ GNOME_APP_UI_SEPARATOR },
	GNOMEUIINFO_HELP ("gedit"),
	GNOMEUIINFO_END */
};

#if PLUGIN_TEST
GnomeUIInfo gedit_plugins_menu []= {
  { GNOME_APP_UI_ITEM, N_("Diff"), NULL, start_diff, data, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
  { GNOME_APP_UI_ITEM, N_("CVS Diff"), NULL, start_cvsdiff, data, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
  { GNOME_APP_UI_ITEM, N_("Reverse"), NULL, start_reverse, data, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
  { GNOME_APP_UI_ENDOFINFO}
};
#endif

GnomeUIInfo gedit_menu [] = {
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
	{ GNOME_APP_UI_SUBTREE, N_("Help"), NULL, &gedit_help_menu, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	GNOMEUIINFO_END
};

#else

GtkMenuEntry menu_items[] =
{
	{"<Main>/File/New", "<control>N", file_new_cmd_callback, data},
	{"<Main>/File/New Window", NULL, file_newwindow_cmd_callback, data},
	{"<Main>/File/Open", "<control>O", file_open_cmd_callback, data},
	{"<Main>/File/Save", "<control>S", file_save_cmd_callback, data},
	{"<Main>/File/Save As...", NULL, file_save_as_cmd_callback, data},
	{"<Main>/File/Close", "<control>W", file_close_cmd_callback, data},
	{"<Main>/File/Close Window", NULL, file_close_window_cmd_callback, data},
	{"<Main>/File/<separator>", NULL, NULL, NULL},
	{"<Main>/File/Print", NULL, file_print_cmd_callback, data,NULL},     
	{"<Main>/File/<separator>", NULL, NULL, NULL},
	{"<Main>/File/Quit", "<control>Q", file_quit_cmd_callback, data},
	{"<Main>/Edit/Cut", "<control>X", edit_cut_cmd_callback, NULL},
	{"<Main>/Edit/Copy", "<control>C", edit_copy_cmd_callback, NULL},
	{"<Main>/Edit/Paste", "<control>V", edit_paste_cmd_callback, NULL},
	{"<Main>/Edit/<separator>", NULL, NULL, NULL},
	{"<Main>/Edit/Select All", "<control>A", edit_selall_cmd_callback, NULL},
	{"<Main>/Search/Search...", NULL, search_search_cmd_callback, NULL},
	{"<Main>/Search/Search and Replace...", NULL, search_replace_cmd_callback, NULL},
	{"<Main>/Search/Search Again", NULL, search_again_cmd_callback, NULL},
	{"<Main>/Options/Text Font...", NULL, prefs_callback, window},
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toggle Autoindent", NULL, auto_indent_toggle_callback, data},
	{"<Main>/Options/Toggle Statusbar", NULL, gE_window_toggle_statusbar, window},
	{"<Main>/Options/Toggle Wordwrap", NULL, gE_document_toggle_wordwrap, window},
	{"<Main>/Options/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs/Top", NULL, tab_top_cback, window},
	{"<Main>/Options/Document Tabs/Bottom", NULL, tab_bot_cback, window},
	{"<Main>/Options/Document Tabs/Left", NULL, tab_lef_cback, window},
	{"<Main>/Options/Document Tabs/Right", NULL, tab_rgt_cback, window},
	{"<Main>/Options/Document Tabs/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Document Tabs/Toggle", NULL, tab_toggle_cback, window},
	{"<Main>/Options/Toolbar/Show Toolbar", NULL, tb_on_cb, window},
	{"<Main>/Options/Toolbar/Hide Toolbar", NULL, tb_off_cb, window},
	{"<Main>/Options/Toolbar/<separator>", NULL, NULL, window},
	{"<Main>/Options/Toolbar/Pictures and Text", NULL, tb_pic_text_cb, windowL},
	{"<Main>/Options/Toolbar/Pictures only", NULL, tb_pic_only_cb, window},
	{"<Main>/Options/Toolbar/Text only", NULL, tb_text_only_cb, window},
	{"<Main>/Options/Toolbar/<separator>", NULL, NULL, NULL},
	{"<Main>/Options/Toolbar/Tooltips On", NULL, tb_tooltips_on_cb, window},
	{"<Main>/Options/Toolbar/Tooltips Off", NULL, tb_tooltips_off_cb, window},
	#if PLUGINS_TEST
	{"<Main>/Plugins/Diff", NULL, start_diff, data},
	{"<Main>/Plugins/CVS Diff", NULL, start_cvsdiff, data},
	#endif
	{"<Main>/Help/About", "<control>H", gE_about_box, NULL}
};

int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
#endif

#ifndef WITHOUT_GNOME
	gnome_app_create_menus (GNOME_APP (window->window), gedit_menu);

#else

    if (initialize)
    	    menus_init();
    
    if (menubar)
            *menubar = subfactory[0]->widget;
#ifdef GTK_HAVE_ACCEL_GROUP
    if(window->accel)
       	    *window->accel = subfactory[0]->accel_group;
#else
    if(window->accel)
       	    *accel = subfactory[0]->table;
#endif

#endif
}

#ifdef WITHOUT_GNOME

void menus_init(void)
{

    if (initialize) {
        initialize = FALSE;
	
	factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
	subfactory[0] = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
	
	gtk_menu_factory_add_subfactory(factory, subfactory[0], "<Main>");
	menus_create(menu_items, nmenu_items);
    }
}
	    
void menus_create(GtkMenuEntry * entries, int nmenu_entries)
{
   char *accelerator;
   int i;
   
   if (initialize)
     menus_init();
   
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
   
   for (i = 0; i < nmenu_entries; i++)
           if (entries[i].widget) {
	       gtk_signal_connect(GTK_OBJECT(entries[i].widget), "install_accelerator",
				  (GtkSignalFunc) menus_install_accel,
				  entries[i].path);
	       gtk_signal_connect(GTK_OBJECT(entries[i].widget), "remove_accelerator",
				  (GtkSignalFunc) menus_remove_accel,
				  entries[i].path);
	   }
}

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
   
   if (initialize)
     menus_init();
   
   menu_path = gtk_menu_factory_find(factory, path);
   if (menu_path)
     gtk_widget_set_sensitive(menu_path->widget, sensitive);
   else
     g_warning("Unable to set sensitivity for menu which doesn't exist: %s", path);
}
	      
#endif
