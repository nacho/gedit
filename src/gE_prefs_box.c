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
#include <gnome.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include "main.h"
#include "gE_prefs.h"
#include "toolbar.h"
#include "gE_prefs_box.h"
#include "gE_document.h"
#include "gE_plugin_api.h"

typedef struct _gE_prefs_data {
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
	
	/* Plugins Settings */
	/* FIXME: Fix it! Damn you! */
	GtkWidget *plugin_list;
	GtkWidget *scroll_window;
	GtkWidget *plugins_toggle;
	gE_data *gData;
	
	/* Toolbar Settings */
	/* Hmm, dunno... */
	
	/* Tab Settings */
	/* Hmm, dunno... */
} gE_prefs_data;

static gE_prefs_data *prefs;
GList *plugin_list;
plugin_callback_struct pl_callbacks;

void cancel()
{
  gtk_widget_destroy (GTK_WIDGET (prefs->pbox));
  g_free(prefs);
  prefs = NULL;
}

void gE_window_refresh(gE_window *w)
{
GtkStyle *style;
gint i;
/* FIXME: This function is basically borked right now... */

/*FIXME    if (w->show_status == 0)
       gtk_widget_hide (GTK_WIDGET (w->statusbar)->parent);
     else
       gtk_widget_show (GTK_WIDGET (w->statusbar)->parent);
*/       
     /* if (w->splitscreen == TRUE) */
/*       gE_document_set_split_screen (gE_document_current(), (gint) w->splitscreen);
  */  
  
  	gE_window_set_status_bar (settings->show_status);
  
/*  style = gtk_style_new();
  gdk_font_unref (style->font);
  style->font = gdk_font_load (settings->font);
  
  gtk_widget_push_style (style);    
  for (i = 0; i < g_list_length (gE_documents); i++)
  {
  	gtk_widget_set_style(GTK_WIDGET(
  		((gE_document *) g_list_nth_data (gE_documents, i))->split_screen), style);

  	gtk_widget_set_style(GTK_WIDGET(
  		((gE_document *) g_list_nth_data (gE_documents, i))->text), style);
  }

  gtk_widget_pop_style ();
*/  	
  
}

void gE_apply(GnomePropertyBox *pbox, gint page, gE_data *data)
{
  FILE *file;
  gchar *rc;


  /* General Settings */
  settings->auto_indent = (GTK_TOGGLE_BUTTON (prefs->autoindent)->active);
  settings->show_status = (GTK_TOGGLE_BUTTON (prefs->status)->active);  
  settings->splitscreen = (GTK_TOGGLE_BUTTON (prefs->split)->active);

  /* Print Settings */
  settings->print_cmd = g_strdup (gtk_entry_get_text (GTK_ENTRY(prefs->pcmd)));
  
  /* Font Settings */
    settings->font = g_strdup (gtk_entry_get_text (GTK_ENTRY(prefs->font)));
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
	fprintf(file, "  font = \"%s\"\n", settings->font);
	fprintf(file, "}\n");
	fprintf(file, "widget_class \"*GtkText\" style \"text\"\n");
	fclose(file);  

  gE_window_refresh(data->window);
  gE_save_settings();
}



void get_prefs(gE_data *data)
{
  gint uP;
  
  gtk_entry_set_text (GTK_ENTRY (prefs->pcmd), settings->print_cmd);
  gtk_entry_set_text (GTK_ENTRY (prefs->font), settings->font);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->autoindent), 
  					   settings->auto_indent);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->status),
  					   settings->show_status);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->split),
  					   settings->splitscreen);
  					   
 
   if (!GTK_TOGGLE_BUTTON(prefs->plugins_toggle)->active)
     gtk_widget_set_sensitive (GTK_WIDGET (prefs->plugin_list), FALSE);
   else
     gtk_widget_set_sensitive (GTK_WIDGET (prefs->plugin_list), TRUE);
}

/* General UI Stuff.. */

static GtkWidget *general_page_new()
{
  GtkWidget *main_vbox, *vbox, *frame, *hbox;

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

/* End of General Stuff.. */

/* Print Stuf.. */

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

/* End of Print Stuff */


/* Font Stuff... */

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
  
  /* label = gtk_label_new (N_("(Remember to Restart gEdit for font changes to take effect)")); As i've got fonts loading dynamically now, this label is uneeded, but it may come in useful sometime 		-- Alex
  
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);*/

  
  return main_vbox;
}

/* End of Fonts Stuff... */


/* Plugins Stuff... */

