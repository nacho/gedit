#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include <gnome.h>

#include "main.h"
#include "menus.h"

/* ---- Misc callbacks ---- */

void save_no_sel (GtkWidget *w, gpointer data);
void save_yes_sel (GtkWidget *w, gpointer data);
void popup_close_verify (gE_document *doc);

/* handles changes in the text widget... */
void document_changed_callback (GtkWidget *w, gpointer doc)
{
	gE_document *document;
	document = (gE_document *) doc;
	/* g_print ("change signal emitted...\n"); */ /* was useful for debugging... */
	document->changed = TRUE;
	gtk_signal_disconnect (GTK_OBJECT(document->text), (gint) document->changed_id);
	document->changed_id = FALSE;
}

void save_yes_sel (GtkWidget *w, gpointer data)
{
	gE_document *doc;
	doc = (gE_document *) data;
	file_save_cmd_callback (NULL, NULL);
	if (doc->filename == NULL)
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(main_window->save_fileselector)->ok_button), "clicked",
		(GtkSignalFunc) file_close_cmd_callback, data);
	else
		file_close_cmd_callback (NULL, NULL);
}

void save_no_sel (GtkWidget *w, gpointer data)
{
	gE_document *doc;
	doc = (gE_document *) data;
	doc->changed = FALSE;
	file_close_cmd_callback (NULL, NULL);
}

void popup_close_verify(gE_document *doc)
{
	GtkWidget *verify_window, *yes, *no, *cancel, *label;
	verify_window = gtk_dialog_new();
	
	gtk_window_set_title (GTK_WINDOW(verify_window), _("Save File?"));
	gtk_widget_set_usize (GTK_WIDGET (verify_window), 280, 90);
	label = gtk_label_new (_("File has been modified, do you wish to save it?"));
#ifdef WITHOUT_GNOME
	yes = gtk_button_new_with_label ("Yes");
	no = gtk_button_new_with_label ("No");
	cancel = gtk_button_new_with_label ("Cancel");
#else
	/* This whole code could be enhanced by using gnome_messagebox */
	yes    = gnome_stock_button (GNOME_STOCK_BUTTON_YES);
	no     = gnome_stock_button (GNOME_STOCK_BUTTON_NO);
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (verify_window)->vbox), label, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (verify_window)->action_area), yes, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (verify_window)->action_area), no, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (verify_window)->action_area), cancel, TRUE, TRUE, 2);
	gtk_widget_show (label);
	gtk_widget_show (yes);
	gtk_widget_show (no);
	gtk_widget_show (cancel);
	gtk_widget_show (verify_window);
	gtk_signal_connect (GTK_OBJECT(yes), "clicked", GTK_SIGNAL_FUNC(save_yes_sel), doc);
	gtk_signal_connect (GTK_OBJECT(no), "clicked", GTK_SIGNAL_FUNC(save_no_sel), doc);
	gtk_signal_connect_object (GTK_OBJECT(yes), "clicked", 
	                                       GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) verify_window);
	gtk_signal_connect_object (GTK_OBJECT(no), "clicked", 
	                                       GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) verify_window);
	gtk_signal_connect_object (GTK_OBJECT(cancel), "clicked",
	                                       GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) verify_window);
	gtk_grab_add (verify_window);
}



void file_open_ok_sel (GtkWidget *w, GtkFileSelection *fs)
{
  char *filename;
  char *nfile;
  if ((gE_document_current (main_window)->filename) || 
      (gE_document_current (main_window)->changed))
  	gE_document_new(main_window);
   
   filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
   
  if (filename != NULL) {
    nfile = g_malloc(strlen(filename)+1);
    strcpy(nfile, filename);
    gE_file_open (gE_document_current(main_window), nfile);
  }
  if (GTK_WIDGET_VISIBLE(fs))
    gtk_widget_hide (GTK_WIDGET(fs));
}

void file_save_ok_sel (GtkWidget *w, GtkFileSelection *fs)
{
  char *filename;
  filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
  if (filename != NULL)
    gE_file_save (gE_document_current(main_window), filename);
  if (GTK_WIDGET_VISIBLE(fs))
    gtk_widget_hide (GTK_WIDGET(fs));
}

void file_cancel_sel (GtkWidget *w, GtkFileSelection *fs)
{
  if (GTK_WIDGET_VISIBLE(fs))
    gtk_widget_hide (GTK_WIDGET(fs));
}

