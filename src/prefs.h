/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
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
#ifndef __GEDIT_PREFS_H__
#define __GEDIT_PREFS_H__

#define DEFAULT_FONT "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1"
#define DEFAULT_FONTSET "-*-*-medium-r-normal-*-14-*-*-*-*-*-*-*,*"

typedef struct _Preference gedit_preference;
typedef struct _PrefsData gedit_prefs_data;

struct _Preference
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
	gchar *print_cmd;
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

struct _PrefsData
{
	GnomePropertyBox *pbox;
	
	/* Font Seleftion */
	GtkWidget *fontsel;
	GtkWidget *font;
	
	/* Print Settings */
	GtkWidget *pcmd;
	
	/* General Settings */
	GtkWidget *autoindent;
	GtkWidget *status;
	GtkWidget *wordwrap;
	GtkWidget *split;
	GtkWidget *bgpick;
	GtkWidget *fgpick;
	
	gedit_data *gData;
	
	/* MDI Settings */
	GtkRadioButton *mdi_type [NUM_MDI_MODES];
	GSList *mdi_list;
	
	/* Window Settings */
	GtkWidget *cur;
	GtkWidget *preW;
	GtkWidget *preH;
	gchar *curW;
	gchar *curH;
	
	/* Doc Settings */
	/* Stuff like Autosave would go here.. if autosave
	 * was actually implemented ;) */
	/* Should print stuff go in here too? hmm.. */
	GtkWidget *DButton1;
	GtkWidget *DButton2;
	
	/* Toolbar Settings */
	/* Hmm, dunno... */
	
	/* Tab Settings */
	/* Hmm, dunno... */
};

extern gedit_preference *settings;
extern gboolean use_fontset;
extern gint mdiMode;

extern void gedit_save_settings (void);
extern void gedit_load_settings (void);

extern void gedit_prefs_dialog (GtkWidget *widget, gpointer cbdata);
extern void gedit_property_box_new (gedit_data *data);
extern void gedit_window_refresh (Window *w);


#endif /* __GEDIT_PREFS_H__ */