static void plugins_fsel_ok (GtkWidget *w, GtkFileSelection *fs)
{
 gchar *clist_plugin [2];
 
 plugin_callback_struct callbacks;
	
	clist_plugin [0] = g_strdup (g_basename (gtk_file_selection_get_filename (fs)));
	clist_plugin [1] = g_strdup (g_dirname (gtk_file_selection_get_filename (fs)));
	
	custom_plugin_query ( clist_plugin[1], clist_plugin[0], &pl_callbacks);

 	gtk_clist_freeze (GTK_CLIST (prefs->plugin_list));
 	gtk_clist_append (GTK_CLIST (prefs->plugin_list), clist_plugin);
 	gtk_clist_thaw (GTK_CLIST (prefs->plugin_list));
	
	gtk_widget_destroy (GTK_WIDGET (fs));
}

static void plugins_clist_add (GtkWidget *w, gpointer data)
{
 GtkWidget *fsel;
 
 	fsel = gtk_file_selection_new ("Plugin Selector");
 	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (fsel));
 	
 	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fsel)->ok_button),
			  "clicked", GTK_SIGNAL_FUNC(plugins_fsel_ok),
			  fsel);
      	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fsel)->cancel_button),
				 "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (fsel));
	
	gtk_widget_show (fsel);
}

static void plugins_clist_remove (GtkWidget *w, gpointer data)
{
  int i, i2;
  plugin_list_data *plugins_data;
  gchar *path;

	/* FIXME: Needs to actually unload the plugin itself.. */

  	/* Try removing plugin from Gnome Menu.. */
  	path = g_new(gchar, strlen(_("_Plugins")) + 2);
  	sprintf(path, "%s/", _("_Plugins"));
  	
  	i  = (g_list_length (plugin_list)) - (GTK_CLIST(data)->focus_row);
  	
  	gnome_app_remove_menu_range (GNOME_APP (mdi->active_window),
  						path, i, 1);

  	for (i = 0; i < GTK_CLIST(data)->focus_row; i++);
  	   
  	   plugins_data = g_list_nth_data (plugin_list, i);
  	   
  	   plugin_list = g_list_remove (plugin_list, plugins_data);
  	
  	gtk_clist_remove (GTK_CLIST (data), GTK_CLIST (data)->focus_row);
  	plugin_save_list ();
}

static void plugins_clist_click_column (GtkCList *clist, gint column, gpointer data)
{
  if (column == 4)
    gtk_clist_set_column_visibility (clist, column, FALSE);
  else if (column == clist->sort_column)
    {
      if (clist->sort_type == GTK_SORT_ASCENDING)
	clist->sort_type = GTK_SORT_DESCENDING;
      else
	clist->sort_type = GTK_SORT_ASCENDING;
    }
  else
    gtk_clist_set_sort_column (clist, column);

  gtk_clist_sort (clist);
}

static void plugins_toggle (GtkWidget *w)
{
  gchar *plug[2];	/* Temporary plugins data */
  gchar *path;
  gint i;
  plugin_list_data *plugins_data;

  /* Plugins Settings.. (Use Plugins) */
  if (!GTK_TOGGLE_BUTTON(prefs->plugins_toggle)->active)
    {
     	/*
     	g_print ("Hrmm.. you dont seemto want to use plugins.. Thats not nice!\n");*/
     	gnome_config_set_int ("/Editor_Plugins/Use/gEdit", -1);
     	gtk_widget_set_sensitive (GTK_WIDGET (prefs->plugin_list), FALSE);
     	
     	  	/* Try removing plugin from Gnome Menu.. */
  	path = g_new(gchar, strlen(_("_Plugins")) + 2);
  	sprintf(path, "%s/", _("_Plugins"));
  	
  	gnome_app_remove_menu_range (GNOME_APP (mdi->active_window),
  						path, 1, g_list_length (plugin_list));
     	
    }
  else
    {
     	gnome_config_set_int ("/Editor_Plugins/Use/gEdit", 1);
     	gtk_widget_set_sensitive (GTK_WIDGET (prefs->plugin_list), TRUE);
     	/*gtk_clist_clear (GTK_CLIST (prefs->plugin_list));*/
     	
     	/*for (i = 0; i < g_list_length (plugin_list); i++);
  	   {
  	    g_print ("bink %d",i);
  	    plugins_data = g_list_nth_data (plugin_list, i);
  	   
  	    plugin_list = g_list_remove (plugin_list, plugins_data);
 	   }*/
 	   /*g_list_free (plugin_list);
 	   plugin_list = g_list_alloc();
 	   */
 	   plugin_list = NULL;
 	   
 	plugin_load_list("gEdit");
 	
  		
    }

}

