#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For size_t */
#include <stdio.h>
#define PLUGIN_TEST 1
#if PLUGIN_TEST
#include "plugin.h"
#endif

/* File Ops */
void file_quit_cmd_callback(GtkWidget *widget, gpointer cbdata);
void file_new_cmd_callback(GtkWidget *widget, gpointer cbdata);
void file_newwindow_cmd_callback (GtkWidget *widget, gpointer cbdata);
void file_open_cmd_callback(GtkWidget *widget, gpointer cbdata);
void file_save_cmd_callback(GtkWidget *widget, gpointer cbdata);
void file_save_as_cmd_callback(GtkWidget *widget, gpointer cbdata);
void file_close_cmd_callback(GtkWidget *widget, gpointer cbdata);
void file_close_window_cmd_callback (GtkWidget *widget, gpointer cbdata);
void file_print_cmd_callback (GtkWidget *widget, gpointer cbdata);

void prefs_callback(GtkWidget *widget, gpointer cbwindow);

/* Edit functions */
void edit_cut_cmd_callback(GtkWidget *widget, gpointer cbdata);
void edit_copy_cmd_callback (GtkWidget *widget, gpointer cbdata);
void edit_paste_cmd_callback (GtkWidget *widget, gpointer cbdata);
void edit_selall_cmd_callback (GtkWidget *widget, gpointer cbdata);

void document_changed_callback (GtkWidget *widget, gpointer);

/* Search and Replace */
void seek_to_line (gE_document *doc, gint line_number);
int point_to_line (gE_document *doc, gint point);
void search_search_cmd_callback (GtkWidget *w, gpointer cbdata);
void search_replace_cmd_callback (GtkWidget *w, gpointer cbdata);
void search_again_cmd_callback (GtkWidget *w, gpointer cbdata);
void search_goto_line_callback (GtkWidget *w, gpointer cbwindow);
void search_create (gE_window *window, gE_search *options, gint replace);

/* Tab positioning */
void tab_top_cback (GtkWidget *widget, gpointer cbwindow);
void tab_bot_cback (GtkWidget *widget, gpointer cbwindow);
void tab_lef_cback (GtkWidget *widget, gpointer cbwindow);
void tab_rgt_cback (GtkWidget *widget, gpointer cbwindow);
void tab_toggle_cback (GtkWidget *widget, gpointer cbwindow);

/* Auto indent */
void auto_indent_callback (GtkWidget *, GdkEventKey *event, gE_window *window);
void auto_indent_toggle_callback (GtkWidget *w, gpointer cbdata);
void line_pos_callback (GtkWidget *w, gE_data *data);
void gE_event_button_press (GtkWidget *w, GdkEventButton *, gE_window *window);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MAIN_H__ */
