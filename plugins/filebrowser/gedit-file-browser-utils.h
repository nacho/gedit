/*
 * gedit-file-browser-utils.h - Gedit plugin providing easy file access 
 * from the sidepanel
 * 
 * Copyright (C) 2006 - Jesse van den Kieboom <jesse@icecrew.nl>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GEDIT_FILE_BROWSER_UTILS_H__
#define __GEDIT_FILE_BROWSER_UTILS_H__

#include <gedit/gedit-window.h>
#include <gio/gio.h>

GdkPixbuf	*gedit_file_browser_utils_pixbuf_from_theme	(gchar const    *name,
								 GtkIconSize     size);

GdkPixbuf	*gedit_file_browser_utils_pixbuf_from_icon	(GIcon          *icon,
								 GtkIconSize     size);
GdkPixbuf	*gedit_file_browser_utils_pixbuf_from_file	(GFile          *file,
								 GtkIconSize     size);

gchar		*gedit_file_browser_utils_file_basename		(GFile          *file);

gboolean	 gedit_file_browser_utils_confirmation_dialog	(GeditWindow    *window,
								 GtkMessageType  type,
								 gchar const    *message,
								 gchar const    *secondary,
								 gchar const    *button_stock,
								 gchar const    *button_label);

#endif /* __GEDIT_FILE_BROWSER_UTILS_H__ */
/* ex:ts=8:noet: */
