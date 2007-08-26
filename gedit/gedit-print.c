/*
 * gedit-print.c
 * This file is part of gedit
 *
 * Copyright (C) 2000-2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi  
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <libgnome/gnome-util.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>	/* For strlen */

#include "gedit-print.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-tab.h"

#define GEDIT_PRINT_CONFIG_FILE "gedit-print-config"

G_DEFINE_TYPE(GeditPrintJob, gedit_print_job, GTK_TYPE_SOURCE_PRINT_JOB)

static void 
gedit_print_job_class_init (GeditPrintJobClass *klass)
{
	/* Empty */
}

static GnomePrintConfig *
load_print_config_from_file ()
{
	gchar *file_name;
	gboolean res;
	gchar *contents;
	GnomePrintConfig *gedit_print_config;
	
	gedit_debug (DEBUG_PRINT);

	file_name = gnome_util_home_file (GEDIT_PRINT_CONFIG_FILE);

	res = g_file_get_contents (file_name, &contents, NULL, NULL);
	g_free (file_name);

	if (res)
	{
		gedit_print_config = gnome_print_config_from_string (contents, 0);
		g_free (contents);
	}
	else
		gedit_print_config = gnome_print_config_default ();

	return gedit_print_config;
}

static void
save_print_config_to_file (GnomePrintConfig *gedit_print_config)
{
	gint fd;
	gchar *str;
	size_t bytes;
	ssize_t written;
	gchar *file_name;
	gboolean res;

	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (gedit_print_config != NULL);

	str = gnome_print_config_to_string (gedit_print_config, 0);
	g_return_if_fail (str != NULL);
	
	file_name = gnome_util_home_file (GEDIT_PRINT_CONFIG_FILE);

	fd = open (file_name, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	g_free (file_name);

	if (fd == -1)
		goto save_error;
	
	bytes = strlen (str);

	/* Save the file content */
	written = write (fd, str, bytes);
	res = ((written != -1) && ((size_t)written == bytes));

	close (fd);

	if (!res)		
		goto save_error;
	
	g_free (str);
	
	return;
	
save_error:
	
	g_message ("gedit cannot save print config file.");
	
	g_free (str);
}

#define GEDIT_PRINT_CONFIG "gedit-print-config-key"

static void
buffer_set (GeditPrintJob *job, GParamSpec *pspec, gpointer d)
{
	GtkSourceBuffer *buffer;
	GtkSourcePrintJob *pjob;
	gpointer data;
	GnomePrintConfig *config;
	
	gedit_debug (DEBUG_PRINT);
	
	pjob = GTK_SOURCE_PRINT_JOB (job);
	
	buffer = gtk_source_print_job_get_buffer (pjob);
	
	data = g_object_get_data (G_OBJECT (buffer),
				  GEDIT_PRINT_CONFIG);

	if (data == NULL)	
	{			  
		config = load_print_config_from_file ();
		g_return_if_fail (config != NULL);
		
		g_object_set_data_full (G_OBJECT (buffer),
					GEDIT_PRINT_CONFIG,
					config,
					(GDestroyNotify)gnome_print_config_unref);
	}
	else
	{
		config = GNOME_PRINT_CONFIG (data);
	}

	gnome_print_config_set_int (config, (guchar *) GNOME_PRINT_KEY_NUM_COPIES, 1);
	gnome_print_config_set_boolean (config, (guchar *) GNOME_PRINT_KEY_COLLATE, FALSE);

	gtk_source_print_job_set_config (pjob, config);
	
	gtk_source_print_job_set_highlight (pjob, 
					    gtk_source_buffer_get_highlight_syntax (buffer) &&
					    gedit_prefs_manager_get_print_syntax_hl ());
		
	if (gedit_prefs_manager_get_print_header ())
	{
		gchar *doc_name;
		gchar *name_to_display;
		gchar *left;

		doc_name = gedit_document_get_uri_for_display (GEDIT_DOCUMENT (buffer));
		name_to_display = gedit_utils_str_middle_truncate (doc_name, 60);

		left = g_strdup_printf (_("File: %s"), name_to_display);

		/* Translators: %N is the current page number, %Q is the total
		 * number of pages (ex. Page 2 of 10) 
		 */
		gtk_source_print_job_set_header_format (pjob,
							left, 
							NULL, 
							_("Page %N of %Q"), 
							TRUE);

		gtk_source_print_job_set_print_header (pjob, TRUE);

		g_free (doc_name);
		g_free (name_to_display);
		g_free (left);
	}	
}

static void
gedit_print_job_init (GeditPrintJob *job)
{
	GtkSourcePrintJob *pjob;

	gchar *print_font_body;
	gchar *print_font_header;
	gchar *print_font_numbers;
	
	PangoFontDescription *print_font_body_desc;
	PangoFontDescription *print_font_header_desc;
	PangoFontDescription *print_font_numbers_desc;
	
	gedit_debug (DEBUG_PRINT);
	
	pjob = GTK_SOURCE_PRINT_JOB (job);
		
	gtk_source_print_job_set_print_numbers (pjob,
			gedit_prefs_manager_get_print_line_numbers ());

	gtk_source_print_job_set_wrap_mode (pjob,
			gedit_prefs_manager_get_print_wrap_mode ());

	gtk_source_print_job_set_tabs_width (pjob,
			gedit_prefs_manager_get_tabs_size ());
	
	gtk_source_print_job_set_print_header (pjob, FALSE);
	gtk_source_print_job_set_print_footer (pjob, FALSE);

	print_font_body = gedit_prefs_manager_get_print_font_body ();
	print_font_header = gedit_prefs_manager_get_print_font_header ();
	print_font_numbers = gedit_prefs_manager_get_print_font_numbers ();
	
	gtk_source_print_job_set_font (pjob, print_font_body);
	gtk_source_print_job_set_numbers_font (pjob, print_font_numbers);
	gtk_source_print_job_set_header_footer_font (pjob, print_font_header);

	print_font_body_desc = pango_font_description_from_string (print_font_body);
	print_font_header_desc = pango_font_description_from_string (print_font_header);
	print_font_numbers_desc = pango_font_description_from_string (print_font_numbers);

	gtk_source_print_job_set_font_desc (pjob, print_font_body_desc);
	gtk_source_print_job_set_numbers_font_desc (pjob, print_font_numbers_desc);
	gtk_source_print_job_set_header_footer_font_desc (pjob, print_font_header_desc);

	g_free (print_font_body);
	g_free (print_font_header);
	g_free (print_font_numbers);

	pango_font_description_free (print_font_body_desc);
	pango_font_description_free (print_font_header_desc);
	pango_font_description_free (print_font_numbers_desc);
	
	g_signal_connect (job,
			  "notify::buffer",
			  G_CALLBACK (buffer_set),
			  NULL);
}

GeditPrintJob *
gedit_print_job_new (GeditDocument *doc)
{
	GeditPrintJob *job;
	
	job = GEDIT_PRINT_JOB (g_object_new (GEDIT_TYPE_PRINT_JOB,
					     "buffer", doc,
					      NULL));

	return job;
}

void
gedit_print_job_save_config (GeditPrintJob *job)
{
	GnomePrintConfig *config;
	
	g_return_if_fail (GEDIT_IS_PRINT_JOB (job));
	
	config = gtk_source_print_job_get_config (GTK_SOURCE_PRINT_JOB (job));
	
	save_print_config_to_file (config);
}

GtkWidget *
gedit_print_dialog_new (GeditPrintJob *job)
{
	GtkWidget *dialog;
	gint selection_flag;
	gint lines;
	GnomePrintConfig *config;
	GtkSourceBuffer *buffer;
	GeditTab *tab;
	GeditTabState tab_state;

	gedit_debug (DEBUG_PRINT);

	g_return_val_if_fail (GEDIT_IS_PRINT_JOB (job), NULL);

	buffer = gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (job));
	g_return_val_if_fail (buffer != NULL, NULL);

	if (!gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (buffer), NULL, NULL))
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION;

	config = gtk_source_print_job_get_config (GTK_SOURCE_PRINT_JOB (job));

	dialog = g_object_new (GNOME_TYPE_PRINT_DIALOG, "print_config", config, NULL);

	gnome_print_dialog_construct (GNOME_PRINT_DIALOG (dialog), 
				      (guchar *) _("Print"),
			              GNOME_PRINT_DIALOG_RANGE | GNOME_PRINT_DIALOG_COPIES);

	lines = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer));

	gnome_print_dialog_construct_range_page (GNOME_PRINT_DIALOG (dialog),
						 GNOME_PRINT_RANGE_ALL |
						 GNOME_PRINT_RANGE_RANGE |
						 selection_flag,
						 1, 
						 lines,
						 (guchar *) "A",
						 (guchar *) _("Lines"));

	/* Disable the print preview button of the gnome print dialog if 
	 * the state of the active tab is print_previewing or 
	 * showing_print_preview 
	 */
	tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (buffer));
	tab_state = gedit_tab_get_state (tab);
	if ((tab_state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW) ||
	    (tab_state == GEDIT_TAB_STATE_PRINT_PREVIEWING))
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
						   GNOME_PRINT_DIALOG_RESPONSE_PREVIEW,
						   FALSE);
	}

	return dialog;
}