void destroy (GtkWidget *w, GtkFileSelection *fs)
{
	fs = NULL;
}

void prefs_callback (GtkWidget *widget, gpointer data)
{
	prefs_window = gE_prefs_window();
}


/* ---- Auto-indent Callback(s) --- */

void auto_indent_toggle_callback (GtkWidget *w, gpointer data)
{
	main_window->auto_indent = !main_window->auto_indent;
}

void auto_indent_callback (GtkWidget *text, GdkEventKey *event)
{
	int i, newlines, newline_1;
	gchar *buffer, *whitespace;

	line_pos_callback (NULL, text);

	if (event->keyval != GDK_Return)
		return;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return;
	if (!main_window->auto_indent)
		return;

	newlines = 0;
	for (i = GTK_EDITABLE (text)->current_pos; i > 0; i--)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i-1, i);
		if (buffer == NULL)
			continue;
		if (buffer[0] == 10)
		{
			if (newlines > 0)
			{
				g_free (buffer);
				break;
			}
			else {
				newlines++;
				newline_1 = i;
				g_free (buffer);
			}
		}
	}

	whitespace = g_malloc0 (newline_1 - i + 2);

	for (i = i; i <= newline_1; i++)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i, i+1);
		if ((buffer[0] != 32) & (buffer[0] != 9))
			break;
		strncat (whitespace, buffer, 1);
		g_free (buffer);
	}

	if (strlen(whitespace) > 0)
	{
		i = GTK_EDITABLE (text)->current_pos;
		gtk_editable_insert_text (GTK_EDITABLE (text), whitespace, strlen(whitespace), &i);
	}
	
	g_free (whitespace);
	line_pos_callback (NULL, text); /* <-- this is so the statusbar updates when it auto-indents */
}


void line_pos_callback(GtkWidget *w, GtkWidget *text)
{
	static char line [32];
	static char col [32];
	int timer = 0;	

	if (main_window->documents > 0)
	{

		sprintf (line,"%d", GTK_TEXT(text)->cursor_pos_y/13);
		sprintf (col, "%d", GTK_TEXT(text)->cursor_pos_x/7);
	
		gtk_label_set (GTK_LABEL(main_window->line_label), line);
		gtk_label_set (GTK_LABEL(main_window->col_label), col);
	}
}


void gE_event_button_press (GtkWidget *w, GdkEventButton *event)
{
	line_pos_callback (NULL, w);
}


/* ---- File Menu Callbacks ---- */

void file_new_cmd_callback (GtkWidget *widget, gpointer data)
{
	gtk_statusbar_push (GTK_STATUSBAR(main_window->statusbar), 1, _("New File..."));
  gE_document_new(main_window);
}

void file_open_cmd_callback (GtkWidget *widget, gpointer data)
{
  if (main_window->open_fileselector == NULL) {
	main_window->open_fileselector = gtk_file_selection_new(_("Open File..."));
	gtk_signal_connect (GTK_OBJECT (main_window->open_fileselector), 
		"destroy", (GtkSignalFunc) destroy, main_window->open_fileselector);
	gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (main_window->open_fileselector)->ok_button), 
		"clicked", (GtkSignalFunc) file_open_ok_sel, main_window->open_fileselector);
	gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (main_window->open_fileselector)->cancel_button),
		"clicked", (GtkSignalFunc) file_cancel_sel, main_window->open_fileselector);
  }

  if (GTK_WIDGET_VISIBLE(main_window->open_fileselector))
    return;
  else {
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (main_window->open_fileselector), "./");
    gtk_widget_show (main_window->open_fileselector);
  }
}

void file_save_cmd_callback (GtkWidget *widget, gpointer data)
{
	gchar *fname;
 	fname = gE_document_current(main_window)->filename;
	if (fname == NULL)
	{
		#ifdef DEBUG
		g_warning("The file hasn't been saved yet!\n");
		#endif
		file_save_as_cmd_callback(NULL, NULL);
	}
	else
		gE_file_save (gE_document_current(main_window), (gE_document_current(main_window))->filename);
}

