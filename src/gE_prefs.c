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
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>	
#include <string.h>	
#include <sys/stat.h>	
#include <unistd.h>	
#include <sys/types.h>	
#include <glib.h>	

 #include "main.h"
 #include "gE_prefs.h"
 
gE_prefs *prefs_window;
	gE_prefs *prefs;

 
 gchar *prefs_filename = NULL;
 gint line;
 gint pos;
 
 char home_dir[STRING_LENGTH_MAX];
 char *home2;
 char rc[STRING_LENGTH_MAX];
 char *rcfile = "/.gedit";
 
 char *gettime(char *buf)
 {
   time_t clock;
   struct tm *now;
   char *buffer;
   
    buffer = buf;
   clock = time (NULL);
   now = gmtime(&clock);
   
   sprintf(buffer, "%04d-%02d-%02d%c%02d:%02d:%02d%c",
	  now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
	  (FALSE ? 'T' : ' '),
	  now->tm_hour, now->tm_min, now->tm_sec,
	  (FALSE ? 'Z' : '\000'));
   return buffer;
 }
 
 void gE_get_rc_file()
 {
	/* Having all this code here looks a lot neater than before! And it's there if we need to change anything -Alex */
	home2 = getenv("HOME");
	#ifdef DEBUG
	g_print("Home Dir: %s\n", home2);
	#endif
	strcpy(rc, "");
	strcpy(home_dir, home2);
	strcat(rc, home_dir);
	strcat(rc, rcfile);
		#ifdef DEBUG
		g_print("rc: %s\n\n", rc);
		#endif
  }

 void gE_rc_parse()
 {
/*	gE_get_rc_file();*/

/*#ifdef HAVE_GTK_RC_REPARSE_ALL*/
		home2 = getenv("HOME");
	#ifdef DEBUG
	g_print("Home Dir: %s\n", home2);
	#endif
	strcpy(rc, "");
	strcpy(home_dir, home2);
	strcat(rc, home_dir);
	strcat(rc, rcfile);
		#ifdef DEBUG
		g_print("rc: %s\n\n", rc);
		#endif
		
   	gtk_rc_parse (rc);
/*   	gtk_rc_reparse_all();

   toplevels = gdk_window_get_toplevels();
  while (toplevels)
    {
      GtkWidget *widget;
      gdk_window_get_user_data (toplevels->data, (gpointer *)&widget);
      
      if (widget)
	gtk_widget_reset_rc_styles (widget);

      toplevels = toplevels->next;
    }

  g_list_free (toplevels);

#endif*/
 }
 
 void ok_prefs(GtkWidget *widget, gE_window *window)
 {
    FILE *file;
    char stamp[50];
    /* GtkStyle *style; */

 	
    
	gE_get_rc_file();

     file = fopen(rc,"w");/*
     file = fopen("./gE_rc","w");*/
     if (file == NULL) {
       g_print ("Cannot open %s!\n",rc);
       return;
     }
/*     while ((gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs_window->tfont)->entry))) != NULL )
     {*/
     strcpy (stamp, "Modified: ");
     gettime(stamp + strlen (stamp));
     fprintf (file,"# gEdit rc file...\n");
     fprintf (file,"#\n");
     fprintf (file,"# %s\n", stamp);
     fprintf (file,"#\n\n");
     fprintf (file,"style \"text\"\n");
     fprintf (file,"{\n");
     fprintf (file,"  font = \"-*-%s-medium-%s-%s--%s-*-*-*-*-*-*-*\"\n",
     		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs_window->tfont)->entry)),
		/*gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs_window->tweight)->entry)),*/
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs_window->tslant)->entry)),
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs_window->tweight)->entry)),
		gtk_entry_get_text(GTK_ENTRY(prefs_window->tsize)));
     fprintf (file,"}\n");
     fprintf (file,"widget_class \"*GtkText\" style \"text\"\n");
/*     }*/
     fclose (file);
 
    /*style = gtk_style_new();
    style->font = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs_window->tfont)->entry));
    gtk_widget_set_style (window->text, style);
    		Argh! It doesn't work, dunno if it is possible to do tho...
					- Alex */
	

