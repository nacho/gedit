/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
 *
 * gE_print.c - Print Functions.
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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <time.h>
#include "main.h"
#include "gE_print.h"
#include "gE_files.h"
#include "gE_document.h"
#include "gE_mdi.h"
#include "commands.h"
#include "dialog.h"
#include "gE_prefs.h"

static void print_destroy(GtkWidget *widget, gpointer data);
static void file_print_execute(GtkWidget *w, gpointer cbdata);
static char *generate_temp_file(gE_document *doc);
static char *get_filename(gE_data *data);

/* these should probably go into gE_window */
static GtkWidget *print_dialog = NULL;
static GtkWidget *print_cmd_entry = NULL;


/*
 * PUBLIC: file_print_cb
 *
 * creates print dialog box.  this should be the only routine global to
 * the world.
 */
void
file_print_cb(GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *tmp;
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);

	if (print_dialog)
		return;

	print_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(print_dialog), _("Print"));
	gtk_signal_connect(GTK_OBJECT(print_dialog), "destroy",
		GTK_SIGNAL_FUNC(print_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(print_dialog), vbox);
	gtk_container_border_width(GTK_CONTAINER(print_dialog), 6);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);
	gtk_widget_show(hbox);

	tmp = gtk_label_new(_("Enter print command below\nRemember to include '%s'"));
	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 5);
	gtk_widget_show(tmp);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 5);
	gtk_widget_show(hbox);

	tmp = gtk_label_new(_("Print Command:"));
	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 5);
	gtk_widget_show(tmp);

	print_cmd_entry = gtk_entry_new_with_max_length(255);
	if (settings->print_cmd)
		gtk_entry_set_text(GTK_ENTRY(print_cmd_entry),
			settings->print_cmd);
	gtk_box_pack_start(GTK_BOX(hbox), print_cmd_entry, TRUE, TRUE, 10);
	gtk_widget_show(print_cmd_entry);

	tmp = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, TRUE, 10);
	gtk_widget_show(tmp);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 5);
	gtk_widget_show(hbox);

	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 15);
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
		GTK_SIGNAL_FUNC(file_print_execute), data);
	gtk_widget_show(tmp);

	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 15);
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
		GTK_SIGNAL_FUNC(print_destroy), NULL);
	gtk_widget_show(tmp);

	gtk_widget_show(print_dialog);
} /* file_print_cb */


/*
 * PRIVATE: print_destroy
 *
 * destroy the print dialog box
 */
static void
print_destroy(GtkWidget *widget, gpointer data)
{
	if (print_dialog) {
		gtk_widget_destroy(print_dialog);
		print_dialog = NULL;
	}
} /* print_destroy */


/*
 * PRIVATE: file_print_execute
 *
 * actually execute the print command
 */
static void
file_print_execute(GtkWidget *w, gpointer cbdata)
{
	gchar *scmd, *pcmd, *tmp, *fname;
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);

	if (settings->print_cmd)
		strcpy(settings->print_cmd,
			gtk_entry_get_text(GTK_ENTRY(print_cmd_entry)));

	/* print using specified command */
	if ((pcmd = gtk_entry_get_text(GTK_ENTRY(print_cmd_entry))) == NULL)
		return;

	/* look for "file variable" and place marker */
	if ((tmp = strstr(pcmd, "%s")) == NULL)
		return;
	*tmp = '\0';
	tmp += 2;

	if ((fname = get_filename(data)) == NULL) {
		print_destroy(NULL, NULL);
		return;	/* canceled */
	}

	/* build command and execute; g_malloc handles memory alloc errors */
	scmd = g_malloc(strlen(pcmd) + strlen(fname) + 1);
	sprintf(scmd, "%s%s%s", pcmd, fname, tmp);
#ifdef DEBUG
	g_print("%s\n", scmd);
#endif
	if (system(scmd) == -1)
		perror("file_print_execute: system() error");
	/*gE_msgbar_set(data->window, MSGBAR_FILE_PRINTED);*/
	gnome_app_flash (mdi->active_window, _(MSGBAR_FILE_PRINTED));

	g_free(scmd);

	/* delete temporary file.  TODO: allow temp dir to be customizable */
	if (strncmp(fname, "/tmp", 4) == 0)
		if (unlink(fname))
			perror("file_print_execute: unlink() error");

	g_free(fname);

	print_destroy(NULL, NULL);
} /* file_print_execute */


