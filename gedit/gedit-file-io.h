/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GEDIT_FILE_IO_H__
#define __GEDIT_FILE_IO_H__


/* we should be able to not expose this functions ...*/
extern gint gedit_file_open (gedit_document *doc, gchar *fname);
extern gint gedit_file_save (gedit_document *doc, gchar *fname);

extern void file_new_cb (GtkWidget *widget, gpointer cbdata);

extern void file_open_cb (GtkWidget *widget, gpointer cbdata);

extern void file_save_cb (GtkWidget *widget);
extern void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
extern void file_save_all_cb (GtkWidget *widget, gpointer cbdata);

extern void file_close_cb (GtkWidget *widget, gpointer cbdata);
extern void file_close_all_cb (GtkWidget *widget, gpointer cbdata);

extern void file_revert_cb (GtkWidget *widget, gpointer cbdata);

#endif /* __GEDIT_FILE_IO_H__ */


