/* gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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

#include <config.h>
#include <gnome.h>
#include "gedit.h"
#include "gE_prefs.h"
#include "gE_prefs_box.h"
#include "gedit-window.h"
#include "gE_view.h"
#include "gE_mdi.h"


typedef struct _gedit_prefs_data gedit_prefs_data;

struct _gedit_prefs_data {
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


static GtkWidget *fs;
static gedit_prefs_data *prefs;
GList *plugin_list;
/*plugin_callback_struct pl_callbacks;*/
extern GList *plugins;

static void properties_changed (GtkWidget *widget, GnomePropertyBox *pbox);

guint mdi_type [NUM_MDI_MODES] = {

	GNOME_MDI_DEFAULT_MODE,
	GNOME_MDI_NOTEBOOK,
	GNOME_MDI_TOPLEVEL,
	GNOME_MDI_MODAL
	
};

gchar *mdi_type_label [NUM_MDI_MODES] = {

	N_("Default"),
	N_("Notebook"),
	N_("Toplevel"),
	N_("Modal"),

};

static void
cancel_cb (void)
{
	gtk_widget_destroy (GTK_WIDGET (prefs->pbox));
	g_free (prefs->curW);
	g_free (prefs->curH);
	g_free (prefs);
	prefs = NULL;

	if (fs)
		gtk_widget_destroy (fs);
	fs = NULL;
}

void
gedit_window_refresh (gedit_window *w)
{
	gint i, j;
	gedit_view *nth_view;
	gedit_document *doc;
	GtkStyle *style;
	GdkColor *bg, *fg;

	gedit_view_set_split_screen (GE_VIEW (mdi->active_view),
				     (gint) GE_VIEW (mdi->active_view)->splitscreen);

	gedit_window_set_status_bar (settings->show_status);

/*  
	for (i = 0; i < NUM_MDI_MODES; i++) {
	
	  if (GTK_TOGGLE_BUTTON (prefs->mdi_type[i])->active) {
	
            if (mdiMode != mdi_type[i]) {
          
              mdiMode = mdi_type[i];
              gnome_mdi_set_mode (mdi, mdiMode);
            }
         
            break;
        
          }
          
        }
*/
	style = gtk_style_copy (gtk_widget_get_style (
		GE_VIEW (mdi->active_view)->text));

	bg = &style->base[0];
	bg->red = settings->bg[0];
	bg->green = settings->bg[1];
	bg->blue = settings->bg[2];

	fg = &style->text[0];
	fg->red = settings->fg[0];
	fg->green = settings->fg[1];
	fg->blue = settings->fg[2];

	for (i = 0; i < g_list_length (mdi->children); i++)
	{
		doc = GE_DOCUMENT(g_list_nth_data (mdi->children, i));
  	
		for (j = 0; j < g_list_length (doc->views); j++)
		{
/* 	  		g_message ("i = %d, j = %d", i, j); */
			nth_view = g_list_nth_data (doc->views, j);
			gedit_view_set_font (nth_view, settings->font);
			gedit_view_set_word_wrap (nth_view, settings->word_wrap);
  	    
			gtk_widget_set_style (GTK_WIDGET (nth_view->split_screen),
					      style);
			gtk_widget_set_style (GTK_WIDGET (nth_view->text),
					      style);
		}
	}
}