/*
 * PRIVATE: get_filename
 *
 * returns the filename to be printed in a newly allocated buffer.
 * the filename is determined by this algorithm:
 *
 * 1. if the file to be printed has not been updated and exists on disk,
 * then print the file on disk immediately.
 * 2. if the file has been updated, or if it is "Untitled", we should
 * prompt the user whether or not to save the file, and then print it.
 * 2a.  if the user does not want to save it, then this is the only time
 * we actually need to create a temporary file.
 */
#define PRINT_TITLE	"Save before printing?"
#define PRINT_MSG	"has not been saved!"
static char *
get_filename(gE_data *data)
{
	gE_document *doc;
	char *fname = NULL;
	char *buttons[] =
		{ N_("Print anyway"), N_(" Save, then print "), GE_BUTTON_CANCEL };
	char *title, *msg;

	g_assert(data != NULL);
	g_assert(data->window != NULL);
	doc = gE_document_current();

	if (!doc->changed && doc->filename) {
		if (doc->sb == NULL)
			doc->sb = g_malloc(sizeof(struct stat));
		if (stat(doc->filename, doc->sb) == -1) {
			/* print warning */
			g_free(doc->sb);
			doc->sb = NULL;
			fname = generate_temp_file(doc);
		} else
			fname = g_strdup(doc->filename);
	} else { /* doc changed or no filename */
		int ret;

		title = g_strdup(_(PRINT_TITLE));
		if (doc->filename)
			msg = (char *)g_malloc(strlen(_(PRINT_MSG)) +
						strlen(doc->filename) + 6);
		else
			msg = (char *)g_malloc(strlen(_(PRINT_MSG)) +
						strlen(_(UNTITLED)) + 6);
		sprintf(msg, " '%s' %s ", 
			(doc->filename) ? doc->filename : _(UNTITLED), PRINT_MSG);

		ret = gnome_dialog_run_and_close ((GnomeDialog *)
			gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION, 
				buttons[0], buttons[1], buttons[2], NULL)) + 1;
		
		g_free(msg);
		g_free(title);

		switch (ret) {
		case 1 :
			fname = generate_temp_file(doc);
			break;
		case 2 :
			if (doc->filename == NULL) {
				data->temp1 = (gpointer)TRUE;	/* non-zero */
				file_save_as_cb(NULL, data);
				while (data->temp1 != NULL)
					gtk_main_iteration_do(TRUE);
			} else
				gE_file_save(doc, doc->filename);

			fname = g_strdup(doc->filename);
			break;
		case 3 :
			fname = NULL;
			break;
		default:
			printf("get_filename: ge_dialog returned %d\n", ret);
			exit(-1);
		} /* switch */
	} /* doc changed or no filename */

	return fname;
} /* get_filename */


/*
 * PRIVATE: generate_temp_file
 *
 * create and write to temp file.  returns name of temp file in malloc'd
 * buffer (needs to be freed afterwards).
 *
 * TODO: define and use system wide temp directory (saved in preferences).
 */
static char *
generate_temp_file(gE_document *doc)
{
	FILE *fp;
	char *fname;

	fname = (char *)g_malloc(STRING_LENGTH_MAX);

	sprintf(fname, "/tmp/.gedit_print.%d%d", (int)time(NULL), getpid());
	if ((fp = fopen(fname, "w")) == NULL) {
		g_error("generate_temp_file: unable to open '%s'\n", fname);
		g_free(fname);
		return NULL;
	}

	if (fputs(gtk_editable_get_chars(GTK_EDITABLE(doc->text), 0,
		gtk_text_get_length(GTK_TEXT(doc->text))), fp) == EOF) {

		perror("generate_temp_file: can't write to tmp file");
		g_free(fname);
		fclose(fp);
		return NULL;
	}
	fflush(fp);
	fclose(fp);

	return fname;
} /* generate_temp_file */

/* the end */
