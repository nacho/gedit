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

typedef struct _GeditToolbar GeditToolbar;

struct _GeditToolbar
{
	GtkWidget *new_button;
	GtkWidget *open_button;
	GtkWidget *save_button;
	GtkWidget *close_button;

	GtkWidget *print_button;

	GtkWidget *undo_button;
	GtkWidget *redo_button;
	GtkWidget *cut_button;
	GtkWidget *copy_button;
	GtkWidget *paste_button;

	GtkWidget *find_button;
	GtkWidget *info_button;
	GtkWidget *exit_button;

	gint new : 1;
	gint open : 1;
	gint save : 1;
	gint close : 1;
	gint print : 1;
	gint undo : 1;
	gint redo : 1;
	gint cut : 1;
	gint copy : 1;
	gint paste : 1;
	gint find : 1;
	gint info : 1;
	gint exit : 1;
};

GtkWindow *	gedit_window_active (void);
GnomeApp *	gedit_window_active_app (void);

void	gedit_window_new (GnomeMDI *mdi, GnomeApp *app);
void	gedit_window_set_auto_indent (gint auto_indent);
void	gedit_window_set_status_bar ();
void	gedit_window_refresh_toolbar (void);
void	gedit_window_set_toolbar_labels (void);
void	gedit_window_load_toolbar_widgets (void);

extern GeditToolbar *gedit_toolbar;

#endif /* __WINDOW_H__ */

/* disabled by Chema.
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
*/
/*extern Window *window; */
/*extern GList *window_list;
extern GtkWidget *col_label;*/
