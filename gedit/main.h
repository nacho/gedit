#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For size_t */
#include <stdio.h>
#define PLUGIN_TEST 1
#if PLUGIN_TEST
#include "plugin.h"
#endif

#define STRING_LENGTH_MAX	256
#define GEDIT_ID	"gEdit 0.4.0"

typedef struct _gE_search {
	GtkWidget *window;
	GtkWidget *start_at_cursor;
	GtkWidget *start_at_beginning;
	GtkWidget *case_sensitive;
	GtkWidget *search_entry;
	gint replace, again, line;
	GtkWidget *replace_box;
	GtkWidget *replace_entry;
	GtkWidget *prompt_before_replacing;
	GtkWidget *search_for;
	GtkWidget *line_item, *text_item;
} gE_search;

typedef struct _gE_window {
	GtkWidget *window;
	GtkWidget *text;
	GtkWidget *statusbox;
	GtkWidget *statusbar;
	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *notebook;
	GtkWidget *open_fileselector;
	GtkWidget *save_fileselector;
	GtkWidget *line_label, *col_label;
#ifdef GTK_HAVE_ACCEL_GROUP
	GtkAccelGroup *accel;
#else
	GtkAcceleratorTable *accel;
#endif
	GList *documents;
	gE_search *search;
	gint auto_indent;
	gint show_tabs;
	gint show_status;
	gint tab_pos;
	gchar *print_cmd;
	gint have_toolbar;
#if PLUGIN_TEST
	plugin *hello;
#endif
} gE_window;

typedef struct _gE_document {
	gE_window *window;
	GtkWidget *text;
	GtkWidget *tab_label;
	gchar *filename;
	gint changed_id;
	gint changed;
	gint word_wrap;
} gE_document;

typedef struct _gE_prefs {
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *tfont;
	GtkWidget *abox;
	GtkWidget *bbox;
	GtkWidget *separator;
	GtkWidget *wrap, *synhi, *indent;
	GtkWidget *tslant;
	GtkWidget *tweight;
	GtkWidget *tsize;
	GtkWidget *pcmd;
} gE_prefs;

typedef struct _gE_data {
	gE_window *window;
	gE_document *document;
	gpointer temp1;
	gpointer temp2;
} gE_data;


void prog_init(char **file);
void destroy_window (GtkWidget *widget, GdkEvent *event, gE_data *data);

#if PLUGIN_TEST
  /* Plugins */
  void start_plugin (GtkWidget *widget, gE_data *data);
  void add_plugin_to_menu (gE_window *window, plugin_info *info);
  void add_plugins_to_window (plugin_info *info, gE_window *window);
#endif

/* Preferences */
void gE_save_settings(gE_window *window, gchar *cmd);
void gE_get_settings(gE_window *window);
gE_prefs *gE_prefs_window();
void gE_get_rc_file();
void gE_rc_parse();

void gE_window_toggle_statusbar (GtkWidget *w, gpointer cbwindow);

void gE_show_version();
void gE_about_box();
void gE_quit ();

void gE_window_new_with_file(gE_window *window, char *filename);
gE_window *gE_window_new();

gE_document *gE_document_new(gE_window *window);
gE_document *gE_document_current(gE_window *window);
void gE_document_toggle_wordwrap (GtkWidget *w, gpointer cbwindow);
void notebook_switch_page (GtkWidget *w, GtkNotebookPage *page, gint num, gE_window *window);

gint gE_file_open (gE_window *window, gE_document *document, gchar *filename);
gint gE_file_save (gE_window *window, gE_document *document, gchar *filename);
gint file_open_wrapper (gE_data *data);

#if !( (GTK_MAJOR_VERSION==1) && (GTK_MINOR_VERSION==1) )
size_t Ctime;
#endif

extern gE_prefs *prefs_window;
extern GList *window_list;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MAIN_H__ */
