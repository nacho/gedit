/* convert.c - number conversion plugin.
 *
 * Copyright (C) 1998 Alex Roberts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

/* TODO: 
 * [ ] libglade-ify me
 */

#include <config.h>
#include <gnome.h>

#include "../../src/plugin.h"


static GtkWidget *entry1;
static GtkWidget *entry2;

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
conv_hex (GtkWidget *widget, gpointer data)
{
	int start;
	char value[255];

	start = atoi (gtk_entry_get_text (GTK_ENTRY (entry1)));
  
	sprintf (value,"%X",start);
	gtk_entry_set_text (GTK_ENTRY(entry2), value);
}

static void
conv_oct (GtkWidget *widget, gpointer data)
{
	int start;
	char value[255];

	start = atoi (gtk_entry_get_text( GTK_ENTRY( entry1 ) ));
  
	sprintf(value,"%o",start);
	gtk_entry_set_text(GTK_ENTRY(entry2), value);
}

static void
conv_dec (GtkWidget *widget, gpointer data)
{
	long start;
	char value[255];

	start = strtol (gtk_entry_get_text (GTK_ENTRY (entry1)), NULL, 16);

	sprintf (value,"%d",start);
	gtk_entry_set_text (GTK_ENTRY(entry2), value);
}

static void
close_plugin (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (data);
}

void
convert_plugin (void)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *dialog;
	GtkWidget *label;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW( dialog ), "Number Converter");
	gtk_container_set_border_width( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), 10);


	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new ("Value to be converted:");
	gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 0 );
	entry1 = gtk_entry_new();
	gtk_box_pack_start( GTK_BOX( hbox ), entry1, TRUE, TRUE, 0 );

  
	entry2 = gtk_entry_new();
	gtk_box_pack_start( GTK_BOX( hbox ), entry2, TRUE, TRUE, 0 );


	vbox = gtk_vbox_new (FALSE, 0);
	button = gtk_button_new_with_label ("Convert Decimal to Hex");
	gtk_signal_connect (GTK_OBJECT (button),
			    "clicked",
			    GTK_SIGNAL_FUNC (conv_hex), NULL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox ), button, FALSE, TRUE, 0 );

	button = gtk_button_new_with_label ("Convert Decimal to Octal");
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC( conv_oct ), NULL );
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), button, FALSE, TRUE, 0 );

	button = gtk_button_new_with_label( "Convert Hex to Decimal" );
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC( conv_dec ), NULL );
	gtk_box_pack_start (GTK_BOX( GTK_DIALOG( dialog )->vbox ), button, FALSE, TRUE, 0 );

	button = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
	gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				   GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (dialog));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), button, FALSE, TRUE, 0);

	gtk_widget_show_all (dialog);
}

gint
init_plugin (PluginData *pd)
{
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Convert");
	pd->desc = _("Number Converter");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";

	pd->private_data = (gpointer) convert_plugin;

	return PLUGIN_OK;
}
