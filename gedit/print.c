/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Cleaned by chema Feb, 24, 2001 */
/*
 * gedit
 *
 * Copyright (C) 2000 Jose Maria Celorio
 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 */ 

#include <config.h>
#include <gnome.h>

#include <libgnomevfs/gnome-vfs.h>

#include "print.h"
#include "print-doc.h"
#include "file.h"
#include "commands.h"
#include "prefs.h"
#include "window.h"

/**
 * gedit_print_calculate_pages:
 * @pji: 
 * 
 * Calculates the number of pages that we need to print
 **/
static void
gedit_print_calculate_pages (PrintJobInfo *pji)
{
	pji->total_lines      = gedit_print_document_determine_lines (pji, FALSE);
	pji->total_lines_real = gedit_print_document_determine_lines (pji, TRUE);
	pji->pages            = ((int) (pji->total_lines_real-1)/
				 pji->lines_per_page)+1;

	pji->page_first = 1;
	pji->page_last = pji->pages;
}
	

/**
 * gedit_print_job_info_new:
 * @void: 
 * 
 * Create and fill the PrintJobInfo struct
 * 
 * Return Value: 
 **/
static PrintJobInfo *
gedit_print_job_info_new (void)
{
	PrintJobInfo *pji;

	pji = g_new0 (PrintJobInfo, 1);

	/* Gedit doc & view */
	pji->view        = gedit_view_active ();
	pji->doc         = gedit_view_get_document (pji->view);

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (pji->doc), NULL);

	pji->preview     = FALSE;
	if (pji->doc->filename == NULL)
		pji->filename = gedit_document_get_tab_name (pji->doc, FALSE); 
	else
		pji->filename = gnome_vfs_unescape_string_for_display (pji->doc->filename);


	/* GnomePrint Master and Context */
	pji->master  = gnome_print_master_new ();
	pji->pc      = gnome_print_master_get_context (pji->master);
	pji->printer = NULL;

	g_return_val_if_fail (GNOME_IS_PRINT_MASTER (pji->master), NULL);
	g_return_val_if_fail (GNOME_IS_PRINT_CONTEXT (pji->pc), NULL);

	/* Buffer */
	pji->buffer      = gedit_document_get_buffer (pji->doc);
	g_return_val_if_fail (pji->buffer != NULL, NULL);
	pji->buffer_size = strlen (pji->buffer);

	/* Paper and Orientation */
	pji->paper       = gnome_paper_with_name (settings->papersize);
	gnome_print_master_set_paper (pji->master, pji->paper);
	g_return_val_if_fail (pji->paper != NULL, NULL);
	pji->orientation = settings->print_orientation;
	if (pji->orientation == PRINT_ORIENT_LANDSCAPE)
	{
		pji->page_width  = gnome_paper_psheight (pji->paper);
		pji->page_height = gnome_paper_pswidth (pji->paper);
	}
	else
	{
		pji->page_width  = gnome_paper_pswidth (pji->paper);
		pji->page_height = gnome_paper_psheight (pji->paper);
	}

	/* Margins and printable area */
	pji->margin_numbers = .25 * 72;
	pji->margin_top     = .75 * 72; /* Printer margins, not page margins */
	pji->margin_bottom  = .75 * 72; /* We should "pull" this from gnome-print when */
	pji->margin_left    = .75 * 72; /* gnome-print implements them */
	pji->margin_right   = .75 * 72;
	pji->header_height  = settings->print_header * 72;
	if (settings->print_lines > 0)
		pji->margin_left += pji->margin_numbers;
	pji->printable_width  = pji->page_width -
		                pji->margin_left -
		                pji->margin_right -
		                ((settings->print_lines>0)?pji->margin_numbers:0);
	pji->printable_height = pji->page_height -
		                pji->margin_top -
		                pji->margin_bottom;

	/* Fonts */
	pji->font_char_width  = 0.0808 * 72; /* Get this from the font itself !! */
	pji->font_char_height = .14 * 72;

	pji->font_body    = gnome_font_new (GEDIT_PRINT_FONT_BODY,
					    GEDIT_PRINT_FONT_SIZE_BODY);
	pji->font_header  = gnome_font_new (GEDIT_PRINT_FONT_HEADER,
					    GEDIT_PRINT_FONT_SIZE_HEADER);
	pji->font_numbers = gnome_font_new (GEDIT_PRINT_FONT_BODY,
					    GEDIT_PRINT_FONT_SIZE_NUMBERS);

	g_return_val_if_fail (GNOME_IS_FONT (pji->font_body),    NULL);
	g_return_val_if_fail (GNOME_IS_FONT (pji->font_header),  NULL);
	g_return_val_if_fail (GNOME_IS_FONT (pji->font_numbers), NULL);
		
	/* Chars, lines and pages */
	pji->wrapping         = settings->print_wrap_lines;
	pji->print_line_numbers = settings->print_lines;
	pji->print_header     = settings->print_header;
	pji->chars_per_line   = (gint)(pji->printable_width /
				       pji->font_char_width);
	pji->lines_per_page   = ((pji->printable_height - pji->header_height) /
				 pji->font_char_height -  1);
	pji->tab_size         = settings->tab_size;
	pji->range            = GNOME_PRINT_RANGE_ALL;
	
	gedit_print_calculate_pages (pji);

	pji->canceled = FALSE;
	
	return pji;
}


