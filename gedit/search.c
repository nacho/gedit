/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
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
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <config.h>

#include "main.h"
#include "gE_document.h"
#include "gE_mdi.h"
#include "search.h"


/* pointers to the dialogs */

static GtkWidget *line_dialog;
static GtkWidget *search_dialog;
static GtkWidget *replace_dialog;

/* private functions */
static GtkWidget *create_line_dialog ();
static void add_search_options (GtkWidget *dialog);
static GtkWidget *create_search_dialog ();
static GtkWidget *create_replace_dialog ();
static void line_dialog_button_cb    (GtkWidget   *widget,
                                      gint         button,
                                      gE_document *doc);
static void search_dialog_button_cb  (GtkWidget   *widget,
                                      gint         button,
                                      gE_document *doc);
static void replace_dialog_button_cb (GtkWidget   *widget,
                                      gint         button,
                                      gE_document *doc);
static void get_search_options       (gE_document *doc,
                                      GtkWidget   *widget,
                                      gchar      **txt,
                                      gulong      *options,
                                      gint        *pos);
static gboolean search               (GtkEditable *text,
                                      gchar       *str,
                                      gint         pos,
                                      gulong       options);
static void search_select            (gE_document *doc,
                                      gchar       *str,
                                      gint         pos,
                                      gulong       options);
static gint ask_replace ();

/* old stuff, remove soon */
void search_replace_cb(GtkWidget *w, gpointer cbdata) { }
void search_again_cb(GtkWidget * w, gpointer cbdata) { }

/*
 * Public interface. 
 */

gint
pos_to_line (gE_document *doc, gint pos, gint *numlines)
{
	gulong lines = 0, i, current_line = 0;
	gchar *c;


	for (i = 0; i < gtk_text_get_length(GTK_TEXT(doc->text)); i++) {
		c = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i, i + 1);
		if (!strcmp(c, "\n")) {
			lines++;
		}
		if (i == pos) {
			current_line = lines;
		}
		g_free (c);
	}
	*numlines = lines;
	return current_line;
}

gint
line_to_pos (gE_document *doc, gint line, gint *numlines)
{
	gulong lines = 1, i, current;
	gchar *c;


	for (i = 1; i < gtk_text_get_length(GTK_TEXT(doc->text)) - 1; i++) {
		c = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i - 1, i);
		if (!strcmp(c, "\n")) {
			lines++;
		}
		g_free (c);
		if (lines == line) {
			current = lines;
		}
	}
	*numlines = lines;
	return (i);
}

gint
get_line_count (gE_document *doc)
{
	gulong lines = 1, i;
	gchar *c;


	if (gtk_text_get_length(GTK_TEXT(doc->text)) == 0) {
		return 0;
	}
	for (i = 1; i < gtk_text_get_length(GTK_TEXT(doc->text)) - 1; i++) {
		c = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i - 1, i);
		if (!strcmp(c, "\n")) {
			lines++;
		}
		g_free (c);
	}
	return lines;
}

void
seek_to_line (gE_document *doc, gint line, gint numlines)
{
	gfloat value, ln, tl;
	gchar *c;
	gint i;

	if (numlines < 0) {
		numlines = get_line_count (doc);
	}

	if (numlines < 3)
		return;
	if (line > numlines)
		return;
	ln = line;
	tl = numlines;
	value = (ln * GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj)->upper) /
		tl - GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj)->page_increment;
	gtk_adjustment_set_value(GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj),
			value);
}
gint
gE_search_search (gE_document *doc, gchar *str, gint pos, gulong options)
{
	gint i, textlen;

	textlen = gtk_text_get_length(GTK_TEXT(doc->text));

	if (options & SEARCH_BACKWARDS) {
		pos -= strlen (str);
		for (i = pos; i >= 0; i--) {
			if (search (GTK_EDITABLE (doc->text), str, i, options)) {
				return i;
			}
		}
	} else {
		for (i = pos; i <= (textlen - strlen(str)); i++) {
			if (search (GTK_EDITABLE (doc->text), str, i, options)) {
				return i;
			}
		}
	}
	return (-1);
}

/*
 * Replace the string searched earlier at position pos with
 * replace.
 *
 * This doesn NOT do any sanity checking
 */ 
void
gE_search_replace (gE_document *doc, gint pos, gint len, gchar *replace)
{
	gtk_editable_delete_text (GTK_EDITABLE (doc->text),
			pos, pos + len);
	gtk_editable_insert_text (GTK_EDITABLE (doc->text),
			replace, strlen (replace), &pos);
}
/*
 * public callbacks (for menus, etc)
 */

void
search_cb (GtkWidget *widget, gpointer data)
{
	gE_document *doc;

	doc = gE_document_current ();

	if (!search_dialog) {
		search_dialog = create_search_dialog();
	}
	gtk_signal_connect (GTK_OBJECT (search_dialog), "clicked",
		GTK_SIGNAL_FUNC (search_dialog_button_cb), doc);
	gtk_widget_show (search_dialog);
}