void
gedit_apply (GnomePropertyBox *pbox, gint page, gedit_data *data)
{
	gint i;
	GtkStyle *style;
	GdkColor *c;

	/* if OK is pressed, lets not run this code twice */
        if (page != -1)
                return;

	/* General Settings */
	settings->auto_indent = (GTK_TOGGLE_BUTTON (prefs->autoindent)->active);
	settings->show_status = (GTK_TOGGLE_BUTTON (prefs->status)->active);
	GE_VIEW (mdi->active_view)->splitscreen = (GTK_TOGGLE_BUTTON (prefs->split)->active);
	settings->word_wrap = (GTK_TOGGLE_BUTTON (prefs->wordwrap)->active);

	/* Print Settings */
	settings->print_cmd = g_strdup (gtk_entry_get_text (GTK_ENTRY(prefs->pcmd)));
  
	/* Font Settings */
	settings->font = g_strdup (gtk_entry_get_text (GTK_ENTRY(prefs->font)));

	/* Window Settings */
	settings->width = atoi (gtk_entry_get_text (GTK_ENTRY(prefs->preW)));
	settings->height = atoi (gtk_entry_get_text (GTK_ENTRY(prefs->preH)));  
  
	/* Document Settings */
	if (GTK_TOGGLE_BUTTON (prefs->DButton1)->active)
		settings->close_doc = FALSE;
	if (GTK_TOGGLE_BUTTON (prefs->DButton2)->active)
		settings->close_doc = TRUE;

	for (i = 0; i < NUM_MDI_MODES; i++)
	{
		if (GTK_TOGGLE_BUTTON (prefs->mdi_type[i])->active)
		{
			if (mdiMode != mdi_type[i])
			{
				mdiMode = mdi_type[i];
				gnome_mdi_set_mode (mdi, mdiMode);
			}
			break;
		}
        }
	
	style = gtk_style_new ();
	
	c = &style->base[0];
	
	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (prefs->bgpick),
				    &c->red, &c->green, &c->blue, 0);
	settings->bg[0] = c->red;
	settings->bg[1] = c->green;
	settings->bg[2] = c->blue;
	
	c = &style->text[0];
	
	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (prefs->fgpick),
				    &c->red, &c->green, &c->blue, 0);
	settings->fg[0] = c->red;
	settings->fg[1] = c->green;
	settings->fg[2] = c->blue;
	 
	gedit_save_settings ();	
	gedit_window_refresh (data->window);
}

void
get_prefs (gedit_data *data)
{
	gint i;
	gchar *tmp;
	GtkStyle *style;
	GdkColor *c;
  
	gtk_entry_set_text (GTK_ENTRY (prefs->pcmd), settings->print_cmd);
	gtk_entry_set_text (GTK_ENTRY (prefs->font), settings->font);

	tmp = g_malloc (1);
	sprintf (tmp, "%d", settings->width);
	gtk_entry_set_text (GTK_ENTRY (prefs->preW), tmp);
	sprintf (tmp, "%d", settings->height);
	gtk_entry_set_text (GTK_ENTRY (prefs->preH), tmp);
	g_free (tmp);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->autoindent), 
				     settings->auto_indent);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->status),
				     settings->show_status);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->split),
				     GE_VIEW (mdi->active_view)->splitscreen);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->wordwrap),
				     settings->word_wrap);
  					   

	style = gtk_style_copy (gtk_widget_get_style (
		GE_VIEW (mdi->active_view)->text));
	c = &style->base[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (prefs->bgpick),
				    c->red, c->green, c->blue, 0);
	
	c = &style->text[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (prefs->fgpick),
				    c->red, c->green, c->blue, 0);
	


	if (!settings->close_doc)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->DButton1), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->DButton2), TRUE);
     
	for (i = 0; i < NUM_MDI_MODES; i++)
		if (mdiMode == mdi_type[i])
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->mdi_type[i]),
						      TRUE);
			break;
		}
}

/* General UI Stuff.. */

