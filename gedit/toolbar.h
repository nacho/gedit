/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
 *
 * Toolbar code by Andy Kahn
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

#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include <gtk/gtk.h>
#include "main.h"

typedef struct {
	char *text;
	char *tooltip_text;
	char *tooltip_private_text;
	char *icon;
	GtkSignalFunc callback;
} toolbar_data_t;

extern void gE_create_toolbar(gE_window *gw, gE_data *data);
extern GtkWidget * gE_create_toolbar_flw(gE_data *data);
extern void tb_on_cb(GtkWidget *w, gpointer cbwindow);
extern void tb_off_cb(GtkWidget *w, gpointer cbwindow);
extern gint tb_tooltips_toggle_cb(GtkWidget *w, gpointer cbwindow);
extern gint tb_relief_toggle_cb(GtkWidget *w, gpointer cbwindow);
extern gint tb_text_toggle_cb(GtkWidget *w, gpointer cbwindow);
extern gint tb_pix_toggle_cb(GtkWidget *w, gpointer cbwindow);

/* deprecated: */
extern void tb_pic_text_cb(GtkWidget *w, gpointer cbwindow);
extern void tb_pic_only_cb(GtkWidget *w, gpointer cbwindow);
extern void tb_text_only_cb(GtkWidget *w, gpointer cbwindow);
extern void tb_tooltips_on_cb(GtkWidget *w, gpointer cbwindow);
extern void tb_tooltips_off_cb(GtkWidget *w, gpointer cbwindow);

#endif

/* the end */
