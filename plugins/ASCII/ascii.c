/* gedit - ASCII table plugin
 * Copyright (C) 2001 Paolo Maggi
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
 * 
 * Authors: Paolo Maggi <maggi@athena.polito.it>
 *
 */
#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "window.h"
#include "document.h"
#include "utils.h"

#include "view.h"
#include "plugin.h"

#define GEDIT_PLUGIN_GLADE_FILE "/asciitable.glade"

static char *names[33] = { 
	"NUL", 
	"SOH",
	"STX",
	"ETX",
	"EOT",
	"ENQ",
	"ACK",
	"BEL",
	"BS",
	"HT",
	"LF",
	"VT",
	"FF",
	"CR",
	"SO",
	"SI",
	"DLE",
	"DC1",
	"DC2",
	"DC3",
	"DC4",
	"NAK",
	"SYN",
	"ETB",
	"CAN",
	"EM",
	"SUB",
	"ESC",
	"FS",
	"GS",
	"RS",
	"US",
	"SPACE"
	};

static GtkWidget *dialog = NULL;
static GtkWidget* ascii_table = NULL;

static gint selected_row = 0;

static void
destroy_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");
}

static void
gedit_plugin_finish (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	gnome_dialog_close (GNOME_DIALOG (widget));
	dialog = NULL;
}

static void
close_button_pressed (GtkWidget *widget, GtkWidget* data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	gnome_dialog_close (GNOME_DIALOG (data));
	dialog = NULL;
}

static void
help_button_pressed (GtkWidget *widget, gpointer data)
{
	/* FIXME: Paolo - change to point to the right help page */
	static GnomeHelpMenuEntry help_entry = { "gedit", "plugins.html" };
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	gnome_help_display (NULL, &help_entry);
}

static void
insert_char (gint i)
{
	GeditView *view = gedit_view_active();
	gchar ch[4];
	
	gedit_debug (DEBUG_PLUGINS, "");

	if (!view)
	     return;

	g_return_if_fail ((i >=0) && (i < 256));

	sprintf (ch, "%c", i);
	gedit_document_insert_text (view->doc, ch, gedit_view_get_position (view), TRUE);	
}

static void
insert_char_button_pressed (GtkWidget *widget, GtkWidget* data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	insert_char (selected_row);
}

static void
ascii_table_row_selected (GtkCList *clist, gint row, gint column, 
		          GdkEventButton *event, gpointer user_data)
{
	gedit_debug (DEBUG_PLUGINS, "");
	
	selected_row = row;
	
	if (!event) 
		return;

	if (event->type == GDK_2BUTTON_PRESS)
	{
		insert_char (selected_row);
	}
}

static void
gedit_plugin_create_dialog (void)
{
	GladeXML *gui = NULL;
	GtkWidget *close_button = NULL;
	GtkWidget *help_button = NULL;
	GtkWidget *insert_char_button = NULL;
		
	gchar *items[4];
	gchar ch[5];
	gchar dec[5];
	gchar hex[5];
 	
	gint i;

	if (dialog != NULL)
	{
		g_return_if_fail (GTK_WIDGET_REALIZED (dialog));
		g_return_if_fail (ascii_table != NULL);

		gdk_window_show (dialog->window);
		gdk_window_raise (dialog->window);
		
		return;		
	}	

	gui = glade_xml_new (GEDIT_GLADEDIR
			     GEDIT_PLUGIN_GLADE_FILE,
			     "dialog");
	
	if (!gui) {
		g_warning ("Could not find %s, reinstall gedit.\n",
			   GEDIT_GLADEDIR
			   GEDIT_PLUGIN_GLADE_FILE);
		return;
	}

	dialog             = glade_xml_get_widget (gui, "dialog");
	ascii_table        = glade_xml_get_widget (gui, "ascii_table");
	close_button       = glade_xml_get_widget (gui, "close_button");
	help_button        = glade_xml_get_widget (gui, "help_button");
	insert_char_button = glade_xml_get_widget (gui, "insert_char_button");

	g_return_if_fail (dialog             != NULL);
	g_return_if_fail (ascii_table        != NULL);
	g_return_if_fail (close_button       != NULL);
	g_return_if_fail (help_button        != NULL);
	g_return_if_fail (insert_char_button != NULL);

	gtk_clist_column_titles_passive (GTK_CLIST (ascii_table));
	gtk_clist_set_column_width (GTK_CLIST(ascii_table), 0, 60);
	gtk_clist_set_column_width (GTK_CLIST(ascii_table), 1, 60);
	gtk_clist_set_column_width (GTK_CLIST(ascii_table), 2, 60);
	gtk_clist_set_column_width (GTK_CLIST(ascii_table), 3, 60);

	gtk_clist_set_column_resizeable	(GTK_CLIST(ascii_table), 0, FALSE);
	gtk_clist_set_column_resizeable	(GTK_CLIST(ascii_table), 1, FALSE);
	gtk_clist_set_column_resizeable	(GTK_CLIST(ascii_table), 2, FALSE);
	gtk_clist_set_column_resizeable	(GTK_CLIST(ascii_table), 3, FALSE);

	for (i=0; i<256; ++i)
	{
		sprintf (ch, "%3c", i);
		sprintf (dec, "%3d", i);
		sprintf (hex, "%2.2X", i);
		
		items[0] = ch;
		items[1] = dec;
		items[2] = hex;
		
		if (i < 33)
			items[3] = names[i];
		else
		{
			if (i == 127)
				items[3] = "DEL";
			else
				items[3] = "";
		}
			
		gtk_clist_append (GTK_CLIST (ascii_table), items);
	}

	gtk_clist_select_row (GTK_CLIST (ascii_table), 0, 0);

	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
			    GTK_SIGNAL_FUNC (close_button_pressed), dialog);
	gtk_signal_connect (GTK_OBJECT (help_button), "clicked",
			    GTK_SIGNAL_FUNC (help_button_pressed), NULL);

	gtk_signal_connect (GTK_OBJECT (insert_char_button), "clicked",
			    GTK_SIGNAL_FUNC (insert_char_button_pressed), NULL);
	gtk_signal_connect (GTK_OBJECT (ascii_table), "select-row",
			    GTK_SIGNAL_FUNC (ascii_table_row_selected), NULL);

	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_finish), NULL);

	/* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent      (GNOME_DIALOG (dialog), gedit_window_active());
	gtk_window_set_modal         (GTK_WINDOW (dialog), FALSE);
	gnome_dialog_set_default     (GNOME_DIALOG (dialog), 0);

	/* Show everything then free the GladeXML memory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}
	
gint
init_plugin (PluginData *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("ASCII table");
	pd->desc = _("ASCII table");
	pd->long_desc = _("This plugin displays a pop-up dialog which contains an ASCII Table.");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->needs_a_document = FALSE;
	pd->modifies_an_existing_doc = FALSE;

	pd->private_data = (gpointer)gedit_plugin_create_dialog;

	pd->installed_by_default = TRUE;	

	return PLUGIN_OK;
}

