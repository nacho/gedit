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
#include <gtk/gtk.h>
#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif

#include "main.h"
#include "gE_document.h"
#include "dialog.h"


static void seek_to_line(gE_document *doc, gint line_number);
static gint point_to_line(gE_document *doc, gint point);
static void search_start(GtkWidget *w, gE_data *data);
static void search_create_dialog(gE_window *w, gE_search *, gboolean replace);
static void search_for_line(GtkWidget *w, gE_search *options);
static void search_replace_yes_sel(GtkWidget *w, gE_data *data);
static void search_replace_no_sel(GtkWidget *w, gE_data *data);
static void search_replace_common(gE_data *data, gboolean do_replace);
static void popup_replace_window(gE_data *data);


/*
 * PUBLIC: search_cb
 *
 * main callback routine from pulldown menu
 */
void 
search_cb(GtkWidget *wgt, gpointer cbdata)
{
	search_replace_common((gE_data *)cbdata, FALSE);
} /* search_cb */


/*
 * PUBLIC: search_replace_cb
 *
 * menu callback
 */
void 
search_replace_cb(GtkWidget *w, gpointer cbdata)
{
	search_replace_common((gE_data *)cbdata, TRUE);
} /* search_replace_cb */


/*
 * PUBLIC: search_again_cb
 *
 * menu callback
 */
void 
search_again_cb(GtkWidget * w, gpointer cbdata)
{
	gE_data *data = (gE_data *) cbdata;

	if (data->window->search->window) {
		data->temp2 = data->window->search;
		data->window->search->replace = FALSE;
		data->window->search->again = TRUE;
		search_start(w, data);
	}
} /* search_again_cb */


/*
 * PUBLIC: goto_line_cb
 *
 * public callback routine to popup dialog box asking user which line number
 * to go to.
 */
void 
goto_line_cb(GtkWidget *wgt, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	g_assert(w != NULL);
	g_assert(w->search != NULL);
	if (!w->search->window)
		search_create_dialog(w, w->search, FALSE);
	gtk_check_menu_item_set_state(
		GTK_CHECK_MENU_ITEM(w->search->line_item), 1);
	gtk_option_menu_set_history(GTK_OPTION_MENU(w->search->search_for), 1);
	search_for_line(NULL, w->search);
	gtk_widget_show(w->search->window);

} /* goto_line_cb */



/*
 * PUBLIC: count_lines_cb
 *
 * public callback routine to count the total number of lines in the current
 * document and the current line number, and display the info in a dialog.
 */

void count_lines_cb (GtkWidget *widget, gpointer cbwindow)
{
	gint total_lines, line_number, i;
	gchar *msg, *c;
	gchar *buttons[] = { GE_BUTTON_OK };
	
	gE_document *doc;
	gE_window *w =  (gE_window *) cbwindow;
	
	g_assert (w != NULL);
	doc = gE_document_current (w);
	
	g_assert (doc != NULL);
	g_assert (doc->text != NULL);

	total_lines = line_number = 1;
	for (i = 1; i < gtk_text_get_length(GTK_TEXT(doc->text)) - 1; i++) {
		c = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i - 1, i);
		if (c == NULL)
			break;
		if (strcmp(c, "\n") == 0)
		{
			total_lines++;
			if (i <= GTK_EDITABLE (doc->text)->current_pos)
				line_number++;
		}
		g_free (c);
	}
	
	msg = g_malloc0 (200);
	sprintf (msg, "Total Lines: %i\nCurrent Line: %i", total_lines, line_number);
	#ifdef WITHOUT_GNOME
	ge_dialog ("Line Information",
		msg,
		1, buttons,
		1, NULL, NULL, TRUE);
	#else
	gnome_dialog_run_and_close ((GnomeDialog *)
		gnome_message_box_new (msg, GNOME_MESSAGE_BOX_INFO,
			buttons[0], NULL));
	#endif
}

static void
seek_to_line(gE_document * doc, gint line_number)
{
	gfloat value, ln, tl;
	gchar *c;
	gint i, total_lines = 0;

	for (i = 0; i < gtk_text_get_length(GTK_TEXT(doc->text)); i++) {
		c = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i, i + 1);
		if (strcmp(c, "\n") == 0)
			total_lines++;
		g_free (c);
	}

	if (total_lines < 3)
		return;
	if (line_number > total_lines)
		return;
	tl = total_lines;
	ln = line_number;
	value = (ln * GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj)->upper) / tl - GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj)->page_increment;

#ifdef DEBUG
	printf("%i\n", total_lines);
	printf("%f\n", value);
	printf("%f, %f\n", GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj)->lower, GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj)->upper);
#endif

	gtk_adjustment_set_value(GTK_ADJUSTMENT(GTK_TEXT(doc->text)->vadj), value);
}


