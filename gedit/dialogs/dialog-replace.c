/*
 * dialog-replace.c: Dialog for replacing text in gedit.
 *
 * Author:
 *  Jason Leach <leach@wam.umd.edu>
 *
 */

/*
 * TODO:
 * [ ] libglade-ify me
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "gedit.h"
#include "view.h"
#include "document.h"
#include "search.h"

static GtkWidget *replace_dialog;

static GtkWidget* create_replace_dialog (void);
static void clicked_cb (GtkWidget *widget, gint button, Document *doc);
static gint ask_replace (void);


void
dialog_replace (void)
{
	Document *doc;

	doc = gedit_document_current ();

	if (!replace_dialog)
		replace_dialog = create_replace_dialog ();

	gtk_signal_connect (GTK_OBJECT (replace_dialog), "clicked",
			    GTK_SIGNAL_FUNC (clicked_cb), doc);
	gtk_widget_show (replace_dialog);
}

/* Callback on the dialog's "clicked" signal */
static void
clicked_cb (GtkWidget *widget, gint button, Document *doc)
{
	GtkWidget *datalink;
	gint pos, dowhat = 0;
	gulong options;
	gchar *str, *replace;
	gboolean confirm = FALSE;

	options = 0;
	if (button < 2)
	{
		get_search_options (doc, widget, &str, &options, &pos);
		datalink = gtk_object_get_data (GTK_OBJECT (widget),
						"replacetext");
		replace = gtk_entry_get_text (GTK_ENTRY (datalink));
		datalink = gtk_object_get_data (GTK_OBJECT (widget),
						"confirm");
		if (GTK_TOGGLE_BUTTON (datalink)->active)
			confirm = TRUE;
		do {
			pos = gedit_search_search (doc, str, pos, options);
			if (pos == -1) break;
			if (confirm)
			{
				search_select (doc, str, pos, options);
				dowhat = ask_replace();
				if (dowhat == 2) 
					break;
				if (dowhat == 1)
				{
					if (!(options | SEARCH_BACKWARDS))
						pos += num_widechars (str);

					continue;
				}
			}

			gedit_search_replace (doc, pos, num_widechars (str),
					      replace);
			if (!(options | SEARCH_BACKWARDS))
				pos += num_widechars (replace);

		} while (button); /* FIXME: this is so busted, its an
                                     infinite loop */
	}
	else
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
					       GTK_SIGNAL_FUNC (clicked_cb),
					       doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
}

static gint
ask_replace (void)
{
	return gnome_dialog_run_and_close (
		GNOME_DIALOG (gnome_message_box_new (
			_("Replace?"), GNOME_MESSAGE_BOX_INFO,
			GNOME_STOCK_BUTTON_YES,
			GNOME_STOCK_BUTTON_NO,
			GNOME_STOCK_BUTTON_CANCEL,
			NULL)));
}

static GtkWidget *
create_replace_dialog (void)
{
	GtkWidget *dialog;
	GtkWidget *frame, *entry, *check;

	dialog = gnome_dialog_new (_("Replace"),
				   _("Replace"),
				   _("Replace all"),
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

	frame = gtk_frame_new (_("Replace with:"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), frame,
			    FALSE, FALSE, 0);
	gtk_widget_show (frame);
	entry = gnome_entry_new ("searchdialogreplace");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_widget_show (entry);
	entry = gnome_entry_gtk_entry (GNOME_ENTRY(entry));
	gtk_object_set_data (GTK_OBJECT (dialog), "replacetext", entry);

	add_search_options (dialog);
	check = gtk_check_button_new_with_label (_("Ask before replacing"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), check,
			    FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "confirm", check);
	gtk_widget_show (check);

	gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);

	return dialog;
}


void
dialog_replace_impl (void)
{

}
