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

GtkWidget *line_dialog;

static void
find_line_destroyed_cb (GtkWidget *widget, gint button)
{
	gedit_debug("\n", DEBUG_SEARCH);
	search_end();
	line_dialog = NULL;
}

static void
find_line_activate_cb (GtkWidget *widget, GtkWidget * dialog)
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
find_line_clicked_cb (GtkWidget *widget, gint button)
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

		g_print("Line dentro de find_line :%i\n", line);
		
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
dialog_find_line (void)
{
	GtkWidget *dialog;
	GtkWidget *spinb;
	GtkAdjustment *adj;
	GladeXML *gui = glade_xml_new (GEDIT_GLADEDIR
				       "/goto-line.glade",
				       NULL);
	gint total_lines, line_number;

	if (!gui)
	{
		g_warning ("Could not find goto-line.glade, reinstall gedit.");
		return;
	}

	dialog = glade_xml_get_widget (gui, "dialog");
	spinb = glade_xml_get_widget (gui, "spinb");

	if (!dialog || !spinb)
	{
		g_print ("Corrupted goto-line.glade detected, reinstall gedit.");
		return;
	}

	search_start();
	line_number = pos_to_line (gtk_editable_get_position (GTK_EDITABLE (VIEW (mdi->active_view)->text)),
				   &total_lines);

	gtk_object_set_data (GTK_OBJECT (dialog), "spinb", spinb);

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinb));

	adj->upper = (gfloat) total_lines;
	
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (find_line_clicked_cb), dialog);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (find_line_destroyed_cb), dialog);
	gtk_signal_connect (GTK_OBJECT (spinb), "activate",
			    GTK_SIGNAL_FUNC (find_line_activate_cb), dialog);

	gtk_widget_show_all (dialog);
	gtk_object_destroy (GTK_OBJECT (gui));
}


















































/*
 * TODO:
 * [ ] revert back to non-glade, this dialog is too small to get anything
 *     from it.
 */

#if 0


/* static GtkWidget * create_line_dialog (void);*/
static void find_line_clicked_cb (GtkWidget *widget, gint button, Document *doc);
       void dialog_find_line (void);


/* Callback on the "clicked" signal in the Find Line dialog */

#if 0
void
dialog_find_line_impl (void)
{

}
#endif

void
dialog_find_line (void)
{
#if 1
	GtkWidget *dialog;
	GtkWidget *spinb;
	GtkAdjustment *adj;
	Document *doc = gedit_document_current ();
	GladeXML *gui = glade_xml_new (GEDIT_GLADEDIR "find-line.glade",
				       NULL);

	if (!gui)
	{
		g_warning ("Could not find find-line.glade, reinstall gedit.");
		return;
	}

	dialog = glade_xml_get_widget (gui, "dialog");
	spinb = glade_xml_get_widget (gui, "spinb");

	if (!dialog || !spinb)
	{
		g_print ("Corrupted find-line.glade detected, reinstall gedit.");
		return;
	}

	gtk_object_set_data (GTK_OBJECT (dialog), "spinb", spinb);

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinb));

	adj->upper = (gfloat) get_line_count (doc);

	/* FIXME: chema.
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (find_line_clicked_cb), doc);
	*/

	gtk_widget_show_all (dialog);
	gtk_object_destroy (GTK_OBJECT (gui));

#else
#endif
}


#if 0
static GtkWidget *
create_line_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *hbox, *label, *spin;
	GtkObject *adj;

	dialog = gnome_dialog_new (_("Go to line"),
				   GNOME_STOCK_BUTTON_OK,
				   GNOME_STOCK_BUTTON_CANCEL,
				   NULL);

	hbox = gtk_hbox_new (TRUE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), hbox,
			    FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	label = gtk_label_new (_("Line number: "));
	gtk_box_pack_start (GTK_BOX (hbox), label,
			    FALSE, FALSE, 0);
	gtk_widget_show (label);

	adj = gtk_adjustment_new (1, 1, 1, 1, 20, 20);
	spin = gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1, 0);
	gtk_box_pack_start (GTK_BOX (hbox), spin,
			    FALSE, FALSE, 0);
	gtk_widget_show (spin);

	gtk_object_set_data (GTK_OBJECT (dialog), "line", spin);
	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);

	return dialog;
}
#endif



#endif
