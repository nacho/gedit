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

#ifndef __GEDIT_SEARCH_H__
#define __GEDIT_SEARCH_H__

#include "view.h"
#include "document.h"


void gedit_search_start  (void);
void gedit_search_end    (void);
gint gedit_search_verify (void);

/* Callbacks */
void gedit_find_cb (GtkWidget *widget, gpointer data);
void gedit_find_again_cb (GtkWidget *widget, gpointer data);
void gedit_replace_cb (GtkWidget *widget, gpointer data);
void gedit_goto_line_cb (GtkWidget *widget, gpointer data);
void gedit_file_info_cb (GtkWidget *widget, gpointer data);


guint gedit_search_line_to_pos (gint line, gint *lines);

gint gedit_search_execute (guint starting_position,
			   gint case_sensitive,
			   const guchar *text_to_search_for,
			   guint *pos_found,
			   gint  *line_found,
			   gint  *total_lines,
			   gboolean return_the_line_number);

gint gedit_replace_all_execute (GeditView *view,
				guint start_pos,
				const guchar *search_text,
				const guchar *replace_text,
				gint case_sensitive,
				guchar **buffer);







#endif /* __GEDIT_SEARCH_H__ */
