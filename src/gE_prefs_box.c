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

#include <stdio.h>
#include <string.h>
#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include "main.h"
#include "gE_prefs.h"
#include "toolbar.h"
#include "gE_prefs_box.h"
#include "gE_document.h"

#ifdef WITHOUT_GNOME
typedef struct _gE_Prop_Box{
	GtkWidget *window;
	GtkWidget *notebook;
	GtkWidget *ok_button;	  
	GtkWidget *apply_button;   
	GtkWidget *close_button;  
	GtkWidget *help_button;	
	
	 GtkWidget *gen_vbox;
	GtkWidget *prn_vbox;
	GtkWidget *fnt_vbox;
	
	GtkWidget *separator;

} gE_Prop_Box;
#endif

typedef struct _gE_prefs_data {
	#ifndef WITHOUT_GNOME
	 GnomePropertyBox *pbox;
	#else
	 gE_Prop_Box *pbox;
	#endif
	
	
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
	GtkWidget *sball;
	
	/* Toolbar Settings */
	/* Hmm, dunno... */
	
	/* Tab Settings */
	/* Hmm, dunno... */
} gE_prefs_data;

static gE_prefs_data *prefs;
#ifdef WITHOUT_GNOME
gE_Prop_Box *pbox;
#endif

void cancel()
{
  #ifndef WITHOUT_GNOME
  gtk_widget_destroy (GTK_WIDGET (prefs->pbox));
  #else
  gtk_widget_hide (GTK_WIDGET (pbox->window));
  g_free (pbox);
  #endif
  g_free(prefs);
  prefs = NULL;
}

void gE_window_refresh(gE_window *w)
{
GtkStyle *style;
gint i;

    if (w->show_status == 0)
       gtk_widget_hide (w->statusbox);
     else
       gtk_widget_show (w->statusbox);
       
    #ifdef GTK_HAVE_FEATURES_1_1_0
     /* if (w->splitscreen == TRUE) */
       gE_document_set_split_screen (gE_document_current(w), (gint) w->splitscreen);
    #endif
    
    #ifndef WITHOUT_GNOME
       gE_document_set_scroll_ball (gE_document_current(w), (gint) w->scrollball);
    #endif
     
  style = gtk_style_new();
  gdk_font_unref (style->font);
  style->font = gdk_font_load (w->font);
  
  gtk_widget_push_style (style);    
  for (i = 0; i < g_list_length (w->documents); i++)
  {
  	#ifdef GTK_HAVE_FEATURES_1_1_0	 
  	gtk_widget_set_style(GTK_WIDGET(
  		((gE_document *) g_list_nth_data (w->documents, i))->split_screen), style);
  	#endif
  	gtk_widget_set_style(GTK_WIDGET(
  		((gE_document *) g_list_nth_data (w->documents, i))->text), style);
  }

  gtk_widget_pop_style ();
  	
  
}

void gE_apply(
#ifndef WITHOUT_GNOME
#ifndef WITHOUT_GNOME
GnomePropertyBox *pbox,
#else
gE_Prop_Box *pbox,
#endif
#endif
 gint page, gE_data *data)
{
  FILE *file;
  gchar *rc;

  
  /* General Settings */
  data->window->auto_indent = (GTK_TOGGLE_BUTTON (prefs->autoindent)->active);
  data->window->show_status = (GTK_TOGGLE_BUTTON (prefs->status)->active);  
  #ifdef GTK_HAVE_FEATURES_1_1_0
  data->window->splitscreen = (GTK_TOGGLE_BUTTON (prefs->split)->active);
  #endif
  #ifndef WITHOUT_GNOME
  data->window->scrollball  = (GTK_TOGGLE_BUTTON (prefs->sball)->active);
  #endif

  /* Print Settings */
  data->window->print_cmd = g_strdup (gtk_entry_get_text (GTK_ENTRY(prefs->pcmd)));
  
  /* Font Settings */
    data->window->font = g_strdup (gtk_entry_get_text (GTK_ENTRY(prefs->font)));
  	if ((rc = gE_prefs_open_file ("gtkrc", "r")) == NULL)
	{
		printf ("Couldn't open gtk rc file for parsing.\n");
		return;
	}

	
	if ((file = fopen(rc, "w")) == NULL) {
		g_print("Cannot open %s!\n", rc);
		return;
	}
	fprintf(file, "# gEdit rc file...\n");
	fprintf(file, "#\n");
	fprintf(file, "style \"text\"\n");
	fprintf(file, "{\n");
	fprintf(file, "  font = \"%s\"\n", data->window->font);
	fprintf(file, "}\n");
	fprintf(file, "widget_class \"*GtkText\" style \"text\"\n");
	fclose(file);  

  gE_window_refresh(data->window);
  gE_save_settings(data->window, data->window);
}



