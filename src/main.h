#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define STRING_LENGTH_MAX	256
typedef struct _gE_search gE_search;
typedef struct _gE_prefs gE_prefs;
typedef struct _gE_window gE_window;
typedef struct _gE_document gE_document;

void prog_init(int fnum, char **file);
void destroy_window (GtkWidget *widget, GtkWidget **window);
void file_quit_cmd_callback(GtkWidget *widget, gpointer data);
void file_new_cmd_callback(GtkWidget *widget, gpointer data);
void file_open_cmd_callback(GtkWidget *widget, gpointer data);
void file_save_cmd_callback(GtkWidget *widget, gpointer data);
void file_save_as_cmd_callback(GtkWidget *widget, gpointer data);
void file_close_cmd_callback(GtkWidget *widget, gpointer data);
void prefs_callback(GtkWidget *widget, gpointer data);
void edit_cut_cmd_callback(GtkWidget *widget, gpointer data);
void edit_copy_cmd_callback (GtkWidget *widget, gpointer data);
void edit_paste_cmd_callback (GtkWidget *widget, gpointer data);
void edit_selall_cmd_callback (GtkWidget *widget, gpointer data);
void document_changed_callback (GtkWidget *widget, gpointer);
void search_search_cmd_callback (GtkWidget *w, gpointer);
void search_replace_cmd_callback (GtkWidget *w, gpointer);
void search_again_cmd_callback (GtkWidget *w, gpointer);
void tab_top_cback (GtkWidget *widget, gpointer data);
void tab_bot_cback (GtkWidget *widget, gpointer data);
void tab_lef_cback (GtkWidget *widget, gpointer data);
void tab_rgt_cback (GtkWidget *widget, gpointer data);
void auto_indent_callback (GtkWidget *text, GdkEventKey *event);
void line_pos_callback (GtkWidget *w, GtkWidget *text);
void gE_event_button_press (GtkWidget *w, GdkEventButton *event);

gE_prefs *gE_prefs_window();
void gE_get_rc_file();
void gE_rc_parse();
void gE_show_version();
void gE_about_box();
void gE_window_new_with_file(gE_window *window, char *filename);
gE_window *gE_window_new();
gE_document *gE_document_new(gE_window *window);
gE_document *gE_document_current(gE_window *window);
gint gE_file_open (gE_document *document, gchar *filename);
gint gE_file_save (gE_document *document, gchar *filename);

size_t Ctime;

extern gE_prefs *prefs_window;
extern gE_window *main_window;

struct _gE_window {
	GtkWidget *window;
	GtkWidget *text;
	GtkWidget *statusbox;
	GtkWidget *statusbar;
	GtkWidget *menubar;
	GtkWidget *notebook;
	GtkWidget *open_fileselector;
	GtkWidget *save_fileselector;
	GtkWidget *line_label, *col_label;
	GtkAcceleratorTable *accel;
	GList *documents;
	gE_search *search;
};

struct _gE_document {
	gE_window *window;
	GtkWidget *text;
	GtkWidget *tab_label;
	gchar *filename;
	gint changed_id;
	gint changed;
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
}; 

struct _gE_search {
	GtkWidget *window;
	GtkWidget *start_at_cursor;
	GtkWidget *start_at_beginning;
	GtkWidget *case_sensitive;
	GtkWidget *search_entry;
	gint replace, again;
	GtkWidget *replace_box;
	GtkWidget *replace_entry;
	GtkWidget *prompt_before_replacing;
	
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MAIN_H__ */
