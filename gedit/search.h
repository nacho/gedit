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

typedef struct _SearchInfo {
	gint state;
	gint original_readonly_state;
	guchar * buffer;
	gulong buffer_length;
	View *view;
	Document *doc;
	guchar* last_text_searched;
	gint last_text_searched_case_sensitive;
} SearchInfo;
extern SearchInfo gedit_search_info;


  gint search_verify_document (void);
  void gedit_search_end (void);
  void gedit_search_start (void);
  void dump_search_state (void);
  void count_lines_cb (GtkWidget *widget, gpointer data);
  void find_cb (GtkWidget *widget, gpointer data);
  void find_again_cb (GtkWidget *widget, gpointer data);
  void replace_cb (GtkWidget *widget, gpointer data);
  void goto_line_cb (GtkWidget *widget, gpointer data);
  gint pos_to_line (gint pos, gint *numlines);
 guint line_to_pos (Document *doc, gint line, gint *lines);
  gint search_text_execute ( gulong starting_position, gint case_sensitive, guchar *text_to_search_for,
			     guint * pos_found, gint * line_found, gint * total_lines, gint return_the_line_number);
  gint gedit_search_replace_all_execute ( View *view, guint start_pos, guchar *search_text,
					  guchar *replace_text, gint case_sensitive,
					  guchar **new_buffer);
 void search_text_not_found_notify (View *view);

/* Stopwatch functions */
void start_time (void);
double print_time (void);

#endif /* __SEARCH_H__ */
