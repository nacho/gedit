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
#include <time.h>
#include <string.h>
#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
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
static gE_Prop_Box *pbox;
#endif

void cancel()
{
  gtk_widget_destroy (GTK_WIDGET (pbox));
  g_free(prefs);
  prefs = NULL;
}

void gE_window_refresh(gE_window *w)
{
     if (w->show_status == FALSE)
       gtk_widget_hide (w->statusbox);
     else
       gtk_widget_show (w->statusbox);
     
}

#ifndef WITHOUT_GNOME
void gE_apply(GnomePropertyBox *pbox, gint page, gE_data *data)
{
  FILE *file;
  gchar *rc;


  
  /* General Settings */
  data->window->auto_indent = GTK_TOGGLE_BUTTON (prefs->autoindent)->active;
  data->window->show_status = GTK_TOGGLE_BUTTON (prefs->status)->active;  

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

#else

void gE_gtk_apply (gE_Prop_Box *pbox, gint page, gE_data *data)
{
  FILE *file;
  gchar *rc;


  printf("eh\n");
  /* General Settings */
  data->window->auto_indent = GTK_TOGGLE_BUTTON (prefs->autoindent)->active;
  data->window->show_status = GTK_TOGGLE_BUTTON (prefs->status)->active;  
printf("eh.\n");
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
printf("huh\n");
  gE_window_refresh(data->window);
  gE_save_settings(data->window, data->window);
}
#endif

void get_prefs(gE_data *data)
{
  gtk_entry_set_text (GTK_ENTRY (prefs->pcmd), data->window->print_cmd);
  gtk_entry_set_text (GTK_ENTRY (prefs->font), data->window->font);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->autoindent), 
  					   data->window->auto_indent);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->status),
  					   data->window->show_status);
}

static GtkWidget *general_page_new()
{
  GtkWidget *vbox, *hbox;

printf("gpn...\n");

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (vbox), 10);
  #ifdef WITHOUT_GNOME
  gtk_box_pack_start (GTK_BOX(pbox->gen_vbox), vbox, TRUE, TRUE, 0);
  #endif
  gtk_widget_show (vbox);
  printf("gpn..2.\n");

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  printf("gpn..3.\n");
  prefs->autoindent = gtk_check_button_new_with_label ("Autoindent");
  printf("gpn..3.1.\n");
  gtk_box_pack_start(GTK_BOX(hbox), prefs->autoindent, TRUE, TRUE, 0);
    printf("gpn..3.2.\n");
  gtk_widget_show (prefs->autoindent);
  printf("gpn..4.\n");
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  prefs->status = gtk_check_button_new_with_label (N_("Statusbar"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->status, TRUE, TRUE, 0);
  gtk_widget_show (prefs->status);
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  prefs->wordwrap = gtk_check_button_new_with_label (N_("Wordwrap"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->wordwrap, TRUE, TRUE, 0);
  gtk_widget_show (prefs->wordwrap);
  
    hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  prefs->split = gtk_check_button_new_with_label (N_("Splitscreen"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->split, TRUE, TRUE, 0);
  gtk_widget_show (prefs->split);
  
    hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  prefs->sball = gtk_check_button_new_with_label (N_("Scrollball"));
  gtk_box_pack_start(GTK_BOX(hbox), prefs->sball, TRUE, TRUE, 0);
  gtk_widget_show (prefs->sball);
  
  return vbox;
}

static GtkWidget *print_page_new()
{
  GtkWidget *vbox, *hbox;
  GtkWidget *label;

  vbox = gtk_vbox_new(FALSE, 0);
  /*gtk_container_border_width (GTK_CONTAINER (vbox), 10);*/
  gtk_widget_show (vbox);
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  label = gtk_label_new (N_("Print Command"));
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  
  prefs->pcmd = gtk_entry_new ();
  gtk_box_pack_start(GTK_BOX(hbox), prefs->pcmd, TRUE, TRUE, 0);
  gtk_widget_show (prefs->pcmd);
  
  return vbox;
}


void font_sel_ok (GtkWidget	*w, GtkWidget *fsel)
{
  gtk_entry_set_text(GTK_ENTRY(prefs->font),
       gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG(fsel)));
  gtk_widget_destroy (fsel);
}

void font_sel_cancel (GtkWidget *w, GtkWidget *fsel)
{
  gtk_widget_destroy (fsel);
}

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
      

static GtkWidget *font_page_new()
{
  GtkWidget *vbox, *hbox;
  GtkWidget *label;
  GtkWidget *button;

  vbox = gtk_vbox_new(FALSE, 0);
  /*gtk_container_border_width (GTK_CONTAINER (vbox), 10);*/
  gtk_widget_show (vbox);
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  label = gtk_label_new (N_("Current Font"));
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  
  prefs->font = gtk_entry_new ();
  gtk_box_pack_start(GTK_BOX(hbox), prefs->font, TRUE, TRUE, 0);
  gtk_widget_show (prefs->font);
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
    
  button = gtk_button_new_with_label ("Select Font");
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
   			  GTK_SIGNAL_FUNC(font_sel),
   			  NULL);
  
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_widget_show(button);  

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  label = gtk_label_new (N_("(Remember to Restart gEdit for font changes to take effect)"));
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  
  return vbox;
}

