/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts and Evan Lawrence
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

#ifndef __WINDOW_H___
#define __WINDOW_H__

#define MSGBAR_CLEAR		" "
#define MSGBAR_FILE_NEW		"New File..."
#define MSGBAR_FILE_OPENED	"File Opened..."
#define MSGBAR_FILE_CLOSED	"File Closed..."
#define MSGBAR_FILE_CLOSED_ALL  "All Files Closed..."
#define MSGBAR_FILE_PRINTED	"Print Command Executed..."
#define MSGBAR_FILE_SAVED	"File Saved..."
#define MSGBAR_CUT		"Selection Cut..."
#define MSGBAR_COPY		"Selection Copied..."
#define MSGBAR_PASTE		"Selection Pasted..."
#define MSGBAR_SELECT_ALL	"All Text Selected..."

typedef struct _Window Window;

struct _Window
{
	GtkWidget *window;
	GtkWidget *statusbox;
	GtkWidget *statusbar;
	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *notebook;
	GtkWidget *open_fileselector;
	GtkWidget *save_fileselector;
	GtkWidget *line_label, *col_label;
	GtkWidget *files_list_window;
	GtkWidget *files_list_window_data;
	GtkWidget *files_list_window_toolbar;

	GtkWidget *popup;

	GtkPositionType tab_pos;
};

/*extern Window *window; */
extern GList *window_list;

extern GtkWidget *col_label;

void gedit_window_set_auto_indent (gint auto_indent);
void gedit_window_set_status_bar (gint show_status);
void gedit_window_new (GnomeMDI *mdi, GnomeApp *app);

void doc_swaphc_cb (GtkWidget *w, gpointer cbdata);

GtkWindow * gedit_window_active (void);
GnomeApp  * gedit_window_active_app (void);


#endif /* __WINDOW_H__ */
