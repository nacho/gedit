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
#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* File Ops */
extern void file_quit_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_new_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_newwindow_cmd_callback (GtkWidget *widget, gpointer cbdata);
extern void file_open_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_save_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_save_as_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_close_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_close_all_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void file_close_window_cmd_callback(GtkWidget *widget, gpointer cbdata);


extern void prefs_callback(GtkWidget *widget, gpointer cbwindow);

/* Edit functions */
extern void edit_cut_cmd_callback(GtkWidget *widget, gpointer cbdata);
extern void edit_copy_cmd_callback (GtkWidget *widget, gpointer cbdata);
extern void edit_paste_cmd_callback (GtkWidget *widget, gpointer cbdata);
extern void edit_selall_cmd_callback (GtkWidget *widget, gpointer cbdata);

extern void doc_changed_callback (GtkWidget *widget, gpointer);

/* Search and Replace */
extern void search_search_cmd_callback (GtkWidget *w, gpointer cbdata);
extern void search_replace_cmd_callback (GtkWidget *w, gpointer cbdata);
extern void search_again_cmd_callback (GtkWidget *w, gpointer cbdata);
extern void search_goto_line_callback (GtkWidget *w, gpointer cbwindow);

/* Tab positioning */
extern void tab_top_cback (GtkWidget *widget, gpointer cbwindow);
extern void tab_bot_cback (GtkWidget *widget, gpointer cbwindow);
extern void tab_lef_cback (GtkWidget *widget, gpointer cbwindow);
extern void tab_rgt_cback (GtkWidget *widget, gpointer cbwindow);
extern void tab_toggle_cback (GtkWidget *widget, gpointer cbwindow);

/* Auto indent */
extern void auto_indent_callback(GtkWidget *, GdkEventKey *, gE_window *);
extern void auto_indent_toggle_callback(GtkWidget *w, gpointer cbdata);
extern void line_pos_callback(GtkWidget *w, gE_data *data);
extern void gE_event_button_press(GtkWidget *w, GdkEventButton *, gE_window *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __COMMANDS_H__ */
