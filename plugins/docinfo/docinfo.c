/* gedit - Document info plugin
 * Copyright (C) 2001 Paolo Maggi
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
 * 
 * Authors: Paolo Maggi <maggi@athena.polito.it>
 *
 * FIXME: Add Support for mb chars
 */
#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include <ctype.h>

#include "window.h"
#include "document.h"
#include "utils.h"

#include "view.h"
#include "plugin.h"

#define GEDIT_PLUGIN_GLADE_FILE "/docinfo.glade"

GtkWidget *dialog = NULL;

GtkWidget *frame = NULL;
GtkWidget *words_label = NULL;
GtkWidget *paragraphs_label = NULL;
GtkWidget *chars_label = NULL;
GtkWidget *chars_ns_label = NULL;
GtkWidget *lines_label = NULL;
GtkWidget *cur_col_label = NULL;
GtkWidget *cur_line_label = NULL;

static void
destroy_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");
}

static void
gedit_plugin_finish (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	gnome_dialog_close (GNOME_DIALOG (widget));
	dialog = NULL;
}

static void
close_button_pressed (GtkWidget *widget, GtkWidget* data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	gnome_dialog_close (GNOME_DIALOG (data));
	dialog = NULL;
}

static void
help_button_pressed (GtkWidget *widget, gpointer data)
{
	/* FIXME: Paolo - change to point to the right help page */
	static GnomeHelpMenuEntry help_entry = { "gedit", "plugins.html" };
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	gnome_help_display (NULL, &help_entry);
}

static void
gedit_plugin_get_info (GeditView *view, gint *total_chars, gint *total_chars_ns, gint *total_words, 
		gint *total_lines, gint *total_paragraphs, gint *line_number, gint *column_number )
{
	gint i;

	gchar* buffer;
	gint length;

	gint pos;
	gint lines = 1 ;
	gint column = 0 ;

	gint newlines_number ; /* this variable is the number of '\n' that there are
				  between two words */

	gboolean in_word = FALSE;
	
	gedit_debug (DEBUG_PLUGINS, "");

	buffer = gedit_document_get_buffer (view->doc);
	length = strlen (buffer);

	pos = gedit_view_get_position (view);
		
	*total_chars      = 0;
	*total_chars_ns   = 0;
	*total_words      = 0;
	*total_lines      = 1;
	*total_paragraphs = 1;

	newlines_number = 0;	
	
	for(i = 0; i < length; ++i)
	{
		if (!isspace(buffer[i])) 
		{
			*total_chars_ns = 1 + *total_chars_ns;
						
			if( buffer[i] == ',' || buffer[i] == ';' ||
			    buffer[i] == ':' || buffer[i] == '.' ) 
			{
				if (in_word) 
				{
					in_word = FALSE;
				}
			} 
			else 
			{
				if (!in_word) 
				{
					in_word = TRUE;
					*total_words = 1 + *total_words;
				}
			}

			if (newlines_number > 1)
			{
				*total_paragraphs = 1 + *total_paragraphs;
				newlines_number = 0;			
			}
		} 
		else 
		{
			if (in_word) 
			{
				in_word = FALSE;
			}
		}

		if (buffer[i] == '\n') 
		{
			newlines_number = 0;

			while (buffer[i] == '\n')
			{
				*total_lines = 1 + *total_lines;
				++newlines_number;

				++i;
			}

			--i;
		}
	}

	*total_chars = length;
	
	if ( *total_words == 0 ) *total_paragraphs = 0 ;

	for (i = 0; i <= pos ; ++i) 
	{
		column++;
		if (i == pos)
		{
			*line_number = lines;
			*column_number = column ;
		}
		if (buffer[i]=='\n')
		{
			lines++;
			column = 0;
		}		
	}
	
	g_free (buffer);
	
	return ;		
}

