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
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 */

#ifndef __FILE_H__
#define __FILE_H__

#include "document.h"

/* we should be able to not expose this functions ...*/
extern gint gedit_file_open (Document *doc, gchar *fname);
extern gint gedit_file_stdin (Document *doc);
extern gint gedit_file_save (Document *doc, gchar *fname);
/* extern gint gedit_file_stdin (Document *doc);*/

extern void file_new_cb (GtkWidget *widget, gpointer cbdata);

extern void file_open_cb (GtkWidget *widget, gpointer cbdata);

extern void file_save_cb (GtkWidget *widget);
extern void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
extern void file_save_all_cb (GtkWidget *widget, gpointer cbdata);

extern void file_close_cb (GtkWidget *widget, gpointer cbdata);
extern void file_close_all_cb (GtkWidget *widget, gpointer cbdata);

extern void file_revert_cb (GtkWidget *widget, gpointer cbdata);

#endif /* __FILE_H__ */