void file_save_as_cmd_callback (GtkWidget *widget, gpointer data)
{
	if (main_window->save_fileselector == NULL) {
		main_window->save_fileselector = gtk_file_selection_new(_("Save As..."));
		gtk_signal_connect (GTK_OBJECT (main_window->save_fileselector), 
			"destroy", (GtkSignalFunc) destroy, main_window->save_fileselector);
		gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (main_window->save_fileselector)->ok_button), 
			"clicked", (GtkSignalFunc) file_save_ok_sel, main_window->save_fileselector);
		gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (main_window->save_fileselector)->cancel_button),
			"clicked", (GtkSignalFunc) file_cancel_sel, main_window->save_fileselector);
  	}
	if (GTK_WIDGET_VISIBLE(main_window->save_fileselector))
    		return;
  	else {
  		gtk_file_selection_set_filename (GTK_FILE_SELECTION (main_window->save_fileselector), "./");
    		gtk_widget_show (main_window->save_fileselector);
	}
}

void file_close_cmd_callback (GtkWidget *widget, gpointer data)
{
  /* This works, but sometimes  segfaults! need someway to check if file has been edited....
  				- Alex */
 /* OK, pretty sure I've fixed the segfaults, will need to test it more though... 
                        -Evan */
/* Checks if the file has been edited or not now too...
				-Evan */

	gE_document *doc;

	doc = gE_document_current(main_window);
	if (doc->changed != TRUE) {
		if (g_list_length(((GtkNotebook *)(main_window->notebook))->first_tab) > 1) {
			gtk_notebook_remove_page(GTK_NOTEBOOK(main_window->notebook),
				gtk_notebook_current_page (GTK_NOTEBOOK(main_window->notebook)));
			g_list_remove(main_window->documents, doc);
			if (doc->filename != NULL)
				g_free (doc->filename);
			g_free (doc);
			gtk_statusbar_push (GTK_STATUSBAR(main_window->statusbar), 1, _("File Closed..."));
		}
		else {
			gtk_notebook_remove_page(GTK_NOTEBOOK(main_window->notebook),
						 gtk_notebook_current_page (GTK_NOTEBOOK(main_window->notebook)));
			if (doc->filename != NULL)
				g_free (doc->filename);
			g_free (doc);
			g_list_free (main_window->documents);
			main_window->documents = NULL;
			gE_document_new (main_window);
		}
	}
	else
		popup_close_verify(doc);

		gtk_statusbar_pop (GTK_STATUSBAR(main_window->statusbar), 1);

}



/* ---- Clipboard Callbacks ---- */

void edit_cut_cmd_callback (GtkWidget *widget, gpointer data)
{
   gtk_editable_cut_clipboard (GTK_EDITABLE(gE_document_current(main_window)->text), Ctime);
}

void edit_copy_cmd_callback (GtkWidget *widget, gpointer data)
{
   gtk_editable_copy_clipboard (GTK_EDITABLE(gE_document_current(main_window)->text), Ctime);
}
	
void edit_paste_cmd_callback (GtkWidget *widget, gpointer data)
{
   gtk_editable_paste_clipboard (GTK_EDITABLE(gE_document_current(main_window)->text), Ctime);
}

void edit_selall_cmd_callback (GtkWidget *widget, gpointer data)
{
   gtk_editable_select_region (GTK_EDITABLE(gE_document_current(main_window)->text),
   			       0,
			      (gtk_text_get_length(GTK_TEXT(gE_document_current(main_window)->text))));
}


/* ---- Search and Replace functions ---- */
void popup_replace_window();

