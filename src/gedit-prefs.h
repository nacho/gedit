/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs.h
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


/* TODO: It should be rewritten to use GConf */
/* TODO: check if all the preferences are really used in the code */

#ifndef __GEDIT_PREFS_H__
#define __GEDIT_PREFS_H__

typedef struct _GeditPreferences GeditPreferences;

typedef enum {
	GEDIT_TOOLBAR_SYSTEM,
	GEDIT_TOOLBAR_ICONS,
	GEDIT_TOOLBAR_ICONS_AND_TEXT
} GeditToolbarSetting;

struct _GeditPreferences
{
	guint auto_indent;
	gint  word_wrap;
	gint  toolbar_labels;
	gint  show_tabs;
	gint  tab_pos; /* Mdi Tab Position */
	gint  tab_size; /* Tab size */
	guint show_status;
	gint  show_tooltips;
	gint  have_toolbar;
	gint  have_tb_pix;
	gint  have_tb_text;
	gint  use_relief_toolbar;
	gint  undo_levels;

/*	gboolean use_fontset;
 */
	gint  use_default_font;
	gchar *font;

	gint  splitscreen;

	gint  mdi_mode;
	
	gint  use_default_colors;
	guint16 bg[3];
	guint16 fg[3];
	
	gint width, height;
	
	gint run;		/* Flag to see if this is the first
				   time gedit is run */
	
	gint close_doc;

	gint print_wrap_lines : 1;        /* Printing stuf ...*/
	gint print_lines;
	gint print_header;
	gint print_orientation;
	gchar *papersize;
};

extern GeditPreferences *settings;

void gedit_prefs_save_settings (void);
void gedit_prefs_load_settings (void);

#endif /* __GEDIT_PREFS_H__ */

