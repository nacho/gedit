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
 */

#ifndef __PREFS_H__
#define __PREFS_H__

#define DEFAULT_FONT "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1"
#define DEFAULT_FONTSET "-*-*-medium-r-normal-*-14-*-*-*-*-*-*-*,*"


typedef struct _Preferences Preferences;

struct _Preferences
{
	guint auto_indent;
	gint  word_wrap;
	gint  show_tabs;
	gint  tab_pos;
	guint show_status;
	gint  show_tooltips;
	gint  have_toolbar;
	gint  have_tb_pix;
	gint  have_tb_text;
	gint  use_relief_toolbar;
	gint  undo_levels;

	gboolean use_fontset;
	gchar *font;

	gint  splitscreen;
	gint  num_recent;	/* Number of recently accessed
				   documents in the Recent Documents
				   menu */
	gint mdi_mode;
	
	guint16 bg[3];
	guint16 fg[3];
	
	gint width, height;
	
	gint run;		/* Flag to see if this is the first
				   time gedit is run */
	
	gint close_doc;

	gint print_wrap_lines : 1;        /* Printing stuf ...*/
	gint print_lines : 1;
	gint print_header;
	gint print_orientation;
	gchar *papersize;
};

extern Preferences *settings;

void gedit_save_settings (void);
void gedit_load_settings (void);
void gedit_window_refresh (void);


#endif /* __PREFS_H__ */


















