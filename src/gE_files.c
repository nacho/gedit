#include <config.h>
#include <gnome.h>
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

gint gE_file_open(gE_document *document, gchar *filename)
{
	char str[10];
	FILE *file_handle;

	if ((file_handle = fopen(filename, "rt")) == NULL)
	{
		document->filename = filename;
		gtk_label_set (GTK_LABEL(document->tab_label), strip_filename(filename));
		gtk_text_set_point (GTK_TEXT(document->text), 0);
		document->changed = FALSE;
		if (!document->changed_id)
			document->changed_id = gtk_signal_connect (GTK_OBJECT(document->text), "changed", 
			                                           GTK_SIGNAL_FUNC(document_changed_callback), document);
	

		gtk_statusbar_push (GTK_STATUSBAR(main_window->statusbar), 1, _("File Opened..."));
		
		return 1;
	}
	gtk_text_freeze (GTK_TEXT(document->text));
	clear_text(document);
	do {
		if (fgets(str, sizeof(str)+1, file_handle) == NULL)
			break;
		else if ( ((int) str[0] > 31 && (int) str[0] < 126) || ((int) str[0] > 7 && (int)str[0] <14) || ((int)str[0] == 0))
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
	

	gtk_statusbar_push (GTK_STATUSBAR(main_window->statusbar), 1, _("File Opened..."));
	return 0;
}

gint gE_file_save(gE_document *document, gchar *filename)
{
	int i;
	FILE *file_handle;
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

	/* Old loop that apparently crashes when you try and save an empty file.. */

/*	for (i=0; i<=(gtk_text_get_length(GTK_TEXT(document->text))-1); i++)
		fputc((int) gtk_editable_get_chars(GTK_EDITABLE(document->text), i, i+1)[0], 
		      file_handle);
*/

	/* New, improved loop, brought to us by T Taneli Vahakangas - thanks for the fix! :-) */

       for (i=1; i<=gtk_text_get_length(GTK_TEXT(document->text)); i++)
               fputc((int) gtk_editable_get_chars(GTK_EDITABLE(document->text), i-1, i)[0], 
                      file_handle);

	fclose(file_handle);
	gtk_label_set(GTK_LABEL(document->tab_label), strip_filename(filename));
	
	/* This shoud be right, it was left out!
				- Alex */
	document->filename = filename;
	document->changed = FALSE;
	if (!document->changed_id)
		document->changed_id = gtk_signal_connect (GTK_OBJECT(document->text), "changed", GTK_SIGNAL_FUNC(document_changed_callback), document);

	gtk_statusbar_push (GTK_STATUSBAR(main_window->statusbar), 1, _("File Saved..."));
	return 0;
}