static gint
point_to_line(gE_document * doc, gint point)
{
	gint i, lines;
	gchar *c;

	lines = 0;
	i = point;
	for (i = point; i > 1; i--) {
		c = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i - 1, i);
		if (strcmp(c, "\n") == 0)
			lines++;
		g_free (c);
	}

	return lines;
}



static void 
search_start(GtkWidget * w, gE_data * data)
{
	gE_search *options;
	gE_document *doc;
	gchar *search_for, *buffer, *replace_with;
	gchar bla[] = " ";
	gint len, start_pos, text_len, match, i, search_for_line, cur_line,
	end_line, oldchanged, replace_diff = 0;
	/*options = data->temp2;*/
	options = data->window->search;
	doc = gE_document_current(data->window);
	search_for = gtk_entry_get_text(GTK_ENTRY(options->search_entry));
	replace_with = gtk_entry_get_text(GTK_ENTRY(options->replace_entry));
	buffer = g_malloc0(sizeof(search_for));
	len = strlen(search_for);
	oldchanged = doc->changed;

	if (options->again) {
		start_pos = gtk_text_get_point(GTK_TEXT(doc->text));
		options->again = FALSE;
	} else if (GTK_TOGGLE_BUTTON(options->start_at_cursor)->active)
		start_pos = GTK_EDITABLE(doc->text)->current_pos;
	else
		start_pos = 0;


	if ((text_len = gtk_text_get_length(GTK_TEXT(doc->text))) < len)
		return;



	if (GTK_CHECK_MENU_ITEM(options->line_item)->active || options->line) {
		start_pos = 0;
		cur_line = 1;
		sscanf(search_for, "%i", &search_for_line);
		for (i = start_pos; i < text_len; i++) {
			if (cur_line == search_for_line)
				break;
			buffer = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i, i + 1);
			if (strcmp(buffer, "\n") == 0)
				cur_line++;
		}
		if (i >= text_len)
			return;
		for (end_line = i; end_line < text_len; end_line++) {
			buffer = gtk_editable_get_chars(GTK_EDITABLE(doc->text), end_line, end_line + 1);
			if (strcmp(buffer, "\n") == 0)
				break;
		}
		seek_to_line(doc, search_for_line);
		gtk_editable_insert_text(GTK_EDITABLE(doc->text), bla, strlen(bla), &i);
		gtk_editable_delete_text(GTK_EDITABLE(doc->text), i - 1, i);
		doc->changed = oldchanged;
		gtk_editable_select_region(GTK_EDITABLE(doc->text), i - 1, end_line);
		return;
	}
	gtk_text_freeze(GTK_TEXT(doc->text));

	for (i = start_pos; i <= (text_len - len - replace_diff); i++) {
		buffer = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i, i + len);
		if (GTK_TOGGLE_BUTTON(options->case_sensitive)->active)
			match = strcmp(buffer, search_for);
		else
			match = g_strcasecmp(buffer, search_for);

		if (match == 0) {
			gtk_text_thaw(GTK_TEXT(doc->text));

			/* This is so the text will scroll to the selection no matter what */
			seek_to_line(doc, point_to_line(doc, i));
			gtk_editable_insert_text(GTK_EDITABLE(doc->text), bla, strlen(bla), &i);
			gtk_editable_delete_text(GTK_EDITABLE(doc->text), i - 1, i);
			i--;
			doc->changed = oldchanged;

			gtk_text_set_point(GTK_TEXT(doc->text), i + len);
			gtk_editable_select_region(GTK_EDITABLE(doc->text), i, i + len);
			g_free(buffer);
			buffer = NULL;


			if (options->replace) {
				if (GTK_TOGGLE_BUTTON(options->prompt_before_replacing)->active) {
					gtk_text_set_point(GTK_TEXT(doc->text), i + 2);
					popup_replace_window(data);
					return;
				} else {
					gtk_editable_delete_selection(GTK_EDITABLE(doc->text));
					gtk_editable_insert_text(GTK_EDITABLE(doc->text), replace_with, strlen(replace_with), &i);
					gtk_text_set_point(GTK_TEXT(doc->text), i + strlen(replace_with));
					replace_diff = replace_diff + (strlen(search_for) - strlen(replace_with));
					gtk_text_freeze(GTK_TEXT(doc->text));
				}
			} else
				break;
		}
		g_free(buffer);
	}
	gtk_text_thaw(GTK_TEXT(doc->text));
} /* search_start */


static void 
search_for_text(GtkWidget * w, gE_search * options)
{
	gtk_widget_show(options->start_at_cursor);
	gtk_widget_show(options->start_at_beginning);
	gtk_widget_show(options->case_sensitive);
	if (options->line)
		gtk_editable_delete_text(GTK_EDITABLE(options->search_entry), 0, -1);
	options->line = 0;
	if (options->replace)
		gtk_widget_show(options->replace_box);
	/*gtk_menu_item_select (GTK_MENU_ITEM (options->text_item));    */
	gtk_option_menu_set_history(GTK_OPTION_MENU(options->search_for), 0);
	gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(options->text_item), 1);
} /* search_for_text */


