/* browse.c - web browse plugin.
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
 *
 * Converted to a gmodule plugin by Jason Leach <leach@wam.umd.edu>
 */

/* TODO:
 * [ ] make me a GnomeDialog instead of a GtkDialog
 * [ ] maybe libglade-ify me
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "document.h"
#include "plugin.h"
#include "view.h"
#include "utils.h"

static GtkWidget *dialog;
static GtkWidget *entry1;


static void
goLynx (GtkWidget *widget, gpointer data)
{
	char buff[1025];
	int fdpipe[2];
	int pid;
	gchar *url;
	guint length, pos;
	Document *doc;

	url = gtk_entry_get_text (GTK_ENTRY (entry1));

	if (pipe (fdpipe) == -1)
	{
		g_assert_not_reached ();
	}
  
	pid = fork();
	if (pid == 0)
	{
		char *argv[4];

		close (1);
		dup (fdpipe[1]);
		close (fdpipe[0]);
		close (fdpipe[1]);
      
		argv[0] = "lynx";
		argv[1] = "-dump";
		argv[2] = url;
		argv[3] = NULL;
		execv ("/usr/bin/lynx", argv);

		g_assert_not_reached ();
	}
	close (fdpipe[1]);

	doc = gedit_document_new_with_title (url);

	length = 1;
	pos = 0;
	while (length > 0)
	{
		buff [ length = read (fdpipe[0], buff, 1024) ] = 0;
		if (length > 0)
		{
		     	gedit_document_insert_text (doc, buff, pos, FALSE);
			pos += length;
		}
	}

/*
	gnome_config_push_prefix ("/Editor_Plugins/Browse/");
	gnome_config_set_string ("Url", url[0]);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
*/
}

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
browse_close (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (dialog);
}

static void
browse (void)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *button;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), "The gEdit Web Browse Plugin");
	gtk_widget_set_usize (GTK_WIDGET (dialog), 353, 100);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (browse_close), NULL);
	gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);

	label = gtk_label_new ("Url: ");
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
  
	entry1 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry1, TRUE, TRUE, 0);

/*
	gnome_config_push_prefix ("/Editor_Plugins/Browse/");
	gtk_entry_set_text (GTK_ENTRY (entry1), gnome_config_get_string ("Url"));
	gnome_config_pop_prefix ();
	gnome_config_sync ();
*/
	button = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (goLynx), NULL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, FALSE, TRUE, 0);

	button = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (browse_close), NULL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, FALSE, TRUE, 0);

	gtk_widget_show_all (dialog);
}

gint
init_plugin (PluginData *pd)
{
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Browse");
	pd->desc = _("Web browse plugin");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";

	pd->private_data = (gpointer)browse;

	return PLUGIN_OK;
}
