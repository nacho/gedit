/* diff.c - diff plugin.
 *
 * Copyright (C) 1998 Chris Lahey.
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

/*
 * TODO:
 * [ ] libglade-ify me.
 * [ ] pretty the output a bit, maybe something like coloring the
 * lines for pluses and minuses
 * [ ] make this plugin useful for people who might not have diff
 * installed in /usr/bin
 *
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "document.h"
#include "plugin.h"
#include "view.h"
#include "utils.h"

static GtkWidget *entry1;
static GtkWidget *entry2;
static GtkWidget *dialog;

void call_diff (GtkWidget *widget, gpointer data);

static void
set_entry (GtkWidget *widget, gpointer data)
{
	GtkWidget *file_sel = gtk_widget_get_toplevel (widget);

	gtk_entry_set_text (GTK_ENTRY (data), gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_sel)));
	gtk_widget_destroy (file_sel);
}

static void
open_file_sel (GtkWidget *widget, gpointer data)
{
	GtkWidget *file_sel = gtk_file_selection_new ("Choose a file");
  
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_sel)->ok_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (set_entry),
			    data);

	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (file_sel)->cancel_button),
				   "clicked",
				   GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (file_sel));
  
	gtk_widget_show_all (file_sel);
}

static void
done (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (dialog);
}

void
call_diff (GtkWidget *widget, gpointer data)
{
	char buff[1025];
	int fdpipe[2];
	int pid;
	char *filenames[2] = { NULL, NULL };
	int length;
	Document *doc;
	gint i;

	filenames[0] = gtk_entry_get_text (GTK_ENTRY (entry1));
	filenames[1] = gtk_entry_get_text (GTK_ENTRY (entry2));

	if (pipe (fdpipe) == -1)
	{
		_exit (1);
	}
  
	pid = fork();
	if (pid == 0)
	{
		/* New process. */
		char *argv[5];

		close (1);
		dup (fdpipe[1]);
		close (fdpipe[0]);
		close (fdpipe[1]);
      
		argv[0] = "diff";
		argv[1] = "-u";
		argv[2] = filenames[0];
		argv[3] = filenames[1];
		argv[4] = NULL;
		execv ("/usr/bin/diff", argv);
		/* This is only reached if something goes wrong. */
		_exit (1);
	}
	close (fdpipe[1]);

	doc = gedit_document_new_with_title ("diff");
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));  

	length = 1;
	while (length > 0)
	{
		buff[ length = read (fdpipe[0], buff, 1024) ] = 0;
		if (length > 0)
		{
			/* FIXME: We are insterting in the begining of the
			   file but we need to insert where the cursor is. Chema */
			views_insert (doc, 0, buff, length, NULL);
		}
	}

	for (i = 0; i < g_list_length (doc->views); i++)
	{
		View *nth_view;
		nth_view = g_list_nth_data (doc->views, i);
		/* Disabled by chema
		gedit_view_refresh (nth_view);
		*/
		gedit_set_title (nth_view->document);
	}
}

static void
destroy_plugin (PluginData *pd)
{

}

static void
diff_plugin (PluginData *pd)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *button;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), "Choose files to diff" );
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (done), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), GNOME_PAD);

	label = gtk_label_new ("Choose files to diff:");
	gtk_box_pack_start (GTK_BOX( GTK_DIALOG( dialog)->vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
  
	entry1 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry1, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Browse...");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (open_file_sel), entry1);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  
	entry2 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry2, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Browse...");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (open_file_sel), entry2);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	button = gtk_button_new_with_label ("Calculate Diff");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (call_diff), NULL);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button,
			    FALSE, TRUE, 0);

	button = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (done), NULL);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button,
			    FALSE, TRUE, 0);

	gtk_widget_show_all (dialog);
}

gint
init_plugin (PluginData *pd)
{
	/* initialize */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Diff");
	pd->desc = _("Diff plugin.");
	pd->author = "Chris Lahey";

	pd->private_data = (gpointer)diff_plugin;

	return PLUGIN_OK;
}
