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

#ifndef __GEDIT_PRINT_H__
#define __GEDIT_PRINT_H__

typedef enum {
	PRINT_ORIENT_LANDSCAPE,
	PRINT_ORIENT_PORTRAIT
} GeditPrintOrientation;

#define GEDIT_PRINT_FONT_BODY   "Courier"
#define GEDIT_PRINT_FONT_HEADER "Helvetica"
#define GEDIT_PRINT_FONT_SIZE_BODY   10
#define GEDIT_PRINT_FONT_SIZE_HEADER 12
#define GEDIT_PRINT_FONT_SIZE_NUMBERS 6

#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include <libgnomeprint/gnome-print-preview.h>

#include "document.h"
#include "view.h"
#include "utils.h"

typedef struct _PrintJobInfo {
	/* gnome print stuff */
        GnomePrintMaster *master;
	GnomePrintContext *pc;
	GnomePrinter *printer;
	const GnomePaper *paper;

	/* document stuff */
	GeditDocument *doc;
	guchar *buffer;
	GeditView *view;
	guint buffer_size;
	guchar *filename;
	gboolean print_header;
	gint print_line_numbers; 
	
	/* Font stuff */ 
	float   font_char_width;
	float   font_char_height;
	GnomeFont *font_body;
	GnomeFont *font_header;
	GnomeFont *font_numbers;

	/* Page stuff */ 
	guint   pages;
	float   page_width, page_height;
	float   margin_top, margin_bottom, margin_left, margin_right, margin_numbers;
	float   printable_width, printable_height;
	float   header_height;
	guint   total_lines, total_lines_real;
	guint   lines_per_page;
	guint   chars_per_line;
	guchar* temp;
	GeditPrintOrientation orientation;

	/* Range stuff */
	gint range;
	gint page_first;
	gint page_last;
	gboolean print_this_page;
	gboolean preview;
	
	/* buffer stuff */
	gint file_offset;
	gint current_line;

	/* Text Wrapping */
	gboolean wrapping;
	gint tab_size;
} PrintJobInfo;


void gedit_print_cb         (GtkWidget *widget, gpointer notused);
void gedit_print_preview_cb (GtkWidget *widget, gpointer notused);
 
#endif /* __GEDIT_PRINT_H__ */
