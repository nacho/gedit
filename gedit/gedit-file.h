/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-file.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_FILE_H__
#define __GEDIT_FILE_H__

#include "gedit-mdi-child.h"
#include "gnome-recent-view.h"

void 		gedit_file_new 		(void);
void 		gedit_file_open 	(GeditMDIChild *active_child);
void 		gedit_file_close 	(GtkWidget *view);
gboolean	gedit_file_save 	(GeditMDIChild *child);
gboolean 	gedit_file_save_as 	(GeditMDIChild *child);
void		gedit_file_save_all 	(void);
gboolean	gedit_file_close_all 	(void);
void		gedit_file_exit 	(void);
gboolean	gedit_file_revert 	(GeditMDIChild *child);

gboolean 	gedit_file_open_uri_list (GList* uri_list, gint line, gboolean create);
gboolean 	gedit_file_open_recent   (GnomeRecentView *view, const gchar* uri, gpointer data);
gboolean 	gedit_file_open_single_uri (const gchar* uri);

gboolean	gedit_file_open_from_stdin (GeditMDIChild *active_child);

#endif /* __GEDIT_FILE_H__ */