void get_prefs(gE_data *data)
{
  gtk_entry_set_text (GTK_ENTRY (prefs->pcmd), data->window->print_cmd);
  gtk_entry_set_text (GTK_ENTRY (prefs->font), data->window->font);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->autoindent), 
  					   data->window->auto_indent);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->status),
  					   data->window->show_status);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->split),
  					   data->window->splitscreen);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->sball),
  					   data->window->scrollball);
}

static GtkWidget *general_page_new()
{
  GtkWidget *main_vbox, *vbox, *frame, *hbox;

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
  #ifdef WITHOUT_GNOME
  gtk_box_pack_start (GTK_BOX(pbox->gen_vbox), main_vbox, TRUE, TRUE, 0);
  #endif
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
  
  prefs->sball = gtk_check_button_new_with_label (_("Show Scrollball"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->sball, TRUE, TRUE, 0);
  gtk_widget_show (prefs->sball);

  frame = gtk_frame_new (_("Editor Behavior"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  prefs->autoindent = gtk_check_button_new_with_label (_("Enable Autoindent"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->autoindent, TRUE, TRUE, 0);
  gtk_widget_show (prefs->autoindent);
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  prefs->wordwrap = gtk_check_button_new_with_label (_("Enable Wordwrap"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->wordwrap, TRUE, TRUE, 0);
  gtk_widget_show (prefs->wordwrap);
  
  return main_vbox;
}

static GtkWidget *print_page_new()
{
  GtkWidget *main_vbox, *vbox, *frame, *hbox;
  GtkWidget *label;

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_widget_show (main_vbox);
  
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
  
  return main_vbox;
}


void font_sel_ok (GtkWidget	*w, GtkWidget *fsel)
{
#ifdef GTK_HAVE_FEATURES_1_1_0	
  gtk_entry_set_text(GTK_ENTRY(prefs->font),
       gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG(fsel)));
#endif
  gtk_widget_destroy (fsel);
}

void font_sel_cancel (GtkWidget *w, GtkWidget *fsel)
{
  gtk_widget_destroy (fsel);
}

#ifdef GTK_HAVE_FEATURES_1_1_0	
void font_sel()
{
       GtkWidget *fs;
        
        fs = gtk_font_selection_dialog_new("Font");
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fs), 
        gtk_entry_get_text(GTK_ENTRY(prefs->font)));

        gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fs)->ok_button), "clicked",
                GTK_SIGNAL_FUNC(font_sel_ok), fs);
        gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fs)->cancel_button), "clicked",
                GTK_SIGNAL_FUNC(font_sel_cancel), fs);

        gtk_widget_show(fs);
/*  
  if (!prefs->fontsel)
    {
      prefs->fontsel = gtk_font_selection_dialog_new ("Select Font");
      
      gtk_signal_connect (GTK_OBJECT (prefs->fontsel), "destroy",
        			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
        			  &prefs->fontsel);
      
      gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
      					(prefs->fontsel)->ok_button), 
      					"clicked",
      					GTK_SIGNAL_FUNC(font_sel_ok),
      					GTK_FONT_SELECTION_DIALOG (prefs->fontsel));
     
      gtk_signal_connect_object (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
                                 (prefs->fontsel)->cancel_button),
				         "clicked",
				         GTK_SIGNAL_FUNC(gtk_widget_destroy),
				         GTK_OBJECT (prefs->fontsel));
     }
      
      if (!GTK_WIDGET_VISIBLE (prefs->fontsel))
        gtk_widget_show (prefs->fontsel);
      else
        gtk_widget_destroy (prefs->fontsel);*/
        
}
#endif /* GTK_HAVE_FEATURES_1_1_0 */

static GtkWidget *font_page_new()
{
  GtkWidget *main_vbox, *vbox, *hbox, *frame;
  GtkWidget *label;
  GtkWidget *button;

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_widget_show (main_vbox);
  
  frame = gtk_frame_new (_("Current Font"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
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
    
  #ifdef GTK_HAVE_FEATURES_1_1_0	
  button = gtk_button_new_with_label (_("Select..."));
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
   			  GTK_SIGNAL_FUNC(font_sel),
   			  NULL);
  
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 2);
  gtk_widget_show(button);
  #endif
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  /* label = gtk_label_new (N_("(Remember to Restart gEdit for font changes to take effect)")); As i've got fonts loading dynamically now, this label is uneeded, but it may come in useful sometime 		-- Alex
  
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);*/

  
  return main_vbox;
}

/* -------- */
#ifdef WITHOUT_GNOME
void apply_close(gE_data *data)
{
  /*gE_apply(NULL, NULL,data);*/

  cancel();

}

