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
#include "document.h"
#include "search.h"
#include "utils.h"
#include "dialogs/dialogs.h"
#include <glade/glade.h>

GtkWidget *goto_line_dialog;

static void goto_line_destroyed_cb (GtkWidget *widget, gint button);
static void goto_line_clicked_cb (GtkWidget *widget, gint button);
static void goto_line_activate_cb (GtkWidget *widget, GtkWidget * dialog);

static void
goto_line_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug (DEBUG_SEARCH, "");
	goto_line_dialog = NULL;
}

static void
goto_line_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	gedit_debug (DEBUG_SEARCH, "");
	goto_line_clicked_cb (dialog, 0);
}

static void
goto_line_clicked_cb (GtkWidget *widget, gint button)
{
	gint line, lines = 0;
	gulong pos;
	View *view = GEDIT_VIEW (mdi->active_view);
	Document *doc = gedit_document_current();
	GtkText *text = GTK_TEXT (view->text);
	GtkWidget *entry;

	gedit_debug (DEBUG_SEARCH, "");
	g_return_if_fail (doc != NULL);
	
	if (button == 0)
	{

		entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");

		g_assert (entry!=NULL);

		line = atoi (gtk_entry_get_text (GTK_ENTRY(entry)));

		gedit_search_start();
		pos = line_to_pos (doc, line, &lines);
		gedit_search_end();

		if (line > lines)
			line = lines;

		
		gedit_view_set_window_position_from_lines (view, line, lines);
		/*
		update_window_position (text, line, lines);
		*/
		gtk_text_set_point (text, pos);
		gtk_text_insert (text, NULL, NULL, NULL, " ", 1);
		gtk_text_backward_delete (text, 1);

	}

	/* Even if the find was succesfull, we need to close
	   the dialog so that the user can continue using gedit
	   rigth away */
	gnome_dialog_close (GNOME_DIALOG (widget));
}

void
dialog_goto_line (void)
{
	GtkWidget *entry;
	GladeXML *gui; 

	gedit_debug(DEBUG_SEARCH, "");

	if (goto_line_dialog!=NULL)
	{
		/* Should we reaise it instead ?? chema */
		g_warning ("This should not happen ... dialog-goto-line.c:106");
		gtk_widget_show (goto_line_dialog);
		return;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR
			     "/goto-line.glade",
			     NULL);

	if (!gui)
	{
		g_warning ("Could not find goto-line.glade, reinstall gedit.");
		return;
	}

	goto_line_dialog = glade_xml_get_widget (gui, "dialog");
	entry            = glade_xml_get_widget (gui, "entry");

	if (!goto_line_dialog || !entry)
	{
		g_print ("Corrupted goto-line.glade detected, reinstall gedit.");
		return;
	}

	/* Set data so that we can get this widget later */ 
	gtk_object_set_data (GTK_OBJECT (goto_line_dialog), "entry", entry);

	/* How about conecting some signals .. */
	gtk_signal_connect (GTK_OBJECT (goto_line_dialog), "clicked",
			    GTK_SIGNAL_FUNC (goto_line_clicked_cb), goto_line_dialog);
	gtk_signal_connect (GTK_OBJECT (goto_line_dialog), "destroy",
			    GTK_SIGNAL_FUNC (goto_line_destroyed_cb), goto_line_dialog);
	gtk_signal_connect (GTK_OBJECT (entry), "activate",
			    GTK_SIGNAL_FUNC (goto_line_activate_cb), goto_line_dialog);

	/* And then a few more magic words */ 
	gtk_window_set_modal (GTK_WINDOW (goto_line_dialog), TRUE);
	gnome_dialog_set_parent (GNOME_DIALOG (goto_line_dialog),
				 GTK_WINDOW (mdi->active_window));
	gtk_widget_show_all (goto_line_dialog);

	/* dont need gui anymore */
	gtk_object_destroy (GTK_OBJECT (gui));
}