static GtkWidget *
general_page_new (void)
{
	GtkWidget *main_vbox, *vbox, *hbox2, *frame, *hbox, *label;

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
	gtk_widget_show (main_vbox);
  
	frame = gtk_frame_new (_("Appearance"));
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
	gtk_widget_show (frame);
  
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);
  
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);
  
	prefs->status = gtk_check_button_new_with_label (_("Show Statusbar"));
	gtk_box_pack_start(GTK_BOX(hbox), prefs->status, TRUE, TRUE, 0);
	gtk_widget_show (prefs->status);
  
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);
  
	prefs->split = gtk_check_button_new_with_label (_("Show Splitscreen"));
	gtk_box_pack_start(GTK_BOX(hbox), prefs->split, TRUE, TRUE, 0);
	gtk_widget_show (prefs->split);
  
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);
 
  
	frame = gtk_frame_new (_("Editor Behavior"));
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
	gtk_widget_show (frame);
  
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_widget_show (hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 10);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show(vbox);

	prefs->autoindent = gtk_check_button_new_with_label (_("Enable Autoindent"));
	gtk_box_pack_start(GTK_BOX(vbox), prefs->autoindent, TRUE, TRUE, 0);
	gtk_widget_show (prefs->autoindent);
  
	prefs->wordwrap = gtk_check_button_new_with_label (_("Enable Wordwrap"));
	gtk_box_pack_start(GTK_BOX(vbox), prefs->wordwrap, TRUE, TRUE, 0);
	gtk_widget_show (prefs->wordwrap);
	
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 10);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show(vbox);
	  
  	
  	hbox2 = gtk_hbox_new (FALSE, 0);
  	gtk_container_border_width(GTK_CONTAINER(hbox2), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 10);
	gtk_widget_show(hbox2);
	
  	prefs->bgpick = gnome_color_picker_new ();
  	gtk_box_pack_start (GTK_BOX(hbox2), prefs->bgpick, FALSE, FALSE, 10);
  	
	gtk_signal_connect (GTK_OBJECT (prefs->bgpick), "color_set",
					GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);
	
  	label = gtk_label_new (_("Text Background"));
  	gtk_box_pack_start (GTK_BOX(hbox2), label, FALSE, FALSE, 0);
  	gtk_widget_show (label);
  	
  	

  	hbox2 = gtk_hbox_new (FALSE, 0);
  	gtk_container_border_width(GTK_CONTAINER(hbox2), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 10);
	gtk_widget_show(hbox2);
	
	prefs->fgpick = gnome_color_picker_new ();
  	gtk_box_pack_start (GTK_BOX(hbox2), prefs->fgpick, FALSE, FALSE, 10);

	gtk_signal_connect (GTK_OBJECT (prefs->fgpick), "color_set",
					GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);
					
  	gtk_widget_show (prefs->bgpick);

  	gtk_widget_show (prefs->fgpick);
  	
  	label = gtk_label_new (_("Text Foreground"));
  	gtk_box_pack_start (GTK_BOX(hbox2), label, FALSE, FALSE, 0);
  	gtk_widget_show (label);
  
	return main_vbox;
}

/* End of General Stuff.. */


/* Font Stuff... */

void
font_sel_ok (GtkWidget *w, GtkWidget *fsel)
{
	gtk_entry_set_text(GTK_ENTRY (prefs->font),
	gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG(fsel)));
	
	gtk_widget_destroy (fsel);
	fs =  NULL;
}

static void
font_sel_cancel (GtkWidget *w, GtkWidget *fsel)
{
	if (fs)
		gtk_widget_destroy (fsel);
	fs =  NULL;
}

void
font_sel (void)
{
	if (fs)
	{
		gdk_window_show (fs->window);
		gdk_window_raise (fs->window);
		return;
	}

	fs = gtk_font_selection_dialog_new ("Font");
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fs), 
	gtk_entry_get_text(GTK_ENTRY(prefs->font)));

	gtk_signal_connect (GTK_OBJECT(GTK_FONT_SELECTION_DIALOG (fs)->ok_button), "clicked",
			    GTK_SIGNAL_FUNC(font_sel_ok), fs);

	gtk_signal_connect (GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fs)->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC(font_sel_cancel), fs);

	gtk_widget_show (fs);
}