static void 
search_for_line(GtkWidget * w, gE_search * options)
{
	gtk_widget_hide(options->start_at_cursor);
	gtk_widget_hide(options->start_at_beginning);
	gtk_widget_hide(options->case_sensitive);
	if (!options->line)
		gtk_editable_delete_text(GTK_EDITABLE(options->search_entry), 0, -1);
	options->line = 1;
	if (options->replace) {
		gtk_widget_hide(options->replace_box);
	}
	gtk_option_menu_set_history(GTK_OPTION_MENU(options->search_for), 1);
} /* search_for_line */


/*
 * PRIVATE: search_create_dialog
 *
 * creates the search dialog box.
 */
static void
search_create_dialog(gE_window *window, gE_search *options, gboolean replace)
{
	GtkWidget *search_label, *replace_label, *ok, *cancel;
	GtkWidget *search_hbox, *replace_hbox, *hbox;
	GtkWidget *search_for_menu_items, *search_for_label;
	gE_data *data;


	options->window = gtk_dialog_new();
	gtk_window_set_policy(GTK_WINDOW(options->window), TRUE, TRUE, TRUE);

	options->replace = replace;
	gtk_window_set_title(GTK_WINDOW(options->window),
		(replace) ? "Search and Replace" : "Search");

	hbox = gtk_hbox_new(FALSE, 1);
	search_for_label = gtk_label_new("Search For:");
	gtk_widget_show(search_for_label);

	search_for_menu_items = gtk_menu_new();
	options->text_item = gtk_radio_menu_item_new_with_label(NULL, "Text");
	gtk_menu_append(GTK_MENU(search_for_menu_items), options->text_item);
	gtk_widget_show(options->text_item);
	options->line_item =
		gtk_radio_menu_item_new_with_label(
			gtk_radio_menu_item_group(
				GTK_RADIO_MENU_ITEM(options->text_item)),
			"Line Number");
	gtk_menu_append(GTK_MENU(search_for_menu_items), options->line_item);
	gtk_widget_show(options->line_item);

	options->search_for = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(options->search_for),
		search_for_menu_items);
	gtk_widget_show(options->search_for);

	gtk_widget_show(hbox);

	gtk_signal_connect(GTK_OBJECT(options->text_item), "activate",
		GTK_SIGNAL_FUNC(search_for_text), options);
	gtk_signal_connect(GTK_OBJECT(options->line_item), "activate",
		GTK_SIGNAL_FUNC(search_for_line), options);

	search_hbox = gtk_hbox_new(FALSE, 1);
	options->search_entry = gtk_entry_new();

	search_label = gtk_label_new("Search:");
	options->start_at_cursor =
		gtk_radio_button_new_with_label(
			NULL, "Start searching at cursor position");
	options->start_at_beginning =
		gtk_radio_button_new_with_label(
			gtk_radio_button_group(
				GTK_RADIO_BUTTON(options->start_at_cursor)),
			"Start searching at beginning of the document");
	options->case_sensitive =
		gtk_check_button_new_with_label("Case sensitive");

	options->replace_box = gtk_vbox_new(FALSE, 1);
	replace_hbox = gtk_hbox_new(FALSE, 1);
	replace_label = gtk_label_new("Replace:");
	options->replace_entry = gtk_entry_new();
	options->prompt_before_replacing =
		gtk_check_button_new_with_label("Prompt before replacing");

#ifdef WITHOUT_GNOME
	ok = gtk_button_new_with_label("OK");
	cancel = gtk_button_new_with_label("Cancel");
#else
	ok = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	cancel = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->vbox),
		hbox, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), search_for_label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), options->search_for, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->vbox),
		search_hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(search_hbox), search_label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(search_hbox), options->search_entry,
		TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->vbox),
		options->start_at_cursor, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->vbox),
		options->start_at_beginning, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->vbox),
		options->case_sensitive, TRUE, TRUE, 2);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->vbox),
		options->replace_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(options->replace_box), replace_hbox,
		TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(replace_hbox), replace_label,
		FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(replace_hbox), options->replace_entry,
		TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(options->replace_box),
		options->prompt_before_replacing, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->action_area),
		ok, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(options->window)->action_area),
		cancel, TRUE, TRUE, 2);

	gtk_widget_show(search_hbox);
	gtk_widget_show(options->search_entry);
	gtk_widget_show(search_label);
	gtk_widget_show(options->start_at_cursor);
	gtk_widget_show(options->start_at_beginning);
	gtk_widget_show(options->case_sensitive);

	if (replace)
		gtk_widget_show(options->replace_box);
	else
		gtk_widget_hide(options->replace_box);

	gtk_widget_show(replace_hbox);
	gtk_widget_show(replace_label);
	gtk_widget_show(options->replace_entry);
	gtk_widget_show(options->prompt_before_replacing);
	gtk_widget_show(ok);
	gtk_widget_show(cancel);
	/*
	 * gtk_widget_show (options->window);
	 */

	data = g_malloc0(sizeof(gE_data));
	data->window = window;
	data->temp2 = options;
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		GTK_SIGNAL_FUNC(search_start), data);
	gtk_signal_connect_object(GTK_OBJECT(ok), "clicked",
		GTK_SIGNAL_FUNC(gtk_widget_hide), (gpointer) options->window);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET(ok), GTK_CAN_DEFAULT);
	gtk_widget_grab_default (GTK_WIDGET(ok));
	 
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
		GTK_SIGNAL_FUNC(gtk_widget_hide), (gpointer) options->window);
} /* search_create_dialog */