#ifndef WITHOUT_GNOME
	 gE_save_settings(window, gtk_entry_get_text(GTK_ENTRY (prefs->pcmd)));
#endif

    gtk_widget_destroy(prefs_window->window);
    prefs_window->window = NULL;
 }
 
 void cancel_prefs(GtkWidget *widget, gpointer *data)
 {
    gtk_widget_destroy(prefs_window->window);
    prefs_window->window = NULL;
 }
 
 gE_prefs *gE_prefs_window(gE_window *window)
 {



	GList *fonts = NULL;
	GList *weight = NULL;
	GList *slant = NULL;
	

	
	fonts = g_list_append(fonts, "courier");
	fonts = g_list_append(fonts, "helvetica");

		weight = g_list_append(weight, "*");
		weight = g_list_append(weight, "normal");
		weight = g_list_append(weight, "bold");
		weight = g_list_append(weight, "medium");


		slant = g_list_append(slant, "*");
		slant = g_list_append(slant, "i");
		slant = g_list_append(slant, "r");
		
 
	prefs = g_malloc (sizeof(gE_prefs));
 	prefs->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
 
       gtk_signal_connect (GTK_OBJECT (prefs->window), "destroy",
			   GTK_SIGNAL_FUNC(cancel_prefs),
			   &prefs->window);
      gtk_signal_connect (GTK_OBJECT (prefs->window), "delete_event",
			  GTK_SIGNAL_FUNC(cancel_prefs),
			  &prefs->window);
 
 	/*prefs_window = gtk_dialog_new ();*/
	gtk_window_set_title (GTK_WINDOW (prefs->window), ("Prefs"));
	/*gtk_widget_set_usize (GTK_WIDGET (prefs->window), 370, 170);*/
	gtk_container_border_width (GTK_CONTAINER (prefs->window), 0);
	
/*	frame = gtk_frame_new (("Font"));
	gtk_container_border_width(GTK_CONTAINER (frame), 10);*/
	
	prefs->abox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (prefs->window), prefs->abox);
	/*gtk_container_add (GTK_CONTAINER(frame), prefs->abox);*/
	gtk_widget_show (prefs->abox);
	prefs->bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);
	
	prefs->label = gtk_label_new(("Font:"));
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->label, TRUE, TRUE, 0);
	gtk_widget_show (prefs->label);
	
	prefs->tfont = gtk_combo_new();
	gtk_combo_set_popdown_strings (GTK_COMBO (prefs->tfont), fonts);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (prefs->tfont)->entry), 
			    "courier");
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->tfont, TRUE, TRUE, 0);
	gtk_widget_show (prefs->tfont);

	prefs->bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);


	prefs->label = gtk_label_new(("Size:"));
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->label, TRUE, TRUE, 0);
	gtk_widget_show (prefs->label);
	
	prefs->tsize = gtk_entry_new();
	gtk_entry_set_text (GTK_ENTRY (prefs->tsize), "12");
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->tsize, TRUE, TRUE, 0);
	gtk_widget_show (prefs->tsize);

	prefs->bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);

	prefs->label = gtk_label_new(("Weight:"));
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->label, TRUE, TRUE, 0);
	gtk_widget_show (prefs->label);
	
	prefs->tweight = gtk_combo_new();
	gtk_combo_set_popdown_strings (GTK_COMBO (prefs->tweight), weight);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (prefs->tweight)->entry), "normal");
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->tweight, TRUE, TRUE, 0);
	gtk_widget_show (prefs->tweight);

	prefs->bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);

	
	prefs->label = gtk_label_new(("Slant:"));
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->label, TRUE, TRUE, 0);
	gtk_widget_show (prefs->label);
	
	prefs->tslant = gtk_combo_new();
	gtk_combo_set_popdown_strings (GTK_COMBO (prefs->tslant), slant);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (prefs->tslant)->entry), "r");
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->tslant, TRUE, TRUE, 0);
	gtk_widget_show (prefs->tslant);

	prefs->bbox = gtk_hbox_new (FALSE, 10);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);

	prefs->separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->separator, FALSE, TRUE, 0);
	gtk_widget_show (prefs->separator);
	
	prefs->bbox = gtk_hbox_new (FALSE, 10);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);
			

	prefs->bbox = gtk_hbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);
	
		prefs->label = gtk_label_new(("Print Command:"));
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->label, TRUE, TRUE, 0);
	gtk_widget_show (prefs->label);
	
	prefs->pcmd = gtk_entry_new();
	gtk_entry_set_text (GTK_ENTRY (prefs->pcmd), window->print_cmd);
	gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->pcmd, TRUE, TRUE, 0);
	gtk_widget_show (prefs->pcmd);
      