static void
gedit_plugin_update_now (void)
{
	GeditView *view;

	gint total_chars;
	gint total_chars_ns;
	gint total_words;
	gint total_lines;
	gint total_paragraphs;
	gint line_number;
	gint column_number;

	gchar* doc_name;
	gchar* tmp_str;
	
	view = gedit_view_active ();

	if (view == NULL)
	{
		GtkWidget* error_dialog = gnome_message_box_new (
	               _("No document exists.\n"
		 	 "The Document Info plugin is going be closed."),
	                GNOME_MESSAGE_BOX_WARNING,
			GNOME_STOCK_BUTTON_OK,
                	NULL);

	        gnome_dialog_run (GNOME_DIALOG (error_dialog));

		gedit_plugin_finish (dialog, NULL);
		return;
	}

	gedit_plugin_get_info (view, &total_chars, &total_chars_ns, &total_words, 
		&total_lines, &total_paragraphs, &line_number, &column_number);

	doc_name = gedit_document_get_tab_name (view->doc, FALSE);
	gtk_frame_set_label (GTK_FRAME (frame), doc_name);
	g_free (doc_name);

	tmp_str = g_strdup_printf("%d", total_words);
	gtk_label_set_text (GTK_LABEL (words_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d",total_paragraphs);
	gtk_label_set_text (GTK_LABEL (paragraphs_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d",total_chars);
	gtk_label_set_text (GTK_LABEL (chars_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d",total_chars_ns);
	gtk_label_set_text (GTK_LABEL (chars_ns_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d",total_lines);
	gtk_label_set_text (GTK_LABEL (lines_label), tmp_str);
	g_free (tmp_str);
	
	tmp_str = g_strdup_printf("%d",line_number);
	gtk_label_set_text (GTK_LABEL (cur_line_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d",column_number);
	gtk_label_set_text (GTK_LABEL (cur_col_label), tmp_str);
	g_free (tmp_str);
}

static void
update_button_pressed (GtkWidget *widget, gpointer data)
{
	gedit_plugin_update_now ();
}

static void
gedit_plugin_create_dialog (void)
{
	GladeXML *gui = NULL;
	
	GtkWidget *close_button = NULL;
	GtkWidget *help_button = NULL;
	GtkWidget *update_button = NULL;

	GeditView *view;	
	view = gedit_view_active ();

	g_return_if_fail (view != NULL);

	if (dialog != NULL)
	{
		g_return_if_fail (GTK_WIDGET_REALIZED (dialog));
		g_return_if_fail (frame              != NULL);
		g_return_if_fail (words_label        != NULL);
		g_return_if_fail (paragraphs_label   != NULL);
		g_return_if_fail (lines_label        != NULL);
		g_return_if_fail (chars_label        != NULL);
		g_return_if_fail (chars_ns_label     != NULL);
		g_return_if_fail (cur_col_label      != NULL);
		g_return_if_fail (cur_line_label     != NULL);

		gdk_window_show (dialog->window);
		gdk_window_raise (dialog->window);
		
		gedit_plugin_update_now ();
		
		return;		
	}	
	
	gui = glade_xml_new (GEDIT_GLADEDIR
			     GEDIT_PLUGIN_GLADE_FILE,
			     "dialog");
	
	if (!gui) {
		g_warning ("Could not find %s, reinstall gedit.\n",
			   GEDIT_GLADEDIR
			   GEDIT_PLUGIN_GLADE_FILE);
		return;
	}

	dialog             = glade_xml_get_widget (gui, "dialog");
	close_button       = glade_xml_get_widget (gui, "close_button");
	help_button        = glade_xml_get_widget (gui, "help_button");
	update_button      = glade_xml_get_widget (gui, "update_button");
	frame              = glade_xml_get_widget (gui, "frame");
	words_label        = glade_xml_get_widget (gui, "words_label");
	paragraphs_label   = glade_xml_get_widget (gui, "paragraphs_label");
	lines_label        = glade_xml_get_widget (gui, "lines_label");
	chars_label        = glade_xml_get_widget (gui, "chars_label");
	chars_ns_label     = glade_xml_get_widget (gui, "chars_ns_label");
	cur_col_label      = glade_xml_get_widget (gui, "cur_col_label");
	cur_line_label     = glade_xml_get_widget (gui, "cur_line_label");

	g_return_if_fail (dialog             != NULL);
	g_return_if_fail (close_button       != NULL);
	g_return_if_fail (help_button        != NULL);
	g_return_if_fail (update_button        != NULL);
	g_return_if_fail (frame              != NULL);
	g_return_if_fail (words_label        != NULL);
	g_return_if_fail (paragraphs_label   != NULL);
	g_return_if_fail (lines_label        != NULL);
	g_return_if_fail (chars_label        != NULL);
	g_return_if_fail (chars_ns_label     != NULL);
	g_return_if_fail (cur_col_label      != NULL);
	g_return_if_fail (cur_line_label     != NULL);

	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
			    GTK_SIGNAL_FUNC (close_button_pressed), dialog);
	gtk_signal_connect (GTK_OBJECT (help_button), "clicked",
			    GTK_SIGNAL_FUNC (help_button_pressed), NULL);
	gtk_signal_connect (GTK_OBJECT (update_button), "clicked",
			    GTK_SIGNAL_FUNC (update_button_pressed), NULL);

	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_finish), NULL);

	gedit_plugin_update_now ();
	
	/* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent      (GNOME_DIALOG (dialog), gedit_window_active());
	gtk_window_set_modal         (GTK_WINDOW (dialog), FALSE);
	gnome_dialog_set_default     (GNOME_DIALOG (dialog), 0);

	/* Show everything then free the GladeXML memory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));	
}

gint
init_plugin (PluginData *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Document info");
	pd->desc = _("Get info on current document.");
	pd->long_desc = _("This plugin displays a pop-up dialog which contains basic information on current document.");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = FALSE;

	pd->private_data = (gpointer)gedit_plugin_create_dialog;
	
	pd->installed_by_default = TRUE;	

	return PLUGIN_OK;
}

