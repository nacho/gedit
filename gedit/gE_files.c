#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"

/* Strips away the leading path... Should maybe be optional? */
char *strip_filename (gchar *string)
{
	int i, j;
	char *new_string;
	
		if (string[0] != '/')
			{
				#ifdef DEBUG
				g_print("%s\n", string);
				#endif
				return string;
			}

	for (i=strlen(string)-1; i>=0; i--)
	{

		if (string[i] == '/')
			break;
	}
	if (i > 0)
		i++;
	new_string = g_malloc0((strlen(string)-i)+1);
	for (j=0; j<=(strlen(string)-i)-1; j++)
		new_string[j] = string[i+j];

	return new_string;
}


void clear_text (gE_document *document)
{
	int i;
	i = gtk_text_get_length (GTK_TEXT(document->text));
	if (i>0) {
		gtk_text_set_point (GTK_TEXT(document->text), i);
		gtk_text_backward_delete (GTK_TEXT(document->text), i);
	}
}

gint gE_file_open(gE_window *window, gE_document *document, gchar *filename)
{
	char str[10];
	FILE *file_handle;
	gchar *title;
	
	if ((file_handle = fopen(filename, "rt")) == NULL)
	{
		document->filename = filename;
		gtk_label_set (GTK_LABEL(document->tab_label), strip_filename(filename));
		gtk_text_set_point (GTK_TEXT(document->text), 0);
		document->changed = FALSE;
		if (!document->changed_id)
			document->changed_id = gtk_signal_connect (GTK_OBJECT(document->text), "changed", 
			                                           GTK_SIGNAL_FUNC(document_changed_callback), document);
	

		gtk_label_set (GTK_LABEL(window->statusbar), ("File Opened..."));
		
		return 1;
	}
	gtk_text_freeze (GTK_TEXT(document->text));
	clear_text(document);
	do {
		if (fgets(str, sizeof(str)+1, file_handle) == NULL)
			break;
		gtk_text_insert (GTK_TEXT(document->text), NULL, &document->text->style->black, NULL, str, strlen(str));

	} while(!feof(file_handle));
	fclose(file_handle);
	gtk_text_thaw (GTK_TEXT(document->text));
	document->filename = filename;
	gtk_label_set (GTK_LABEL(document->tab_label), strip_filename(filename));
	gtk_text_set_point (GTK_TEXT(document->text), 0);
	document->changed = FALSE;
	if (!document->changed_id)
		document->changed_id = gtk_signal_connect (GTK_OBJECT(document->text), "changed", 
		                                           GTK_SIGNAL_FUNC(document_changed_callback), document);

	title = g_malloc0 (strlen (GEDIT_ID) + strlen (GTK_LABEL(document->tab_label)->label) + 4);
	sprintf (title, "%s - %s", GEDIT_ID, GTK_LABEL (document->tab_label)->label);
	gtk_window_set_title (GTK_WINDOW (window->window), title);
	g_free (title);

	gtk_label_set (GTK_LABEL(window->statusbar), ("File Opened..."));
	return 0;
}

gint gE_file_save(gE_window *window, gE_document *document, gchar *filename)
{
	int i;
	FILE *file_handle;
	gchar *title;
	
	if ((file_handle = fopen(filename, "w")) == NULL)
	{
		g_warning ("Unable to save file %s.", filename);
		return 1;
	}
	gtk_text_thaw (GTK_TEXT(document->text)); /* don't know if this is really neccasary... */

	/* If someone wants to write a better loop here, I'm open to suggestions...
	   This looks inefficient, but I was told (I think - possible I misunderstood)
	   not to read in all the characters at once...
							-Evan */

	/* New, improved loop, brought to us by T Taneli Vahakangas - thanks for the fix! :-) */

       for (i=1; i<=gtk_text_get_length(GTK_TEXT(document->text)); i++)
               fputc((int) gtk_editable_get_chars(GTK_EDITABLE(document->text), i-1, i)[0], 
                      file_handle);

	fclose(file_handle);
	
	document->filename = filename;
	/*g_print("%s\n",filename);*/
	document->changed = FALSE;
	gtk_label_set(GTK_LABEL(document->tab_label), strip_filename(filename));
	if (!document->changed_id)
		document->changed_id = gtk_signal_connect (GTK_OBJECT(document->text), "changed", GTK_SIGNAL_FUNC(document_changed_callback), document);

	title = g_malloc0 (strlen (GEDIT_ID) + strlen (GTK_LABEL(document->tab_label)->label) + 4);
	sprintf (title, "%s - %s", GEDIT_ID, GTK_LABEL (document->tab_label)->label);
	gtk_window_set_title (GTK_WINDOW (window->window), title);
	g_free (title);
	
	gtk_label_set (GTK_LABEL(window->statusbar), ("File Saved..."));
	return 0;
}