/*      prefs->synhi = gtk_check_button_new_with_label("Syntax Highlighting");
      gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->synhi, FALSE, TRUE, 0);
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(prefs->synhi), FALSE);
      gtk_widget_show (prefs->synhi);
      
      prefs->indent = gtk_check_button_new_with_label("Auto Indent");
      gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->indent, FALSE, TRUE, 0);
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(prefs->indent), TRUE);
      gtk_widget_show (prefs->indent);
	*/
	prefs->separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->separator, FALSE, TRUE, 0);
	gtk_widget_show (prefs->separator);
	
	prefs->bbox = gtk_hbox_new (FALSE, 10);
	gtk_container_border_width (GTK_CONTAINER (prefs->bbox), 10);
	gtk_box_pack_start (GTK_BOX (prefs->abox), prefs->bbox, TRUE, TRUE, 0);
	gtk_widget_show (prefs->bbox);

#ifdef WITHOUT_GNOME
	prefs->button = gtk_button_new_with_label ("Ok");
#else
	prefs->button = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
#endif
	
   gtk_signal_connect (GTK_OBJECT (prefs->button), "clicked",
		       GTK_SIGNAL_FUNC (ok_prefs), window);
   gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->button, TRUE, TRUE, 0);
   GTK_WIDGET_SET_FLAGS (prefs->button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default (prefs->button);
   gtk_widget_show (prefs->button);

#ifdef WITHOUT_GNOME
   prefs->button = gtk_button_new_with_label ("Cancel");
#else
   prefs->button = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
   gtk_signal_connect (GTK_OBJECT (prefs->button), "clicked",
		       GTK_SIGNAL_FUNC (cancel_prefs), (gpointer) "button");
   gtk_box_pack_start (GTK_BOX (prefs->bbox), prefs->button, TRUE, TRUE, 0);
   gtk_widget_show (prefs->button);
	gtk_widget_show (prefs->window);
  return prefs;
}

#ifndef WITHOUT_GNOME

void gE_save_settings(gE_window *window, gchar *cmd)
{
  /*char cmd[256];*/

  gnome_config_push_prefix ("/gEdit/Global/");

  gnome_config_set_int ("tab pos", (gint) window->tab_pos);
  gnome_config_set_int ("auto indent", (gint) window->auto_indent);
  gnome_config_set_int ("show statusbar", (gint) window->show_status);
  gnome_config_set_int ("toolbar", (gint) window->have_toolbar);

  if (window->print_cmd == "")
    gnome_config_set_string ("print command", "lpr -rs ");
  else
    /*gnome_config_set_string ("print command", window->print_cmd);*/
    gnome_config_set_string ("print command", cmd);


  gnome_config_pop_prefix ();
  gnome_config_sync ();

}

void gE_get_settings(gE_window *window)
{
  gnome_config_push_prefix ("/gEdit/Global/");
  
  window->tab_pos = gnome_config_get_int ("tab pos");
  window->show_status = gnome_config_get_int ("show statusbar");
  window->have_toolbar = gnome_config_get_int ("toolbar");
  
 window->print_cmd = gnome_config_get_string("print command");



  gnome_config_pop_prefix ();
  gnome_config_sync ();
 
  if (window->tab_pos == 0) window->tab_pos = 2;
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK(window->notebook), window->tab_pos);
  
  if (window->show_status != 1)
  {
    gtk_widget_hide (window->statusbox);
    window->show_status = 0;
  }
 
  if (window->have_toolbar == 1)
    {
     tb_on_cb(NULL,window);
     window->have_toolbar = 1;
    }
  if (window->have_toolbar == 0)
    {
     tb_off_cb(NULL, window);
     window->have_toolbar = 0;
    }
  
}
#endif