void
replace_cb (GtkWidget *widget, gpointer data)
{
	gE_document *doc;

	doc = gE_document_current ();

	if (!replace_dialog) {
		replace_dialog = create_replace_dialog();
	}
	gtk_signal_connect (GTK_OBJECT (replace_dialog), "clicked",
		GTK_SIGNAL_FUNC (replace_dialog_button_cb), doc);
	gtk_widget_show (replace_dialog);
}
void
goto_line_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *spin;
	GtkAdjustment *adj;
	gE_document *doc;
	gfloat val, max;
	gint linecount;

	doc = gE_document_current ();

	if (!line_dialog) {
		line_dialog = create_line_dialog();
	}

	spin = gtk_object_get_data (GTK_OBJECT (line_dialog), "line");

	adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin));
	adj->value = (gfloat) pos_to_line (doc,
		gtk_editable_get_position (GTK_EDITABLE (doc->text)),
		&linecount);
	adj->upper = (gfloat) linecount;
	gtk_signal_connect (GTK_OBJECT (line_dialog), "clicked",
		GTK_SIGNAL_FUNC (line_dialog_button_cb), doc);

	gtk_widget_show (line_dialog);
}

/*
 * PUBLIC: count_lines_cb
 *
 * public callback routine to count the total number of lines in the current
 * document and the current line number, and display the info in a dialog.
 */

void count_lines_cb (GtkWidget *widget, gpointer data)
{
	gint total_lines, line_number;
	gchar *msg;
	gE_document *doc;

	doc = gE_document_current ();
	
	g_assert (doc != NULL);
	g_assert (doc->text != NULL);

	line_number = pos_to_line (doc,
			gtk_editable_get_position (GTK_EDITABLE (doc->text)),
			&total_lines);
	
	msg = g_malloc0 (200);
	sprintf (msg, _("Total Lines: %i\nCurrent Line: %i"), total_lines, line_number);
	
	gnome_dialog_run_and_close ((GnomeDialog *)
		gnome_message_box_new (msg, GNOME_MESSAGE_BOX_INFO,
			GNOME_STOCK_BUTTON_OK, NULL));
}

/* private functions */

/* Creating the dialogs
 *
 * goto line dialog
 */

static GtkWidget *
create_line_dialog ()
{
	GtkWidget *dialog;
	GtkWidget *hbox, *label, *spin;
	GtkObject *adj;

	dialog = gnome_dialog_new (_("Go to line"),
		GNOME_STOCK_BUTTON_OK,
		GNOME_STOCK_BUTTON_CANCEL,
		NULL);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), hbox,
		FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	label = gtk_label_new (_("Line number"));
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
	return (dialog);
}