/**
 * gedit_print_job_info_destroy:
 * @pji: 
 * 
 * Destroy and free the internals of a PrintJobInfo stucture
 **/
static void
gedit_print_job_info_destroy (PrintJobInfo *pji)
{
	gnome_print_master_close (pji->master);

	g_free (pji->buffer);
	pji->buffer = NULL;

	gnome_font_unref (pji->font_body);
	gnome_font_unref (pji->font_header);
	gnome_font_unref (pji->font_numbers);
			  
	g_free (pji);
}


/**
 * gedit_print_range_is_selection:
 * @pji: 
 * 
 * The user has chosen range = selection. Modify the relevant
 * fields inside pji to print only the selected text
 **/
static void
gedit_print_range_is_selection (PrintJobInfo *pji, guint start, guint end)
{
	g_free (pji->buffer);
	pji->buffer = gedit_document_get_chars (pji->doc, start, end);
	g_return_if_fail (pji->buffer!=NULL);
	pji->buffer_size = end - start;
	gedit_print_calculate_pages (pji);
	
	pji->page_first = 1;
	pji->page_last = pji->pages;
}

/**
 * gedit_print_run_dialog:
 * @pji: 
 * 
 * Run the print dialog 
 * 
 * Return Value: TRUE if the printing was canceled by the user
 **/
static gboolean
gedit_print_run_dialog (PrintJobInfo *pji)
{
	GnomeDialog *dialog;
	guint start_pos;
	guint end_pos;
	gint selection_flag;

	if (!gedit_view_get_selection (pji->view, &start_pos, &end_pos))
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION;
		
	dialog = (GnomeDialog *) gnome_print_dialog_new (
		(const char *) _("Print Document"),
		GNOME_PRINT_DIALOG_RANGE);
	
	gnome_print_dialog_construct_range_page ( (GnomePrintDialog * ) dialog,
						  GNOME_PRINT_RANGE_ALL |
						  GNOME_PRINT_RANGE_RANGE |
						  selection_flag,
						  1, pji->pages, "A",
						  _("Pages"));
	
	switch (gnome_dialog_run (GNOME_DIALOG (dialog))) {
	case GNOME_PRINT_PRINT:
		break;
	case GNOME_PRINT_PREVIEW:
		pji->preview = TRUE;
		break;
	case -1:
		return TRUE;
	default:
		gnome_dialog_close (GNOME_DIALOG (dialog));
		return TRUE;
	}
	
	pji->printer = gnome_print_dialog_get_printer (GNOME_PRINT_DIALOG (dialog));
	/* If preview, do not set the printer so that the print button in the preview
	 * window will pop another print dialog */
	if (pji->printer && !pji->preview)
		gnome_print_master_set_printer (pji->master, pji->printer);
	
	pji->range = gnome_print_dialog_get_range_page (
		GNOME_PRINT_DIALOG (dialog),
		&pji->page_first,
		&pji->page_last);

	if (pji->range == GNOME_PRINT_RANGE_SELECTION)
		gedit_print_range_is_selection (pji, start_pos, end_pos);

	gnome_dialog_close (GNOME_DIALOG (dialog));

	return FALSE;
}
	


/**
 * gedit_print_preview_real:
 * @pji: 
 * 
 * Create and show the print preview window
 **/
static void
gedit_print_preview_real (PrintJobInfo *pji)
{
	GnomePrintMasterPreview *gpmp;
	gchar *title;
	
	title = g_strdup_printf (_("gedit: Print Preview\n"));
	gpmp = gnome_print_master_preview_new (pji->master, title);
	g_free (title);

	gtk_widget_show (GTK_WIDGET (gpmp));
}

/**
 * gedit_print:
 * @preview: 
 * 
 * The main printing function
 **/
static void
gedit_print (gboolean preview)
{
	PrintJobInfo *pji;
	gboolean cancel = FALSE;

	gedit_debug (DEBUG_PRINT, "");

	if (!gedit_print_verify_fonts ())
		return;

	pji = gedit_print_job_info_new ();
	pji->preview = preview;

	if (!pji->preview)
		cancel = gedit_print_run_dialog (pji);

	/* The canceled button on the dialog was clicked */
	if (cancel) {
		gedit_print_job_info_destroy (pji);
		return;
	}
		
	gedit_print_document (pji);

	/* The printing was canceled while in progress */
	if (pji->canceled) {
		gedit_print_job_info_destroy (pji);
		return;
	}
		
	if (pji->preview)
		gedit_print_preview_real (pji);
	else
		gnome_print_master_print (pji->master);
	
	gedit_print_job_info_destroy (pji);
}

/**
 * gedit_print_cb:
 * @widget: 
 * @notused: 
 * 
 * A callback for the menu/toolbar for printing
 **/
void
gedit_print_cb (GtkWidget *widget, gpointer notused)
{
	gedit_print (FALSE);
}

/**
 * gedit_print_preview_cb:
 * @widget: 
 * @notused: 
 * 
 * A callback for the menus/toolbars for print previewing
 **/
void
gedit_print_preview_cb (GtkWidget *widget, gpointer notused)
{
	gedit_print (TRUE);
}
