/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
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
#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define STRING_LENGTH_MAX	256
#define GEDIT_ID	"gEdit 0.4.5"

#define UNKNOWN		"Unknown"
#define UNTITLED	"Untitled"

typedef struct _gE_search {
	GtkWidget *window;
	GtkWidget *start_at_cursor;
	GtkWidget *start_at_beginning;
	GtkWidget *case_sensitive;
	GtkWidget *search_entry;
	gint       line;
	gboolean   replace, again;
	GtkWidget *replace_box;
	GtkWidget *replace_entry;
	GtkWidget *prompt_before_replacing;
	GtkWidget *search_for;
	GtkWidget *line_item, *text_item;
} gE_search;

typedef struct _gE_window {
	GtkWidget *window;
	GtkWidget *statusbox;
	GtkWidget *statusbar;
	GtkWidget *menubar;
	GtkMenuFactory *factory; /* <-- Auto-plugin detection needs this */
	GtkWidget *toolbar;
	GtkWidget *toolbar_handle;	/* holds the toolbar */
	GtkWidget *notebook;
	GtkWidget *open_fileselector;
	GtkWidget *save_fileselector;
	GtkWidget *line_label, *col_label;
	GtkWidget *files_list_window;
	GtkWidget *files_list_window_data;
	GtkWidget *files_list_window_toolbar;
#ifdef GTK_HAVE_FEATURES_1_1_0
	GtkAccelGroup *accel;
#else
	GtkAcceleratorTable *accel;
#endif
	GList *documents;
	GtkWidget *popup;
	gE_search *search;
	int num_recent; /* Number of recently accessed documents in the 
	                         Recent Documents menu */
#ifdef WITHOUT_GNOME
	GtkWidget *menubar_handle;	/* holds the menubar */
	gboolean auto_indent;
	gboolean show_tabs;
	gboolean show_status;
	gboolean have_toolbar;
	gboolean have_tb_pix;
	gboolean have_tb_text;
	gboolean show_tooltips;
#else
	gint auto_indent;
	gint show_tabs;
	gint show_status;
	gint show_tooltips;
	gint have_toolbar;
	gint have_tb_pix;
	gint have_tb_text;
#endif
	gchar *print_cmd;
	gchar *font;
	GtkPositionType tab_pos;

} gE_window;

typedef struct _gE_document {
	gE_window *window;
	GtkWidget *text;
	GtkWidget *tab_label;
	gchar *filename;
	gint changed_id;
	gint changed;
	gint word_wrap;
	struct stat *sb;
#ifndef WITHOUT_GNOME
	GtkWidget *scrollball;
#endif
#ifdef GTK_HAVE_FEATURES_1_1_0
	gint split;
	GtkWidget *split_screen;
	GtkWidget *flag;
#endif
} gE_document;

typedef struct _gE_data {
	gE_window *window;
	gE_document *document;
	gpointer temp1;
	gpointer temp2;
	gboolean flag;	/* general purpose flag to indicate if action completed */
} gE_data;

typedef struct _gE_function {
	gchar *name;
	gchar *tooltip_text;
	#ifdef WITHOUT_GNOME
	gchar **icon;
	#else
	gchar *icon;
	#endif
	GtkSignalFunc callback;
} gE_function;

#ifdef WITHOUT_GNOME
#define N_(String) (String)	/* To avoid duplication of code when adding gnome internationalization. */
#endif

extern GList *window_list;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MAIN_H__ */