static GtkWidget *plugins_page_new()
{
  GtkWidget *main_vbox, *vbox, *hbox, *frame, *box;
  GtkWidget *label;
  GtkWidget *button;
  gchar clist_plugins_data [5][80];
  gchar *clist_plugins [2];
  plugin_list_data *plugins_data;
  int i, uP;
  
  static char *titles[] =
  {
    N_("Name"), N_("Location")
  };
  
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_widget_show (main_vbox);
  
  frame = gtk_frame_new (_("Plugins"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);
  

  prefs->plugins_toggle = gtk_check_button_new_with_label ("Use Plugins");
   uP = gnome_config_get_int ("/Editor_Plugins/Use/gEdit");
  if (uP == -1)
    uP = 0;
    
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->plugins_toggle),
  					   uP);
  gtk_signal_connect (GTK_OBJECT (prefs->plugins_toggle), "clicked",
		      GTK_SIGNAL_FUNC (plugins_toggle), NULL);
  
  gtk_box_pack_start (GTK_BOX(hbox), prefs->plugins_toggle, FALSE, TRUE, 0);
  gtk_widget_show (prefs->plugins_toggle);

#ifdef ENABLE_NLS
  titles[0]=_(titles[0]);
  titles[1]=_(titles[1]);
#endif
  
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);
  
  prefs->plugin_list = gtk_clist_new_with_titles (2, titles);
  gtk_widget_show (prefs->plugin_list);

  prefs->scroll_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (prefs->scroll_window),
				      GTK_POLICY_AUTOMATIC, 
				      GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (prefs->scroll_window), prefs->plugin_list);

  gtk_signal_connect (GTK_OBJECT (prefs->plugin_list), "click_column",
			  (GtkSignalFunc) plugins_clist_click_column, (gpointer) prefs->plugin_list); 

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(box), 5);
  gtk_box_pack_start(GTK_BOX(vbox), box, TRUE, TRUE, 0);
  gtk_widget_show(box);
  
  gtk_clist_set_column_auto_resize (GTK_CLIST (prefs->plugin_list), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (prefs->plugin_list), 1, TRUE);
  
   for (i = 0; i < g_list_length (plugin_list) ; i++) 
     {
       plugins_data = g_list_nth_data (plugin_list, i);
       
       /*
       sprintf (clist_plugins[0], "%s", plugins_data->name);
       sprintf (clist_plugins[1], "%s", plugins_data->location);
       */
       clist_plugins[0] = g_strdup (plugins_data->name);
       clist_plugins[1] = g_strdup (plugins_data->location);
       
       gtk_clist_append (GTK_CLIST (prefs->plugin_list), clist_plugins);
     }
  
  
   gtk_container_set_border_width (GTK_CONTAINER (prefs->scroll_window), 5);
   gtk_box_pack_start (GTK_BOX (box), prefs->scroll_window, TRUE, TRUE, 0);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 10);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);
  
  button = gtk_button_new_with_label (_("Add"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) plugins_clist_add, (gpointer) prefs->plugin_list);

  button = gtk_button_new_with_label (_("Remove"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) plugins_clist_remove, (gpointer) prefs->plugin_list);
  
  return main_vbox;
}

/* End of Plugins Stuff... */

void properties_modified (GtkWidget *widget, GnomePropertyBox *pbox)
{
  gnome_property_box_changed (pbox);
}

void gE_prefs_dialog(GtkWidget *widget, gpointer cbdata)
{
  static GnomeHelpMenuEntry help_entry = { NULL, "properties" };
  GtkWidget *label;

  
  gE_data *data = (gE_data *)cbdata;

  prefs = g_malloc (sizeof(gE_prefs_data));

  if (!prefs)
    {
      /*gdk_window_raise (GTK_WIDGET (prefs->pbox)->window);*/
      return;
    }


   prefs->pbox = (GNOME_PROPERTY_BOX (gnome_property_box_new ()));
   prefs->gData = data;
  

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "destroy",
		      GTK_SIGNAL_FUNC (cancel), prefs);

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_false), NULL);

  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "apply",
		      GTK_SIGNAL_FUNC (gE_apply), data);

  help_entry.name = gnome_app_id;
  gtk_signal_connect (GTK_OBJECT (prefs->pbox), "help",
		      GTK_SIGNAL_FUNC (gnome_help_pbox_display),
		      &help_entry);


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
  
  /* Plugins Settings */
  label = gtk_label_new (_("Plugins"));
  gtk_notebook_append_page ( GTK_NOTEBOOK( (prefs->pbox)->notebook),
                                           plugins_page_new(), label);
    
  get_prefs(data);


  gtk_signal_connect (GTK_OBJECT (prefs->autoindent), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);
  gtk_signal_connect (GTK_OBJECT (prefs->status), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->wordwrap), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->split), "toggled",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->pcmd), "changed",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);

  gtk_signal_connect (GTK_OBJECT (prefs->font), "changed",
		      GTK_SIGNAL_FUNC (properties_modified), prefs->pbox);
  
  
  gtk_widget_show_all (GTK_WIDGET (prefs->pbox));
                                    
}