static GtkWidget *
doc_page_new (void)
{
	GtkWidget *main_vbox, *vbox, *frame, *hbox;
	GtkWidget *button;
/*	GtkWidget *label; */

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
	gtk_widget_show (main_vbox);
  
	/* Document Closing stuffs */
	frame = gtk_frame_new (_("Document Settings"));
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
	gtk_widget_show (frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show(hbox);

	prefs->DButton1 = gtk_radio_button_new_with_label (NULL, _("New Document After Closing Last"));
	gtk_box_pack_start (GTK_BOX (hbox), prefs->DButton1, TRUE, TRUE, 0);
	gtk_widget_show (prefs->DButton1);
  
	prefs->DButton2 = gtk_radio_button_new_with_label (
					gtk_radio_button_group (GTK_RADIO_BUTTON (prefs->DButton1)),
												_("Close Window After Closing Last"));
	
	gtk_box_pack_start (GTK_BOX (hbox), prefs->DButton2, TRUE, TRUE, 0);
	gtk_widget_show (prefs->DButton2);
  
	/* End of docyment closing stuffs */
  
	/* Font Settings (within Document Settings frame) */
  
	frame = gtk_frame_new (_("Current Font"));
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 2);
	gtk_widget_show (frame);
  
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);
  
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show(hbox);

	prefs->font = gtk_entry_new ();
	gtk_box_pack_start(GTK_BOX(hbox), prefs->font, TRUE, TRUE, 0);
	gtk_widget_show (prefs->font);
    
	button = gtk_button_new_with_label (_("Select..."));
  	gtk_signal_connect (GTK_OBJECT(button), "clicked",
					GTK_SIGNAL_FUNC(font_sel),
					NULL);
  
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	gtk_widget_show(button);
  
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);
  
	/* End of Font Setttings */

	/* Print Command */
	frame = gtk_frame_new (_("Print Command"));
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
	gtk_widget_show (frame);
  
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);
  
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show(hbox);
  
	prefs->pcmd = gtk_entry_new ();
	gtk_box_pack_start(GTK_BOX(hbox), prefs->pcmd, TRUE, TRUE, 0);
	gtk_widget_show (prefs->pcmd);
	/* End Of Print Command */

	return main_vbox;

}


/* Window Stuff.. */

void
use_current (GtkWidget *w, gpointer size)
{
	if (size == 0)
		gtk_entry_set_text (GTK_ENTRY (prefs->preW), prefs->curW);
	else
		gtk_entry_set_text (GTK_ENTRY (prefs->preH), prefs->curH);
}

static GtkWidget *
window_page_new (void)
{
	GtkWidget *main_vbox, *vbox, *vbox2, *frame, *hbox;
	GtkWidget *label, *button;
	gint i;
/*	gchar *tmp; */

	prefs->curW = g_malloc (1);
	prefs->curH = g_malloc (1);
	sprintf (prefs->curW, "%d", GTK_WIDGET(mdi->active_window)->allocation.width);
	sprintf (prefs->curH, "%d", GTK_WIDGET(mdi->active_window)->allocation.height);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
	gtk_widget_show (main_vbox);
  
	
	/* Window Size Settings */
  
	frame = gtk_frame_new (_("Window Size"));
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
	gtk_widget_show (frame);
  
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);
  
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (hbox), 4);
	gtk_widget_show (hbox);
  
	label = gtk_label_new (_("Current Width:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);
  	
	prefs->cur = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (prefs->cur), prefs->curW);
	gtk_widget_set_sensitive (prefs->cur, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), prefs->cur, FALSE, FALSE, 3);
	gtk_widget_show (prefs->cur);
	
	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox2), 1);
	gtk_widget_show (vbox2);
  	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (hbox), 4);
	gtk_widget_show (hbox);

	label = gtk_label_new (_("Startup Width:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);
  	
	prefs->preW = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), prefs->preW, FALSE, FALSE, 3);
	gtk_widget_show (prefs->preW);
	
	button = gtk_button_new_with_label (_("Use Current"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
					GTK_SIGNAL_FUNC (use_current), (gint) 0);
	gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 4);
	gtk_widget_show (button);

  	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (hbox), 4);
	gtk_widget_show (hbox);
  
	label = gtk_label_new (_("Current Height:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);
  	
	prefs->cur = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (prefs->cur), prefs->curH);
	gtk_widget_set_sensitive (prefs->cur, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), prefs->cur, FALSE, FALSE, 3);
	gtk_widget_show (prefs->cur);
	
	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox2), 1);
	gtk_widget_show (vbox2);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (hbox), 4);
	gtk_widget_show (hbox);
	  	
	label = gtk_label_new (_("Startup Height:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);
	
	prefs->preH = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), prefs->preH, FALSE, FALSE, 3);
	gtk_widget_show (prefs->preH);

	button = gtk_button_new_with_label (_("Use Current"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
					GTK_SIGNAL_FUNC (use_current), (gpointer) 1);
	gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 4);
	gtk_widget_show (button);  	
  
	/* MDI Mode settnigs */
  
	frame = gtk_frame_new (_("MDI Mode"));
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
	gtk_widget_show (frame);
	 
	vbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_widget_show (vbox);
  
	prefs->mdi_list = NULL;
  
	for (i = 0; i < NUM_MDI_MODES; i++)
	{
		prefs->mdi_type[i] = GTK_RADIO_BUTTON (
			gtk_radio_button_new_with_label (prefs->mdi_list,
							 _(mdi_type_label[i])));
       
		gtk_widget_show (GTK_WIDGET (prefs->mdi_type[i]));
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(prefs->mdi_type[i]), TRUE, TRUE, 2);
		prefs->mdi_list = gtk_radio_button_group (prefs->mdi_type[i]);
	}
  
	return main_vbox;
}