void search_start (GtkWidget *w, gE_search *options)
{
	gE_document *doc;
	gchar *search_for, *buffer, *replace_with;
	gchar bla[] = " ";
	gint len, start_pos, text_len, match, i, replace_diff = 0;
	doc = gE_document_current (main_window);
	search_for = gtk_entry_get_text (GTK_ENTRY(options->search_entry));
	replace_with = gtk_entry_get_text (GTK_ENTRY(options->replace_entry));
	buffer = g_malloc0 (sizeof(search_for));
	len = strlen(search_for);

	if (options->again) {
		start_pos = gtk_text_get_point (GTK_TEXT (doc->text));
		options->again = 0;
	}
	else if (GTK_TOGGLE_BUTTON(options->start_at_cursor)->active)
		start_pos = GTK_EDITABLE(doc->text)->current_pos;
	else
		start_pos = 0;

	if ((text_len = gtk_text_get_length(GTK_TEXT(doc->text))) < len)
		return;
	gtk_text_freeze (GTK_TEXT(doc->text));

/*	for (i = start_pos; i <= (text_len - start_pos - len - replace_diff); i++)
*/
	for (i = start_pos; i <= (text_len - len - replace_diff); i++)
        {
                buffer = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i, i+len);
                if (GTK_TOGGLE_BUTTON(options->case_sensitive)->active)
			match = strcmp(buffer, search_for);
		else
			match = strcasecmp(buffer, search_for);

		if (match == 0) {
			gtk_text_thaw (GTK_TEXT(doc->text));

			/* This is so the text will scroll to the selection no matter what */
			gtk_editable_insert_text (GTK_EDITABLE(doc->text), bla, strlen(bla), &i);
			gtk_editable_delete_text (GTK_EDITABLE(doc->text), i-1, i);
			i--;

			gtk_text_set_point (GTK_TEXT(doc->text), i+len);
			gtk_editable_select_region (GTK_EDITABLE(doc->text), i, i+len);
			g_free (buffer);
			buffer = NULL;

			
			if (options->replace)
			{
				if (GTK_TOGGLE_BUTTON(options->prompt_before_replacing)->active) {
					gtk_text_set_point (GTK_TEXT(doc->text), i+2);
					popup_replace_window();
					return;
				}
				else {
					gtk_editable_delete_selection (GTK_EDITABLE(doc->text));
					/*gtk_text_insert (GTK_TEXT(doc->text), NULL, &doc->text->style->black, NULL, replace_with, strlen(replace_with));*/
					gtk_editable_insert_text (GTK_EDITABLE(doc->text), replace_with, strlen(replace_with), &i);
					gtk_text_set_point (GTK_TEXT(doc->text), i+strlen(replace_with));
					replace_diff = replace_diff + (strlen(search_for) - strlen(replace_with));
					gtk_text_freeze (GTK_TEXT(doc->text));
				}
			}
			else
				break;
		}
		g_free (buffer);
	}
	gtk_text_thaw (GTK_TEXT (doc->text));
	
}




void search_popup (gE_search *options, gint replace)
{
	GtkWidget *search_label, *replace_label, *ok, *cancel, *search_hbox, *replace_hbox;
	options->window = gtk_dialog_new();
	gtk_window_set_policy (GTK_WINDOW(options->window), TRUE, TRUE, TRUE);

	if (!replace) {
		options->replace = 0;
		gtk_window_set_title (GTK_WINDOW (options->window), _("Search"));
	}
	else {
		options->replace = 1;
		gtk_window_set_title (GTK_WINDOW (options->window), _("Search and Replace"));
	}

	search_hbox = gtk_hbox_new(FALSE, 1);
	options->search_entry = gtk_entry_new();
	search_label = gtk_label_new (_("Search:"));
	options->start_at_cursor = gtk_radio_button_new_with_label (NULL, _("Start searching at cursor position"));
	options->start_at_beginning = gtk_radio_button_new_with_label(
	                                           gtk_radio_button_group(GTK_RADIO_BUTTON(options->start_at_cursor)),
	                                           _("Start searching at beginning of the document"));
	options->case_sensitive = gtk_check_button_new_with_label (_("Case sensitive"));
	
	options->replace_box = gtk_vbox_new(FALSE, 1);
	replace_hbox = gtk_hbox_new(FALSE, 1);
	replace_label = gtk_label_new (_("Replace:"));
	options->replace_entry = gtk_entry_new();
	options->prompt_before_replacing = gtk_check_button_new_with_label (_("Prompt before replacing"));

#ifdef WITHOUT_GNOME
	ok = gtk_button_new_with_label ("OK");
	cancel = gtk_button_new_with_label ("Cancel");
#else
	ok = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), search_hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(search_hbox), search_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX(search_hbox), options->search_entry, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->start_at_cursor, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->start_at_beginning, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->case_sensitive, TRUE, TRUE, 2);

	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->replace_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(options->replace_box), replace_hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(replace_hbox), replace_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX(replace_hbox), options->replace_entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(options->replace_box), options->prompt_before_replacing, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->action_area), ok, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->action_area), cancel, TRUE, TRUE, 2);

	gtk_widget_show (search_hbox);
	gtk_widget_show (options->search_entry);
	gtk_widget_show (search_label);
	gtk_widget_show (options->start_at_cursor);
	gtk_widget_show (options->start_at_beginning);
	gtk_widget_show (options->case_sensitive);

	if (replace)
		gtk_widget_show (options->replace_box);

	gtk_widget_show (replace_hbox);
	gtk_widget_show (replace_label);
	gtk_widget_show (options->replace_entry);
	gtk_widget_show (options->prompt_before_replacing);
	gtk_widget_show (ok);
	gtk_widget_show (cancel);
	gtk_widget_show (options->window);
	gtk_signal_connect (GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(search_start), options);
	gtk_signal_connect_object (GTK_OBJECT(ok), "clicked", 
	                                       GTK_SIGNAL_FUNC(gtk_widget_hide), (gpointer) options->window);
	/*GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (ok);*/
	gtk_signal_connect_object (GTK_OBJECT(cancel), "clicked", 
	                                       GTK_SIGNAL_FUNC(gtk_widget_hide), (gpointer) options->window);
}



