/* vi:set ts=8 sts=0 sw=8:
 *
 * gedit
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
#ifndef __SEARCH_H__
#define __SEARCH_H__

#define SEARCH_NOCASE		0x00000001
#define SEARCH_BACKWARDS	0x00000002

/* interface */
gint pos_to_line (Document *doc, gint pos, gint *numlines);
gint line_to_pos (Document *doc, gint line, gint *numlines);
gint get_line_count (Document *doc);
void seek_to_line (Document *doc, gint line, gint numlines);

gint gedit_search_search (Document *doc, gchar *str, gint pos, gulong options);
void gedit_search_replace (Document *doc, gint pos, gint len, gchar *replace);

/* gui for interface */
void search_cb (GtkWidget *widget, gpointer data);
void replace_cb (GtkWidget *widget, gpointer data);
void find_line_cb (GtkWidget *widget, gpointer data);
void count_lines_cb (GtkWidget *w, gpointer cbwindow);

/* find in files functions */
void find_in_files_cb (GtkWidget *widget, gpointer data);
void remove_search_result_cb (GtkWidget *widget, gpointer data); 
void search_result_clist_cb (GtkWidget *list, gpointer func_data);

#endif /* __SEARCH_H__ */