/* End of Window Stuff */

static void
properties_changed (GtkWidget *widget, GnomePropertyBox *pbox)
{
	GnomePropertyBox *prop = GNOME_PROPERTY_BOX (prefs->pbox);
  
	gnome_property_box_changed (prop);
}

void
gedit_prefs_dialog (GtkWidget *widget, gpointer data)
{
	static GnomeHelpMenuEntry help_entry = { NULL, "properties" };
	GtkWidget *label;
	gint i;

	gedit_data *data = (gedit_data *)data;

	prefs = g_malloc (sizeof(gedit_prefs_data));

	if (!prefs)
		return;

	prefs->pbox = (GNOME_PROPERTY_BOX (gnome_property_box_new ()));
	prefs->gData = data;
  
	gtk_signal_connect (GTK_OBJECT (prefs->pbox), "destroy",
			    GTK_SIGNAL_FUNC (cancel_cb), prefs);

	gtk_signal_connect (GTK_OBJECT (prefs->pbox), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_false), NULL);

	gtk_signal_connect (GTK_OBJECT (prefs->pbox), "apply",
			    GTK_SIGNAL_FUNC (gedit_apply), data);

	help_entry.name = gnome_app_id;
	gtk_signal_connect (GTK_OBJECT (prefs->pbox), "help",
			    GTK_SIGNAL_FUNC (gnome_help_pbox_display),
			    &help_entry);


	/* General Settings */
	gtk_notebook_append_page (GTK_NOTEBOOK((prefs->pbox)->notebook),
				  general_page_new(),
				  gtk_label_new _("General"));

	/* Window Settings */
	gtk_notebook_append_page (GTK_NOTEBOOK ((prefs->pbox)->notebook),
				  window_page_new(),
				  gtk_label_new _("Window"));

	/* Docuemnt Settings */
	gtk_notebook_append_page (GTK_NOTEBOOK( (prefs->pbox)->notebook),
				  doc_page_new(),
				  gtk_label_new _("Document"));
 
  
	get_prefs (data);

	gtk_signal_connect (GTK_OBJECT (prefs->autoindent), "toggled",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->status), "toggled",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->wordwrap), "toggled",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->split), "toggled",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->pcmd), "changed",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->font), "changed",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->preW), "changed",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->preH), "changed",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	for (i = 0; i < NUM_MDI_MODES; i++)
		gtk_signal_connect (GTK_OBJECT (prefs->mdi_type[i]), "clicked",
				    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

  
	gtk_signal_connect (GTK_OBJECT (prefs->DButton1), "toggled",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);

	gtk_signal_connect (GTK_OBJECT (prefs->DButton2), "toggled",
			    GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);
  
/*
	gtk_signal_connect (GTK_OBJECT (prefs->bgpick), "color_set",
					GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);
	
	gtk_signal_connect (GTK_OBJECT (prefs->fgpick), "color_set",
					GTK_SIGNAL_FUNC (properties_changed), prefs->pbox);
*/
	gtk_widget_show_all (GTK_WIDGET (prefs->pbox));
}