/* -------- */
#ifdef WITHOUT_GNOME
void apply_close(gE_data *data)
{
  gE_gtk_apply(pbox, 1, data);
  
  cancel();

}

void gE_property_box_new(gE_data *data)
{
  GtkWidget *hbox, *vbox, *label;


 
  printf(".\n");
  pbox = g_malloc(sizeof(gE_Prop_Box));
printf("..\n");
  pbox->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
printf(".X.\n");
      gtk_signal_connect (GTK_OBJECT (pbox->window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  pbox->window);
printf(".!.\n");
      gtk_window_set_title (GTK_WINDOW (pbox->window), "gedit");
      gtk_container_border_width (GTK_CONTAINER (pbox->window), 0);
 printf("..\n");     
 
 	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pbox->window), vbox);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);
 printf("..\n");
  pbox->notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(hbox), pbox->notebook, TRUE, TRUE, 0);
  gtk_widget_show (pbox->notebook);
 
  printf("..\n");
  /* General Settings */
  label = gtk_label_new ("General");
  pbox->gen_vbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook), pbox->gen_vbox, label);
  gtk_widget_show(pbox->gen_vbox);
  gtk_widget_show(label);
  
  general_page_new();
  
  
 printf(".t.\n");
  /* Print Settings */
  label = gtk_label_new ("Print");
  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook),
                               print_page_new(), label);
 printf("e..\n");                                          
  /* Font Settings */
  label = gtk_label_new ("Font");
  gtk_notebook_append_page ( GTK_NOTEBOOK (pbox->notebook),
                                           font_page_new(), label);
 gtk_widget_show (pbox->notebook);
  printf("..e\n");  
  get_prefs(data); 
 
  printf("ARGH\n");
 
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
  
  pbox->ok_button = gtk_button_new_with_label ("Ok");
  gtk_signal_connect (GTK_OBJECT (pbox->ok_button), "clicked",
  		      GTK_SIGNAL_FUNC (apply_close),
			  data);
  gtk_box_pack_start(GTK_BOX(hbox), pbox->ok_button, TRUE, TRUE, 0);
  gtk_widget_show (pbox->ok_button);

  pbox->apply_button = gtk_button_new_with_label ("Apply");
  gtk_signal_connect (GTK_OBJECT (pbox->apply_button), "clicked",
			  GTK_SIGNAL_FUNC (gE_gtk_apply),
			  data);
  gtk_box_pack_start(GTK_BOX(hbox), pbox->apply_button, TRUE, TRUE, 0);
  gtk_widget_show (pbox->apply_button);

  pbox->close_button = gtk_button_new_with_label ("Close");
  gtk_signal_connect (GTK_OBJECT (pbox->close_button), "clicked",
			  GTK_SIGNAL_FUNC (cancel),
			  NULL);
  gtk_box_pack_start(GTK_BOX(hbox), pbox->close_button, TRUE, TRUE, 0);
  gtk_widget_show (pbox->close_button);
  

  gtk_widget_show (pbox->window);

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
#ifndef WITHOUT_GNOME
  GtkWidget *label;
#endif
  
  gE_data *data = (gE_data *)cbdata;

  prefs = g_malloc (sizeof(gE_prefs_data));
  
  #ifdef WITHOUT_GNOME
    gE_property_box_new (data);
  #endif
#ifndef WITHOUT_GNOME
  if (prefs)
    {
      gdk_window_raise (GTK_WIDGET (GNOME_DIALOG (prefs->pbox))->window);
      return;
    }

  

   prefs->pbox = GNOME_PROPERTY_BOX (gnome_property_box_new ());
  

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "destroy",
		      GTK_SIGNAL_FUNC (cancel), prefs);

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_false), NULL);


  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "apply",
		      GTK_SIGNAL_FUNC (gE_apply), data);

 
  /* General Settings */
  label = gtk_label_new ("General");
  gtk_notebook_append_page ( GTK_NOTEBOOK( (prefs->pbox)->notebook),
                                           general_page_new(), label);

  /* Print Settings */
  label = gtk_label_new ("Print");
  gtk_notebook_append_page ( GTK_NOTEBOOK( (prefs->pbox)->notebook),
                                           print_page_new(), label);
                                             
  /* Font Settings */
  label = gtk_label_new ("Font");
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

  gtk_widget_show_all (prefs->pbox);    
  #endif                                    
}

