/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * gedit
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
};

extern Preferences *settings;
extern gboolean use_fontset;
extern gint mdiMode;

extern void gedit_save_settings (void);
extern void gedit_load_settings (void);

extern void gedit_prefs_dialog (GtkWidget *widget, gpointer cbdata);
extern void gedit_property_box_new (void);
extern void gedit_window_refresh (void);


#endif /* __PREFS_H__ */
