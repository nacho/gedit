/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * dialog-goto-line.c: Dialog box for going to a specified line.
 *
 * Author:
 * Chema Celorio <chema@celorio.com>
 */

#include <config.h>
#include <gnome.h>

#include "view.h"
#include "search.h"
#include "utils.h"
#include "dialogs/dialogs.h"
#include <glade/glade.h>

static void
gedit_goto_line (gint line)
{
	gint lines = 0;
	gulong pos;
	View *view;

	gedit_debug (DEBUG_SEARCH, "");

	if (line < 1)
		return;
	
	view = gedit_view_active ();

	if (view == NULL)
		return;
	
	gedit_search_start();
	pos = line_to_pos (line, &lines);
	gedit_search_end();

	if (line > lines)
		line = lines;
		
	gedit_view_set_window_position_from_lines (view, line, lines);
	gedit_view_set_position (view, pos);
	
}

static gboolean
dialog_goto_get_dialog (GtkWidget **dialog_, GtkWidget **entry_)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GladeXML *gui;

	gui = glade_xml_new (GEDIT_GLADEDIR "/goto-line.glade", "dialog");

	if (!gui) {
		g_warning ("Could not find goto-line.glade, reinstall gedit.");
		return FALSE;
	}

	dialog = glade_xml_get_widget (gui, "dialog");
	entry  = glade_xml_get_widget (gui, "entry");

	if (!dialog || !entry) {
		g_print ("Could not find the required widgets inside goto-line.glade.");
		return FALSE;
	}
	gtk_object_unref (GTK_OBJECT (gui));
	
	gnome_dialog_set_default     (GNOME_DIALOG (dialog), 0);
	gnome_dialog_editable_enters (GNOME_DIALOG (dialog), GTK_EDITABLE (entry));
	gnome_dialog_close_hides     (GNOME_DIALOG (dialog), TRUE);
	
	*dialog_ = dialog;
	*entry_  = entry;

	return TRUE;
}


/* - On the first method, we don't free the dialog, and thus avoid memleaking, but it uses
   more memory
   - The second method uses less memory overall since the dialog is freed after it is used
     but it memleaks, probably a mem-leak in gnome-libs. I keep this code here to eventually
     find this leak
*/
#if 0 
void
dialog_goto_line (void)
{
	const gchar *text;
	static GtkWidget *goto_line_dialog = NULL;
	static GtkWidget *entry;
	gint ret;

	gedit_debug(DEBUG_SEARCH, "");

	if (goto_line_dialog == NULL)
		if (!dialog_goto_get_dialog (&goto_line_dialog, &entry))
			return;

	gnome_dialog_set_parent (GNOME_DIALOG (goto_line_dialog),
				 GTK_WINDOW (mdi->active_window));

	ret = gnome_dialog_run (GNOME_DIALOG (goto_line_dialog));

	if (ret == -1)
		return;

	if (ret == 0) {
		text = gtk_entry_get_text (GTK_ENTRY(entry));
		if (text != NULL && text[0] != 0)
			gedit_goto_line (atoi (text));
	}

	gnome_dialog_close (GNOME_DIALOG (goto_line_dialog));
	
	return;
}

#else

/* Why is this function leaking 52 bytes ???? */
void
dialog_goto_line (void)
{
	GtkWidget *goto_line_dialog;
	GladeXML *gui;
	gint ret;
	gint n;

	for (n = 0; n < 100; n++) {
		gedit_debug(DEBUG_SEARCH, "");
		
		gui = glade_xml_new (GEDIT_GLADEDIR "/goto-line.glade", "dialog");
		
		if (!gui) {
			g_warning ("Could not find goto-line.glade, reinstall gedit.");
			return;
		}
		
		goto_line_dialog = glade_xml_get_widget (gui, "dialog");

		if (!goto_line_dialog) {
			g_print ("Could not find the required widgets inside goto-line.glade.");
			return;
		}
		
		ret = gnome_dialog_run (GNOME_DIALOG (goto_line_dialog));

		g_print ("Ret %i\n", ret);
		
		if (ret == -1) {
			gtk_object_unref (GTK_OBJECT (gui));
			return;
		}
		
		gnome_dialog_close (GNOME_DIALOG (goto_line_dialog));

		gtk_object_unref (GTK_OBJECT (gui));
	}
	
	return;
}

#endif
