/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __SEARCH_H__
#define __SEARCH_H__


void search_end (void);
void search_start (void);

void dump_search_state (void);

void count_lines_cb (GtkWidget *widget, gpointer data);
void find_cb (GtkWidget *widget, gpointer data);
void replace_cb (GtkWidget *widget, gpointer data);
void goto_line_cb (GtkWidget *widget, gpointer data);

gint pos_to_line (gint pos, gint *numlines);
gulong line_to_pos (Document *doc, gint line, gint *lines);
void update_text (GtkText *text, gint line, gint lines);

gint search_text_execute ( gulong starting_position,
			   gint case_sensitive,
			   guchar *text_to_search_for,
			   gulong * pos_found,
			   gint * line_found,
			   gint * total_lines,
			   gint return_the_line_number);

/* Stopwatch functions */
void start_time (void);
double print_time (void);

#endif /* __SEARCH_H__ */






/*
#define SEARCH_NOCASE		0x00000001
#define SEARCH_BACKWARDS	0x00000002
*/

/* interface */
/*
gint pos_to_line (Document *doc, gint pos, gint *numlines);
gint line_to_pos (Document *doc, gint line, gint *numlines);
gint get_line_count (Document *doc);
void seek_to_line (Document *doc, gint line, gint numlines);

gint gedit_search_search (Document *doc, gchar *str, gint pos, gulong options);
void gedit_search_replace (Document *doc, gint pos, gint len, gchar *replace);
*/

/* gui for interface */
/*
void search_cb (GtkWidget *widget, gpointer data);
void replace_cb (GtkWidget *widget, gpointer data);
void find_line_cb (GtkWidget *widget, gpointer data);
*/

/* find in files functions */
/*
void find_in_files_cb (GtkWidget *widget, gpointer data);
void remove_search_result_cb (GtkWidget *widget, gpointer data); 
void search_result_clist_cb (GtkWidget *list, gpointer func_data);

void add_search_options (GtkWidget *dialog);
gint num_widechars (const gchar *str);
void get_search_options       (Document *doc,
			       GtkWidget   *widget,
			       gchar      **txt,
			       gulong      *options,
			       gint        *pos);
void search_select            (Document *doc,
			       gchar       *str,
			       gint         pos,
			       gulong       options);


extern GtkWidget *search_result_window;
extern GtkWidget *search_result_clist;
*/

