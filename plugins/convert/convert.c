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
 * [X] libglade-ify me
 */

#include <config.h>
#include <gnome.h>
#include <glade/glade.h>


#include "../../src/plugin.h"

static GtkWidget *from, *to;

void convert_plugin (void);

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
conv_hex (GtkWidget *widget, gpointer data)
{
	int start;
	gchar *value;

	start = atoi (gtk_entry_get_text (GTK_ENTRY (from)));
  	value = g_strdup_printf ("%X", start);
	
	gtk_entry_set_text (GTK_ENTRY(to), value);

	g_free (value);
}

static void
conv_oct (GtkWidget *widget, gpointer data)
{
	int start;
	gchar *value;

	start = atoi (gtk_entry_get_text( GTK_ENTRY( from ) ));
  	value = g_strdup_printf ("%o", start);
	gtk_entry_set_text(GTK_ENTRY(to), value);

	g_free (value);
}

static void
conv_dec (GtkWidget *widget, gpointer data)
{
	long start;
	gchar *value;

	start = strtol (gtk_entry_get_text (GTK_ENTRY (from)), NULL, 16);

	value = g_strdup_printf ("%lui", start);
	gtk_entry_set_text (GTK_ENTRY(to), value);

	g_free (value);
}

static void
destroy_convert_dialog (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (data);
}
void
convert_plugin (void)
{
	GladeXML *gui;
	GtkWidget *dialog;
	GtkWidget *dectohex;
	GtkWidget *dectooct;
	GtkWidget *hextodec;
	GtkWidget *button;

	gui = glade_xml_new (GEDIT_GLADEDIR "/convert.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find convert.glade");
		return;
	}

	
	dialog		= glade_xml_get_widget (gui, "dialog1");
	to 		= glade_xml_get_widget (gui, "to");
	from 		= glade_xml_get_widget (gui, "from");
	dectohex 	= glade_xml_get_widget (gui, "dectohex");
	dectooct 	= glade_xml_get_widget (gui, "dectooct");
	hextodec 	= glade_xml_get_widget (gui, "hextodec");
	button		= glade_xml_get_widget (gui, "button6");



	gtk_signal_connect (GTK_OBJECT (dectohex),
			    "clicked",
			    GTK_SIGNAL_FUNC (conv_hex),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (dectooct),
			    "clicked",
			    GTK_SIGNAL_FUNC( conv_oct ),
			    NULL );

	gtk_signal_connect (GTK_OBJECT (hextodec),
			    "clicked",
			    GTK_SIGNAL_FUNC( conv_dec ),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (dialog),
			    "destroy",
			    GTK_SIGNAL_FUNC (destroy_convert_dialog),
			    dialog);

	gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				   GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (dialog));

	gtk_widget_show_all (dialog);

	gtk_object_unref (GTK_OBJECT (gui));

}

gint
init_plugin (PluginData *pd)
{
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Convert");
	pd->desc = _("Number Converter");
	pd->long_desc = _("Number Converter");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->needs_a_document = FALSE;

	pd->private_data = (gpointer) convert_plugin;

	return PLUGIN_OK;
}
