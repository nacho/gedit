/* gEdit Menus Definition */


#include <gtk/gtk.h>
#include <strings.h>

#include "main.h"

static void menus_remove_accel(GtkWidget * widget, gchar * signal_name, gchar * path);
static gint menus_install_accel(GtkWidget * widget, gchar * signal_name, gchar key, gchar modifiers, gchar * path);
void menus_init(void);
void menus_create(GtkMenuEntry * entries, int nmenu_entries);

static GtkMenuEntry menu_items[] =
{
     {"<Main>/File/New", "<control>N", file_new_cmd_callback, NULL},
     {"<Main>/File/Open", "<control>O", file_open_cmd_callback, NULL},
     {"<Main>/File/Save", "<control>S", file_save_cmd_callback, NULL},
     {"<Main>/File/Save As...", NULL, file_save_as_cmd_callback, NULL},
     {"<Main>/File/Close", "<control>W", file_close_cmd_callback, NULL,NULL},
     {"<Main>/File/<separator>", NULL, NULL, NULL},
     {"<Main>/File/Print", NULL, file_print_cmd_callback, NULL,NULL},     
     {"<Main>/File/<separator>", NULL, NULL, NULL},
     {"<Main>/File/Quit", "<control>Q", file_quit_cmd_callback, NULL},
     {"<Main>/Edit/Cut", "<control>X", edit_cut_cmd_callback, NULL},
     {"<Main>/Edit/Copy", "<control>C", edit_copy_cmd_callback, NULL},
     {"<Main>/Edit/Paste", "<control>V", edit_paste_cmd_callback, NULL},
     {"<Main>/Edit/<separator>", NULL, NULL, NULL},
     {"<Main>/Edit/Select All", "<control>A", edit_selall_cmd_callback, NULL},
     {"<Main>/Search/Search...", NULL, search_search_cmd_callback, NULL},
     {"<Main>/Search/Search and Replace...", NULL, search_replace_cmd_callback, NULL},
     {"<Main>/Search/Search Again", NULL, search_again_cmd_callback, NULL},
     {"<Main>/Options/Text Font...", NULL, prefs_callback, NULL},
     {"<Main>/Options/<separator>", NULL, NULL, NULL},
     {"<Main>/Options/Word Wrap", NULL, NULL, NULL},
     {"<Main>/Options/Syntax Highlighting", NULL, NULL, NULL},
     {"<Main>/Options/Auto Indent", NULL, NULL, NULL},
     {"<Main>/Options/<separator>", NULL, NULL, NULL},
     {"<Main>/Options/Tab Location", NULL, NULL, NULL},
     {"<Main>/Options/Tab Location/Top", NULL, tab_top_cback, NULL},
     {"<Main>/Options/Tab Location/Bottom", NULL, tab_bot_cback, NULL},
     {"<Main>/Options/Tab Location/Left", NULL, tab_lef_cback, NULL},
     {"<Main>/Options/Tab Location/Right", NULL, tab_rgt_cback, NULL},
     {"<Main>/Help/About", "<control>H", gE_about_box, NULL}
};

static int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

static int initialize = TRUE;
static GtkMenuFactory *factory = NULL;
static GtkMenuFactory *subfactory[1];
static GHashTable *entry_ht = NULL;

void get_main_menu(GtkWidget ** menubar, GtkAcceleratorTable ** table)
{
    if (initialize)
    	    menus_init();
    
    if (menubar)
            *menubar = subfactory[0]->widget;
    if(table)
       	    *table = subfactory[0]->table;
}

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
	      