void gE_property_box_new(gE_data *data)
{
  GtkWidget *hbox, *vbox, *label;


 
  pbox = g_malloc(sizeof(gE_Prop_Box));
  pbox->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (GTK_WIDGET(pbox->window), 500, 400);
  
      gtk_signal_connect (GTK_OBJECT (pbox->window), "destroy",
			  GTK_SIGNAL_FUNC(cancel),
			  NULL);
  gtk_signal_connect (GTK_OBJECT (pbox->window), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_false), NULL);
		      
      gtk_window_set_title (GTK_WINDOW (pbox->window), _("Preferences"));
      gtk_container_border_width (GTK_CONTAINER (pbox->window), 0);
 
 	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pbox->window), vbox);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(GTK_BOX(hbox)), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

  pbox->notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(hbox), pbox->notebook, TRUE, TRUE, 0);
  gtk_widget_show (pbox->notebook);
 
  /* General Settings */
  label = gtk_label_new ("General");
  pbox->gen_vbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook), pbox->gen_vbox, label);
/*  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook), general_page_new(), label);*/
  gtk_widget_show(pbox->gen_vbox);
  gtk_widget_show(label);
  
  general_page_new();
  
  
  /* Print Settings */
  label = gtk_label_new ("Print");
  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook),
                               print_page_new(), label);

  /* Font Settings */
  label = gtk_label_new ("Font");
  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook),
                                           font_page_new(), label);
 gtk_widget_show (pbox->notebook);
  get_prefs(data); 
 
 
 	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	pbox->separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), pbox->separator, FALSE, TRUE, 0);
	gtk_widget_show(pbox->separator);
 
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);  
  
  pbox->ok_button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (pbox->ok_button), "clicked",
			  GTK_SIGNAL_FUNC (gE_apply),
			  data);
  gtk_signal_connect (GTK_OBJECT (pbox->ok_button), "clicked",
			  GTK_SIGNAL_FUNC (cancel),
			  NULL);
  gtk_box_pack_start(GTK_BOX(hbox), pbox->ok_button, TRUE, TRUE, 0);
  gtk_widget_show (pbox->ok_button);

  pbox->apply_button = gtk_button_new_with_label ("Apply");
  gtk_signal_connect (GTK_OBJECT (pbox->apply_button), "clicked",
			  GTK_SIGNAL_FUNC (gE_apply),
			  data);
  gtk_box_pack_start(GTK_BOX(hbox), pbox->apply_button, TRUE, TRUE, 0);
  gtk_widget_show (pbox->apply_button);

  pbox->close_button = gtk_button_new_with_label ("Close");
  gtk_signal_connect (GTK_OBJECT (pbox->close_button), "clicked",
			  GTK_SIGNAL_FUNC (cancel),
			  NULL);
  gtk_box_pack_start(GTK_BOX(hbox), pbox->close_button, TRUE, TRUE, 0);
  gtk_widget_show (pbox->close_button);
  

  gtk_widget_show_all (pbox->window);

}  
#endif


/* -------- */
#ifndef WITHOUT_GNOME
void properties_modified (GtkWidget *widget, GnomePropertyBox *pbox)
{
  gnome_property_box_changed (pbox);
}
#endif


void gE_prefs_dialog(GtkWidget *widget, gpointer cbdata)
{
  GtkWidget *label;

  
  gE_data *data = (gE_data *)cbdata;

  prefs = g_malloc (sizeof(gE_prefs_data));

 
 #ifdef WITHOUT_GNOME
    gE_property_box_new (data);
  #endif
#ifndef WITHOUT_GNOME
  if (!prefs)
    {
      /*gdk_window_raise (GTK_WIDGET (prefs->pbox)->window);*/
      return;
    }


   prefs->pbox = (GNOME_PROPERTY_BOX (gnome_property_box_new ()));

  

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "destroy",
		      GTK_SIGNAL_FUNC (cancel), prefs);

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_false), NULL);


  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "apply",
		      GTK_SIGNAL_FUNC (gE_apply), data);


  /* General Settings */
  label = gtk_label_new (_("General"));
  gtk_notebook_append_page ( GTK_NOTEBOOK( (prefs->pbox)->notebook),
                                           general_page_new(), label);

  /* Print Settings */
  label = gtk_label_new (_("Print"));
  gtk_notebook_append_page ( GTK_NOTEBOOK( (prefs->pbox)->notebook),
                                           print_page_new(), label);

  /* Font Settings */
  label = gtk_label_new (_("Font"));
  gtk_notebook_append_page ( GTK_NOTEBOOK( (prefs->pbox)->notebook),
                                           font_page_new(), label);
   
  get_prefs(data);


  gtk_signal_connect (GTK_OBJECT (prefs->autoindent), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);
  gtk_signal_connect (GTK_OBJECT (prefs->status), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->wordwrap), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->split), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->sball), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->pcmd), "changed",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->font), "changed",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_widget_show_all (GTK_WIDGET (prefs->pbox));

 #endif                                    
}