void search_search_cmd_callback (GtkWidget *w, gpointer data)
{
	if (!main_window->search->window)
		search_popup (main_window->search, 0);
	else {
		gtk_widget_show (main_window->search->window);
		gtk_widget_hide (main_window->search->replace_box);
		main_window->search->replace = 0;
		main_window->search->again = 0;
	}
}

void search_replace_cmd_callback (GtkWidget *w, gpointer data)
{
	if (!main_window->search->window)
		search_popup (main_window->search, 1);
	else {
		gtk_widget_show (main_window->search->window);
		gtk_window_set_title (GTK_WINDOW (main_window->search->window), _("Search and Replace"));
		gtk_widget_show (main_window->search->replace_box);
		main_window->search->replace = 1;
		main_window->search->again = 0;
	}
}

void search_again_cmd_callback (GtkWidget *w, gpointer data)
{
	if (main_window->search->window) {
		main_window->search->replace = 0;
		main_window->search->again = 1;
		search_start (w, main_window->search);
	}
}

void search_replace_yes_sel (GtkWidget *w, gpointer data)
{
	gchar *replace_with;
	gE_document *doc;
	gint i;
	doc = gE_document_current (main_window);
	replace_with = gtk_entry_get_text (GTK_ENTRY(main_window->search->replace_entry));
	i = gtk_text_get_point (GTK_TEXT (doc->text)) - 2;
	gtk_editable_delete_selection (GTK_EDITABLE(doc->text));
	gtk_editable_insert_text (GTK_EDITABLE(doc->text), replace_with, strlen(replace_with), &i);
	gtk_text_set_point (GTK_TEXT(doc->text), i+strlen(replace_with));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(main_window->search->start_at_cursor), TRUE);
	search_start (w, main_window->search);
}

void search_replace_no_sel (GtkWidget *w, gpointer data)
{
	/*gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(main_window->search->start_at_cursor), TRUE);*/
	main_window->search->again = 1;
	search_start (w, main_window->search);
}

void popup_replace_window()
{
	GtkWidget *window, *yes, *no, *cancel, *label;
	window = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW(window), _("Replace?"));
	gtk_widget_set_usize (GTK_WIDGET (window), 280, 90);
	label = gtk_label_new (_("Are you sure you want to replace this?"));
#ifdef WITHOUT_GNOME
	yes = gtk_button_new_with_label ("Yes");
	no = gtk_button_new_with_label ("No");
	cancel = gtk_button_new_with_label ("Cancel");
#else
	/* This whole code could be enhanced by using gnome_messagebox */
	yes    = gnome_stock_button (GNOME_STOCK_BUTTON_YES);
	no     = gnome_stock_button (GNOME_STOCK_BUTTON_NO);
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), yes, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), no, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, TRUE, TRUE, 2);
	gtk_widget_show (label);
	gtk_widget_show (yes);
	gtk_widget_show (no);
	gtk_widget_show (cancel);
	gtk_widget_show (window);
	gtk_signal_connect (GTK_OBJECT(yes), "clicked", GTK_SIGNAL_FUNC(search_replace_yes_sel), NULL);
	gtk_signal_connect (GTK_OBJECT(no), "clicked", GTK_SIGNAL_FUNC(search_replace_no_sel), NULL);
	gtk_signal_connect_object (GTK_OBJECT(yes), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) window);
	gtk_signal_connect_object (GTK_OBJECT(no), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) window);
	gtk_signal_connect_object (GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) window);
	gtk_grab_add (window);
}




