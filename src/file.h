/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 */

#ifndef __FILE_H__
#define __FILE_H__

#include "document.h"

/*
typedef struct _gedit_data gedit_data;

struct _gedit_data
{
	Window *window;
	Document *document;
	gpointer temp1;
	gpointer temp2;
	gboolean flag;	general purpose flag to indicate if action completed 
};*/

gint gedit_file_open  (GeditDocument *doc, const gchar *fname);
gint gedit_file_stdin (GeditDocument *doc);
gint gedit_file_save  (GeditDocument *doc, const gchar *fname);
gint gedit_file_create_popup (const gchar *title);
void file_save_all (void);

void file_new_cb (GtkWidget *widget, gpointer cbdata);
void file_open_cb (GtkWidget *widget, gpointer cbdata);
void file_save_cb (GtkWidget *widget, gpointer cbdata);
gint file_save_document (GeditDocument *doc);
void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_cb (GtkWidget *widget, gpointer cbdata);
void file_close_cb (GtkWidget *widget, gpointer cbdata);
void file_close_all_cb (GtkWidget *widget, gpointer cbdata);
void file_revert_cb (GtkWidget *widget, gpointer cbdata);
void uri_open_cb (GtkWidget *widget, gpointer cbdata);

void gedit_close_all_flag_clear (void);

gchar * gedit_file_convert_to_full_pathname (const gchar * fname);

#endif /* __FILE_H__ */


