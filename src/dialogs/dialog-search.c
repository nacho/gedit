#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "document.h"
#include "search.h"
#include "utils.h"

static GtkWidget *search_dialog;
static GtkWidget* create_search_dialog (void);
static void search_dialog_button_cb  (GtkWidget   *widget,
                                      gint         button,
                                      Document *doc);

/*
 * Search dialog
 */
static void
search_dialog_button_cb (GtkWidget *widget, gint button, Document *doc)
{
	gint pos;
	gulong options = 0;
	gchar *str;

	gedit_debug ("F:search_dialog_button_cb\n", DEBUG_SEARCH);

	if (!gedit_document_current ())
	{
		gnome_dialog_close(GNOME_DIALOG(widget));
		return;
	}
	
	if (button == 0)
	{
		get_search_options (doc, widget, &str, &options, &pos);
		pos = gedit_search_search (doc, str, pos, options);
		if (pos != -1)
			search_select (doc, str, pos, options);
	}
	else
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
					       GTK_SIGNAL_FUNC (search_dialog_button_cb),
					       doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
}

static GtkWidget *
create_search_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *frame, *entry;

	gedit_debug ("F:create_search_dialog\n", DEBUG_SEARCH);

	dialog = gnome_dialog_new (_("Search"),
				   _("Search"),
				   GNOME_STOCK_BUTTON_CLOSE,
				   NULL);

	frame = gtk_frame_new (_("Search for:"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), frame,
			    FALSE, FALSE, 0);
	gtk_widget_show (frame);

	entry = gnome_entry_new ("searchdialogsearch");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_widget_show (entry);
	entry = gnome_entry_gtk_entry (GNOME_ENTRY(entry));
	gtk_object_set_data (GTK_OBJECT (dialog), "searchtext", entry);
	add_search_options (dialog);
	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);
	
	return dialog;
}

void
dialog_search (GtkWidget *widget, gpointer data)
{
	Document *doc;

	gedit_debug ("F:search_cb\n", DEBUG_SEARCH);

	doc = gedit_document_current ();

	if (!doc)
	     return;

	if (!search_dialog)
		search_dialog = create_search_dialog ();

	gtk_signal_connect (GTK_OBJECT (search_dialog), "clicked",
			    GTK_SIGNAL_FUNC (search_dialog_button_cb), doc);
	gtk_widget_show (search_dialog);
}
