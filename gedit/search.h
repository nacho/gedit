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

  gint search_verify_document (void);
  void search_end (void);
  void search_start (void);
  void dump_search_state (void);
  void count_lines_cb (GtkWidget *widget, gpointer data);
  void find_cb (GtkWidget *widget, gpointer data);
  void replace_cb (GtkWidget *widget, gpointer data);
  void goto_line_cb (GtkWidget *widget, gpointer data);
  gint pos_to_line (gint pos, gint *numlines);
gulong line_to_pos (Document *doc, gint line, gint *lines);
  gint search_text_execute ( gulong starting_position, gint case_sensitive, guchar *text_to_search_for,
			     guint * pos_found, gint * line_found, gint * total_lines, gint return_the_line_number);
  gint gedit_search_replace_all_execute ( View *view, guchar *search_text,
					  guchar *replace_text, gint case_sensitive,
					  guchar **new_buffer);

/* Stopwatch functions */
void start_time (void);
double print_time (void);

#endif /* __SEARCH_H__ */
