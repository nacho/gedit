#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "document.h"
#include "search.h"

/* Callback on the "clicked" signal in the Find Line dialog */
static void
find_line_clicked_cb (GtkWidget *widget, gint button, Document *doc)
{
	GtkWidget *spinb;
	gint line, linecount;
	gint pos;
/*	gedit_view *view = GE_VIEW (mdi->active_view); */

	g_return_if_fail (doc != NULL);
	
	if (button == 0)
	{
		spinb = gtk_object_get_data (GTK_OBJECT (widget), "spinb");
		line = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinb));
		seek_to_line (doc, line, -1);
		pos = line_to_pos (doc, line, &linecount);
/*		gtk_editable_set_position (GTK_EDITABLE (view->text), pos); */
	}
	else
		gnome_dialog_close (GNOME_DIALOG (widget));
}

void
find_line_cb (GtkWidget *widget, gpointer data)
{
#if 1
	GladeXML *gui = glade_xml_new (GEDIT_GLADEDIR
				       "/find-line.glade",
				       NULL);
	GtkWidget *dialog;
	GtkWidget *spinb;
	GtkAdjustment *adj;
	Document *doc = gedit_document_current ();

	if (!gui)
	{
		g_warning ("Could not find find-line.glade");
		return;
	}

	dialog = glade_xml_get_widget (gui, "dialog");
	spinb = glade_xml_get_widget (gui, "spinb");

	gtk_object_set_data (GTK_OBJECT (dialog), "spinb", spinb);

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinb));

	adj->upper = (gfloat) get_line_count (doc);
	
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (find_line_clicked_cb), doc);

	gtk_widget_show_all (dialog);
	gtk_object_destroy (GTK_OBJECT (gui));

#else
	GtkWidget *spin;
	GtkAdjustment *adj;
	Document *doc;
	gint linecount;
	gedit_view *view = GE_VIEW (mdi->active_view);

	doc = gedit_document_current ();

	if (!line_dialog) 
		line_dialog = create_line_dialog ();

	spin = gtk_object_get_data (GTK_OBJECT (line_dialog), "line");

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin));
	adj->value = (gfloat) pos_to_line (doc,
					   gtk_editable_get_position (GTK_EDITABLE (view->text)),
					   &linecount);
	adj->upper = (gfloat) linecount;
	gtk_signal_connect (GTK_OBJECT (line_dialog), "clicked",
			    GTK_SIGNAL_FUNC (line_dialog_button_cb), doc);

	gtk_widget_show (line_dialog);
#endif
}
