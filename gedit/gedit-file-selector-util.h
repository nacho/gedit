/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-file-selector-util.h
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

/* This file is a modified version of bonobo-file-selector-util.c
 * taken from libbonoboui
 */

/*
 * bonobo-file-selector-util.h - functions for getting files from a
 * selector
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *
 * Copyright 2001 Ximian, Inc.
 *
 */

#ifndef __GEDIT_FILE_SELECTOR_UTIL_H_
#define __GEDIT_FILE_SELECTOR_UTIL_H_

#include <glib/gmacros.h>
#include <gtk/gtkwindow.h>

#include <gedit/gedit-encodings.h>

G_BEGIN_DECLS

char  *gedit_file_selector_open       (GtkWindow  *parent,
					gboolean    enable_vfs,
					const char *title,
					const char *default_path,
					const GeditEncoding **encoding);

GSList *gedit_file_selector_open_multi (GtkWindow  *parent,
					gboolean    enable_vfs,
					const char *title,
					const char *default_path,
					const GeditEncoding **encoding);

char  *gedit_file_selector_save       (GtkWindow  *parent,
					gboolean    enable_vfs,
					const char *title,
					const char *default_path,
					const char *default_filename,
					const char *untitled_name,
					const GeditEncoding **encoding);

G_END_DECLS

#endif /* __GEDIT_FILE_SELECTOR_UTIL_H_ */
