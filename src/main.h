#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For size_t */
#include <stdio.h>
#if PLUGIN_TEST
#include "plugin.h"
#endif

#define STRING_LENGTH_MAX	256
#define GEDIT_ID	"gEdit 0.4.0"
typedef struct _gE_search gE_search;
typedef struct _gE_prefs gE_prefs;
typedef struct _gE_window gE_window;
typedef struct _gE_document gE_document;

void prog_init(char **file);
void destroy_window (GtkWidget *widget, GtkWidget **window);

/* File Ops */
void file_quit_cmd_callback(GtkWidget *widget, gpointer data);
void file_new_cmd_callback(GtkWidget *widget, gpointer data);
void file_open_cmd_callback(GtkWidget *widget, gpointer data);
void file_save_cmd_callback(GtkWidget *widget, gpointer data);
void file_save_as_cmd_callback(GtkWidget *widget, gpointer data);
void file_close_cmd_callback(GtkWidget *widget, gpointer data);
void file_print_cmd_callback (GtkWidget *widget, gpointer data);

void prefs_callback(GtkWidget *widget, gpointer data);

/* Edit functions */
void edit_cut_cmd_callback(GtkWidget *widget, gpointer data);
void edit_copy_cmd_callback (GtkWidget *widget, gpointer data);
void edit_paste_cmd_callback (GtkWidget *widget, gpointer data);
void edit_selall_cmd_callback (GtkWidget *widget, gpointer data);

void document_changed_callback (GtkWidget *widget, gpointer);

/* Search and Replace */
void search_search_cmd_callback (GtkWidget *w, gpointer);
void search_replace_cmd_callback (GtkWidget *w, gpointer);
void search_again_cmd_callback (GtkWidget *w, gpointer);
void search_goto_line_callback (GtkWidget *w, gpointer data);
void search_create (gE_search *options, gint replace);

/* Tab positioning */
void tab_top_cback (GtkWidget *widget, gpointer data);
void tab_bot_cback (GtkWidget *widget, gpointer data);
void tab_lef_cback (GtkWidget *widget, gpointer data);
void tab_rgt_cback (GtkWidget *widget, gpointer data);
void tab_toggle_cback (GtkWidget *widget, gpointer data);

#if PLUGIN_TEST
  /* Plugins */
  void send_hello (GtkWidget *widget, gpointer data);
#endif

/* Auto indent */
void auto_indent_callback (GtkWidget *text, GdkEventKey *event);
void auto_indent_toggle_callback (GtkWidget *w, gpointer data);
void line_pos_callback (GtkWidget *w, GtkWidget *text);
void gE_event_button_press (GtkWidget *w, GdkEventButton *event);

/* Preferences */
void gE_save_settings(gchar *cmd);
void gE_get_settings();
gE_prefs *gE_prefs_window();
void gE_get_rc_file();
void gE_rc_parse();

void gE_window_toggle_statusbar (GtkWidget *w, gpointer data);

void gE_show_version();
void gE_about_box();
void gE_quit ();

void gE_window_new_with_file(gE_window *window, char *filename);
gE_window *gE_window_new();

gE_document *gE_document_new(gE_window *window);
gE_document *gE_document_current(gE_window *window);
void gE_document_toggle_wordwrap (GtkWidget *w, gpointer data);

gint gE_file_open (gE_document *document, gchar *filename);
gint gE_file_save (gE_document *document, gchar *filename);
gint file_open_wrapper (char *name);

#if !( (GTK_MAJOR_VERSION==1) && (GTK_MINOR_VERSION==1) )
size_t Ctime;
#endif

extern gE_prefs *prefs_window;
extern gE_window *main_window;

struct _gE_window {
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
};

struct _gE_document {
	gE_window *window;
	GtkWidget *text;
	GtkWidget *tab_label;
	gchar *filename;
	gint changed_id;
	gint changed;
	gint word_wrap;
};

struct _gE_prefs {
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
}; 

struct _gE_search {
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
	
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MAIN_H__ */
