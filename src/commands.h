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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

/* File Ops */
void file_quit_cb (GtkWidget *widget, gpointer cbdata);

extern void window_new_cb (GtkWidget *widget, gpointer cbdata);

/* Edit functions */
void edit_cut_cb (GtkWidget *widget, gpointer cbdata);
void edit_copy_cb (GtkWidget *widget, gpointer cbdata);
void edit_paste_cb (GtkWidget *widget, gpointer cbdata);
void edit_select_all_cb (GtkWidget *widget, gpointer cbdata);

/* Tab positioning */
void tab_top_cb    (GtkWidget *widget, gpointer cbwindow);
void tab_bottom_cb (GtkWidget *widget, gpointer cbwindow);
void tab_left_cb   (GtkWidget *widget, gpointer cbwindow);
void tab_right_cb  (GtkWidget *widget, gpointer cbwindow);
void tab_toggle_cb (GtkWidget *widget, gpointer cbwindow);

#if 0
/* Auto indent */
void auto_indent_toggle_cb (GtkWidget *w, gpointer cbdata);
#endif

/* DND */
void filenames_dropped (GtkWidget * widget,
			       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time);

void options_toggle_split_screen_cb (GtkWidget *widget, gpointer data);
void options_toggle_status_bar_cb (GtkWidget *widget, gpointer data);
void options_toggle_word_wrap_cb (GtkWidget *widget, gpointer data);
void options_toggle_read_only_cb (GtkWidget *widget, gpointer data);

void tab_pos (GtkPositionType pos);

void replace_cb (GtkWidget *widget, gpointer data);
void about_cb (GtkWidget *widget, gpointer data);

#endif /* __COMMANDS_H__ */
