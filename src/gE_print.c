/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
 *
 * gE_print.c - Print Functions.
 * Code borrowed from gIDE (http://www.pn.org/gIDE)
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

#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#else
#include <time.h>
#include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "gE_print.h"
#include "gE_document.h"

static void print_destroy(GtkWidget *widget, gpointer data);
static void file_print_execute(GtkWidget *w, gpointer cbdata);

static GtkWidget *print_dialog;
static GtkWidget *print_cmd_entry;


static void
print_destroy(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy( print_dialog );
    print_dialog = NULL;
}


/*
 * heavily modified from _file_print() in gIDE.
 *
 * TODO: currently, this routine always creates a temporary file and
 * then operates on it.  we can improve on this!
 * 1. if the file to be printed has not been updated and exists on disk,
 * then print the file on disk immediately.
 * 2. if the file has been updated, or if it is "Untitled", we should
 * prompt the user whether or not to save the file, and then print it.
 * 2a.  if the user does not want to save it, then this is the only time
 * we actually need to create a temporary file.
 */
static void
file_print_execute(GtkWidget *w, gpointer cbdata)
{
	FILE *fp;
	gE_document *current;
	gchar fname[STRING_LENGTH_MAX];
	gchar *scmd, *pcmd, *tmp;
	gE_data *data = (gE_data *) cbdata;

	current = gE_document_current(data->window);
#ifndef WITHOUT_GNOME
	strcpy(data->window->print_cmd,
		   gtk_entry_get_text(GTK_ENTRY(print_cmd_entry)));
#endif

	/* print using specified command */
	if ((pcmd = gtk_entry_get_text(GTK_ENTRY(print_cmd_entry))) == NULL)
		return;

	/* look for "file variable" and place marker */
	if ((tmp = strstr(pcmd, "%s")) == NULL)
		return;
	*tmp = '\0';
	tmp += 2;

	/*
	 * create and write to temp file.  TODO: define and use system wide
	 * temporary directory (saved in preferences).
	 */
	sprintf(fname, "/tmp/.gedit_print.%d%d", time(NULL), getpid());
	if ((fp = fopen(fname, "w")) == NULL) {
		g_error("\n    Unable to Open '%s'    \n", fname);
		return;
	}
	if (fputs(
			 gtk_editable_get_chars(GTK_EDITABLE(current->text), 0,
				  gtk_text_get_length(GTK_TEXT(current->text))),
			 fp) == EOF) {
		perror("file_print_execute: can't write to tmp file");
		fclose(fp);
		return;
	}
	fclose(fp);

	/* build command and execute; g_malloc handles memory alloc errors */
	scmd = g_malloc(strlen(pcmd) + strlen(fname) + 1);
	sprintf(scmd, "%s%s%s", pcmd, fname, tmp);
#ifdef DEBUG
	g_print("%s\n", scmd);
#endif
	if (system(scmd) == -1)
		perror("file_print_execute: system() error");
	gE_msgbar_set(data->window, MSGBAR_FILE_PRINTED);

	g_free(scmd);

	/* delete temporary file - not done in gIDE (duh!  totally braindead) */
	if (unlink(fname))
		perror("file_print_execute: unlink() error");

	print_destroy(NULL, NULL);
} /* file_print_execute */


/* ---- Print Function ---- */

void file_print_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *tmp;
	gE_data *data = (gE_data *) cbdata;

	print_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(print_dialog), "Print");
	gtk_signal_connect(GTK_OBJECT(print_dialog), "destroy",
			   GTK_SIGNAL_FUNC(print_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(print_dialog), vbox);
	gtk_container_border_width(GTK_CONTAINER(print_dialog), 6);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);
	gtk_widget_show(hbox);

	tmp = gtk_label_new("Enter print command below\nRemember to include '%s'");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 5);
	gtk_widget_show(tmp);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 5);
	gtk_widget_show(hbox);

	tmp = gtk_label_new("Print Command:");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 5);
	gtk_widget_show(tmp);

	print_cmd_entry = gtk_entry_new_with_max_length(255);
#ifndef WITHOUT_GNOME
	gtk_entry_set_text(GTK_ENTRY(print_cmd_entry), data->window->print_cmd);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), print_cmd_entry, FALSE, TRUE, 10);
	gtk_widget_show(print_cmd_entry);

	tmp = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, TRUE, 10);
	gtk_widget_show(tmp);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 5);
	gtk_widget_show(hbox);

#ifdef WITHOUT_GNOME
	tmp = gtk_button_new_with_label("OK");
#else
	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 15);
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
			   GTK_SIGNAL_FUNC(file_print_execute), data);
	gtk_widget_show(tmp);

#ifdef WITHOUT_GNOME
	tmp = gtk_button_new_with_label("Cancel");
#else
	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 15);
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
			   GTK_SIGNAL_FUNC(print_destroy), data->window);
	gtk_widget_show(tmp);

	gtk_widget_show(print_dialog);
}

