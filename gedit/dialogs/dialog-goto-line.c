/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * dialog-goto-line.c: Dialog box for going to a specified line.
 *
 * Author:
 * Chema Celorio <chema@celorio.com>
 */

#include <config.h>
#include <gnome.h>

#include "document.h"
#include "search.h"
#include "view.h"
#include "utils.h"
#include "dialogs/dialogs.h"

#include <glade/glade.h>

GtkWidget *goto_line_dialog;

static void
goto_line_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug("\n", DEBUG_SEARCH);
	search_end();
	goto_line_dialog = NULL;
}

static void
goto_line_activate_cb (GtkWidget *widget, GtkWidget * dialog)
{
	GtkSpinButton *spinb;
	gint line, lines = 0;
	gulong pos;
	View *view = VIEW (mdi->active_view);
	Document *doc = gedit_document_current();
	GtkText *text = GTK_TEXT (view->text);

	gedit_debug("\n", DEBUG_SEARCH);
	g_return_if_fail (doc != NULL);

	spinb = GTK_SPIN_BUTTON (widget);

	gtk_spin_button_update (GTK_SPIN_BUTTON (spinb));
	line = (gint) gtk_spin_button_get_value_as_float (spinb);
	pos = line_to_pos (doc, line, &lines);
	update_text (text, line, lines);

	gtk_text_set_point(text, pos);

	gtk_text_insert (text, NULL, NULL, NULL, " ", 1);
	gtk_text_backward_delete (text, 1);

	gnome_dialog_close (GNOME_DIALOG (dialog));

}

static void
goto_line_clicked_cb (GtkWidget *widget, gint button)
{
	GtkWidget *spinb;
	gint line, lines = 0;
	gulong pos;
	View *view = VIEW (mdi->active_view);
	Document *doc = gedit_document_current();
	GtkText *text = GTK_TEXT (view->text);

	gedit_debug("\n", DEBUG_SEARCH);
	g_return_if_fail (doc != NULL);
	
	if (button == 0)
	{
		spinb = gtk_object_get_data (GTK_OBJECT (widget), "spinb");
		g_assert (spinb != NULL);
		line = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinb));

		pos = line_to_pos (doc, line, &lines);
		
		update_text (text, line, lines);
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
	GtkWidget *spinb;
	GtkAdjustment *adj;
	GladeXML *gui; 
	gint total_lines, line_number;

	gedit_debug("\n", DEBUG_SEARCH);

	if (goto_line_dialog!=NULL)
	{
		/* Should we reaise it instead ?? chema */
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
	spinb = glade_xml_get_widget (gui, "spinb");

	if (!goto_line_dialog || !spinb)
	{
		g_print ("Corrupted goto-line.glade detected, reinstall gedit.");
		return;
	}

	search_start();
	line_number = pos_to_line (gtk_editable_get_position (GTK_EDITABLE (VIEW (mdi->active_view)->text)),
				   &total_lines);

	gtk_object_set_data (GTK_OBJECT (goto_line_dialog), "spinb", spinb);

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinb));

	adj->upper = (gfloat) total_lines;
	
	gtk_signal_connect (GTK_OBJECT (goto_line_dialog), "clicked",
			    GTK_SIGNAL_FUNC (goto_line_clicked_cb), goto_line_dialog);
	gtk_signal_connect (GTK_OBJECT (goto_line_dialog), "destroy",
			    GTK_SIGNAL_FUNC (goto_line_destroyed_cb), goto_line_dialog);
	gtk_signal_connect (GTK_OBJECT (spinb), "activate",
			    GTK_SIGNAL_FUNC (goto_line_activate_cb), goto_line_dialog);

	gtk_window_set_modal (GTK_WINDOW (goto_line_dialog), TRUE);
	gtk_widget_show_all (goto_line_dialog);
	gtk_object_destroy (GTK_OBJECT (gui));
}