static void
add_search_options (GtkWidget *dialog)
{
	GtkWidget *radio, *check;
	GSList *radiolist;

	radio = gtk_radio_button_new_with_label (NULL,
			_("Search from cursor"));
	radiolist = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), radio,
		FALSE, FALSE, 0);
	gtk_widget_show (radio);
	
	radio = gtk_radio_button_new_with_label (radiolist,
			_("Search from beginning of document"));
	radiolist = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), radio,
		FALSE, FALSE, 0);
	gtk_widget_show (radio);
	
	radio = gtk_radio_button_new_with_label (radiolist,
			_("Search from end of document"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), radio,
		FALSE, FALSE, 0);
	gtk_widget_show (radio);
	radiolist = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	gtk_object_set_data (GTK_OBJECT (dialog), "searchfrom", radiolist);
	
	check = gtk_check_button_new_with_label (_("Case sensitive"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), check,
		FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "case", check);
	gtk_widget_show (check);
	
	check = gtk_check_button_new_with_label (_("Reverse search"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), check,
		FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "direction", check);
	gtk_widget_show (check);
}
static GtkWidget *
create_search_dialog ()
{
	GtkWidget *dialog;
	GtkWidget *frame, *entry, *radio, *check;

	dialog = gnome_dialog_new (_("Search"),
		"Search",
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
	
	return (dialog);
}

static GtkWidget *
create_replace_dialog ()
{
	GtkWidget *dialog;
	GtkWidget *frame, *entry, *radio, *check;

	dialog = gnome_dialog_new (_("Replace"),
		"Replace",
		"Replace all",
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

	return (dialog);
}
/*
 * callback for the goto line dialog
 */
static void
line_dialog_button_cb (GtkWidget *widget, gint button, gE_document *doc)
{
	GtkWidget *spin;
	gint line, linecount;
	gulong pos;

	if (button == 0) {
		spin = gtk_object_get_data (GTK_OBJECT (widget), "line");
		line = gtk_spin_button_get_value_as_int
			(GTK_SPIN_BUTTON (spin));
		seek_to_line (doc, line, -1);
	}
	gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
			GTK_SIGNAL_FUNC (line_dialog_button_cb),
			doc);
	gnome_dialog_close (GNOME_DIALOG (widget));
}

/*
 * Search dialog
 */
static void
search_dialog_button_cb (GtkWidget *widget, gint button, gE_document *doc)
{
	GtkWidget *datalink;
	gint pos, line, numlines;
	gulong options;
	gchar *str;

	options = 0;
	if (button == 0) {
		get_search_options (doc, widget, &str, &options, &pos);
		pos = gE_search_search (doc, str, pos, options);
		if (pos != -1) {
			search_select (doc, str, pos, options);
		}
	} else {
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				GTK_SIGNAL_FUNC (search_dialog_button_cb),
				doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
}

/*
 * Replace dialog
 */
static void
replace_dialog_button_cb (GtkWidget *widget, gint button, gE_document *doc)
{
	GtkWidget *datalink;
	gint pos, line, numlines, dowhat = 0;
	gulong options;
	gchar *str, *replace;
	gboolean confirm = FALSE;

	options = 0;
	if (button < 2) {
		get_search_options (doc, widget, &str, &options, &pos);
		datalink = gtk_object_get_data (GTK_OBJECT (widget),
				"replacetext");
		replace = gtk_entry_get_text (GTK_ENTRY (datalink));
		datalink = gtk_object_get_data (GTK_OBJECT (widget),
				"confirm");
		if (GTK_TOGGLE_BUTTON (datalink)->active)
			confirm = TRUE;
		do {
			pos = gE_search_search (doc, str, pos, options);
			if (pos == -1) break;
			if (confirm) {
				search_select (doc, str, pos, options);
				dowhat = ask_replace();
				if (dowhat == 2)  break;
				if (dowhat == 1) {
					if ( !(options | SEARCH_BACKWARDS)) {
						pos += strlen (str);
					}
					continue;
				}
			}

			gE_search_replace (doc, pos, strlen (str),
					replace);
			if ( !(options | SEARCH_BACKWARDS)) {
				pos += strlen (replace);
			}
		} while (button);
	} else {
		gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				GTK_SIGNAL_FUNC (replace_dialog_button_cb),
				doc);
		gnome_dialog_close (GNOME_DIALOG (widget));
	}
}
/* Utility functions
 *
 * the actual search
 */

static void
get_search_options (gE_document *doc, GtkWidget *widget,
		gchar **txt, gulong *options, gint *pos)
{
	GtkWidget *datalink;
	GtkRadioButton *pos_but;
	gint pos_type = 0;
	GSList *from;

	datalink = gtk_object_get_data (GTK_OBJECT (widget), "case");
	if (GTK_TOGGLE_BUTTON (datalink)->active) {
		*options |= SEARCH_NOCASE;
	}
	datalink = gtk_object_get_data (GTK_OBJECT (widget), "direction");
	if (GTK_TOGGLE_BUTTON (datalink)->active) {
		*options |= SEARCH_BACKWARDS;
	}
	datalink = gtk_object_get_data (GTK_OBJECT (widget), "searchtext");
	*txt = gtk_entry_get_text (GTK_ENTRY (datalink));

	from = gtk_object_get_data (GTK_OBJECT (widget), "searchfrom");
	while (from) {
		pos_but = from->data;
		if (GTK_TOGGLE_BUTTON(pos_but)->active) {
			break;
		}
		from = from->next;
		pos_type++;
	}
	switch (pos_type) {
		case 0:
			*pos = gtk_text_get_length (GTK_TEXT (doc->text));
			break;
		case 1:
			*pos = 0;
			break;
		case 2:
			*pos = gtk_editable_get_position (GTK_EDITABLE (doc->text));
			break;
	}
}
static gboolean
search (GtkEditable *text, gchar *str, gint pos, gulong options)
{
	gchar *buffer;

	buffer = g_malloc0 (strlen (str) + 1);
	buffer = gtk_editable_get_chars (text, pos, pos + strlen (str));
	if (options & SEARCH_NOCASE) {
		return (!g_strcasecmp (buffer, str));
	} else {
		return (!strcmp (buffer, str));
	}
}
static void
search_select (gE_document *doc, gchar *str, gint pos, gulong options)
{
	gint line, numlines;

	line = pos_to_line (doc, pos, &numlines);
	seek_to_line (doc, line, numlines);
	if (options | SEARCH_BACKWARDS) {
		gtk_editable_set_position (GTK_EDITABLE (doc->text),
				pos);
		gtk_editable_select_region (GTK_EDITABLE (doc->text),
				pos + strlen (str), pos);
	} else {
		gtk_editable_set_position (GTK_EDITABLE (doc->text),
				pos + strlen (str));
		gtk_editable_select_region (GTK_EDITABLE (doc->text),
				 pos, pos + strlen (str));
	}
}

static gint
ask_replace ()
{
	return gnome_dialog_run_and_close (GNOME_DIALOG (
		gnome_message_box_new (_("Replace?"), GNOME_MESSAGE_BOX_INFO,
			GNOME_STOCK_BUTTON_YES,
			GNOME_STOCK_BUTTON_NO,
			GNOME_STOCK_BUTTON_CANCEL,
			NULL)));
}
/* the end */