static void 
search_replace_yes_sel(GtkWidget * w, gE_data * data)
{
	gchar *rep;
	gE_document *doc;
	gint i;

	doc = gE_document_current(data->window);
	rep = gtk_entry_get_text(
			GTK_ENTRY(data->window->search->replace_entry));
	i = gtk_text_get_point(GTK_TEXT(doc->text)) - 2;
	gtk_editable_delete_selection(GTK_EDITABLE(doc->text));
	gtk_editable_insert_text(GTK_EDITABLE(doc->text), rep, strlen(rep), &i);
	gtk_text_set_point(GTK_TEXT(doc->text), i + strlen(rep));
	gtk_toggle_button_set_state(
		GTK_TOGGLE_BUTTON(data->window->search->start_at_cursor), TRUE);
	data->temp2 = data->window->search;
	search_start(w, data);
} /* search_replace_yes_sel */


static void 
search_replace_no_sel(GtkWidget * w, gE_data * data)
{
	data->window->search->again = TRUE;
	data->temp2 = data->window->search;
	search_start(w, data);
} /* search_replace_no_sel */


/*
 * PRIVATE: popup_replace_window
 *
 * create popup to ask user whether or not to replace text
 */
#define REPLACE_TITLE   "Replace Text"
#define REPLACE_MSG     " Are you sure you want to replace this? "
static void
popup_replace_window(gE_data *data)
{
	char *title, *msg;
	char *buttons[] = { GE_BUTTON_YES, GE_BUTTON_NO, GE_BUTTON_CANCEL } ;
	int ret;

	title = g_strdup(REPLACE_TITLE);
	msg   = g_strdup(REPLACE_MSG);

	ret = ge_dialog(title, msg, 3, buttons, 3, NULL, NULL, TRUE);

	g_free(msg);
	g_free(title);

	switch (ret) {
	case 1 :
		search_replace_yes_sel(NULL, data);
		break;
	case 2 :
		search_replace_no_sel(NULL, data);
		break;
	case 3 :
		/* do nothing, user cancelled */
		break;
	default:
		printf("popup_replace_window: ge_dialog returned %d\n", ret);
		exit(-1);
	} /* switch */

} /* popup_replace_window */

/*
 * PRIVATE: search_replace_common
 *
 * common routine for search and search/replace callbacks.
 */
static void
search_replace_common(gE_data *data, gboolean do_replace)
{
	gE_window *w;
	gE_data *d;

	g_assert(data != NULL);
	w = data->window;
	g_assert(w != NULL);
	g_assert(w->search != NULL);

	if (!w->search->window)
		search_create_dialog(data->window, w->search, do_replace);

	w->search->replace = do_replace;
	w->search->again = FALSE;
	search_for_text(NULL, w->search);
	gtk_window_set_title(GTK_WINDOW(w->search->window),
			(do_replace) ? "Search and Replace" : "Search");

	/*d = g_malloc0(sizeof(gE_data));
	d->window = w;
	d->temp2 = w->search;
*/
	if (do_replace)
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (w->search->search_entry),
			GTK_SIGNAL_FUNC (search_start), data);
		gtk_signal_disconnect_by_func (GTK_OBJECT (w->search->search_entry),
			GTK_SIGNAL_FUNC (gtk_widget_hide), (gpointer)w->search->window);
		gtk_widget_show(w->search->replace_box);
	}
	else
	{
		gtk_signal_connect (GTK_OBJECT (w->search->search_entry), "activate",
			GTK_SIGNAL_FUNC (search_start), data);
		gtk_signal_connect_object (GTK_OBJECT (w->search->search_entry), "activate",
			GTK_SIGNAL_FUNC (gtk_widget_hide), (gpointer) w->search->window);
		gtk_widget_hide(w->search->replace_box);
	}
	
	gtk_widget_show(w->search->window);
} /* search_replace_common */

/* the end */
