/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 */ 


#include <config.h>
#include <gnome.h>

#include "print.h"
#include "file.h"
#include "view.h"
#include "document.h"
#include "commands.h"
#include "prefs.h"
#include "utils.h"

#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer-dialog.h>

#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include <libgnomeprint/gnome-print-preview.h>

typedef struct _PrintJobInfo {
	/* gnome print stuff */
        GnomePrintMaster *master;
	GnomePrintContext *pc;
	const GnomePaper *paper;

	/* document stuff */
	Document *doc;
	guchar *buffer;
	View *view;
	guint buffer_size;
	guchar *filename;
	
	/* Font stuff */ 
	guchar *font_name;
	int   font_size;
	float font_char_width;
	float font_char_height;

	/* Page stuff */ 
	int    pages;
	float  page_width, page_height;
	float  margin_top, margin_bottom, margin_left, margin_right, margin_numbers;
	float  printable_width, printable_height;
	float  header_height;
	gint   total_lines, total_lines_real;
	gint   lines_per_page;
	gint   chars_per_line;
	guchar* temp;

	/* Range stuff */
	gint range;
	gint print_first;
	gint print_last;
	gint print_this_page;
	gint preview;
	
	/* buffer stuff */
	gint file_offset;
	gint current_line;

	/* Text Wrapping */
	gint wrapping;
	gint break_chars;
	gint add_marks; /* little mark on lines that are a continuation of the above */ 
} PrintJobInfo;


       void file_print_cb (GtkWidget *widget, gpointer data, gint file_printpreview);
       void file_print_preview_cb (GtkWidget *widget, gpointer data);
static void print_document (Document *doc, PrintJobInfo *pji, GnomePrinter *printer);
static void print_line (PrintJobInfo *pji, int line);
static void print_ps_line(PrintJobInfo * pji, int line);
static int  print_determine_lines (PrintJobInfo *pji, int real);
static void print_header (PrintJobInfo *pji, unsigned int page);
static void start_job (GnomePrintContext *pc);
static void print_header (PrintJobInfo *pji, unsigned int page);
static void print_setfont (PrintJobInfo *pji);
static void end_page (PrintJobInfo *pji);
static void end_job (GnomePrintContext *pc);
static void preview_destroy_cb (GtkObject *obj, PrintJobInfo *pji);
static void print_pji_destroy (PrintJobInfo *pji);
static void set_pji ( PrintJobInfo * pji, Document *doc);


/**
 * file_print_cb:
 * @widget: 
 * @data: 
 * @file_printpreview: FALSE if print_preview is needed.
 * 
 * Starts the printing process and creates a print dialog box.
 * This should be the only routine global to the world (and Print Preview_cb)
 *
 **/
void
file_print_cb (GtkWidget *widget, gpointer data, gint file_printpreview)
{
	GtkWidget *dialog;
	GnomePrinter *printer;
	PrintJobInfo *pji;
	Document * doc = gedit_document_current();

	gedit_debug_mess ("F:file_print_cb\n", DEBUG_PRINT);
	
	if ( doc == NULL)
		return;

	pji = g_new0 (PrintJobInfo, 1);
	pji->paper = gnome_paper_with_name (settings->papersize);
	g_return_if_fail (pji->paper != NULL);
	pji->master = gnome_print_master_new();
	gnome_print_master_set_paper( pji->master, pji->paper);
	pji->pc = gnome_print_master_get_context(pji->master);
	g_return_if_fail(pji->pc != NULL);
	
	/* We need to calculate the number of pages
	   before running the dialog */
	set_pji (pji, doc);

	/* file_print preview is a false when preview is requested */ 
	if (file_printpreview)
	{
		dialog = (GnomePrintDialog *)gnome_print_dialog_new ( (const char *)"Print Document", GNOME_PRINT_DIALOG_RANGE);
		gnome_print_dialog_construct_range_page ( (GnomePrintDialog * )dialog,
							  GNOME_PRINT_RANGE_ALL | GNOME_PRINT_RANGE_RANGE,
							  1, pji->pages, "A",
							  _("Pages")/* Translators: As in [Range] Pages from:[x]  to*/);

		switch (gnome_dialog_run (GNOME_DIALOG (dialog))) {
		case GNOME_PRINT_PRINT:
			break;
		case GNOME_PRINT_PREVIEW:
			pji->preview = TRUE;
			break;
		case -1:
			print_pji_destroy (pji);
			return;
		default:
			print_pji_destroy (pji);
			gnome_dialog_close (GNOME_DIALOG (dialog));
			return;
		}

		printer = gnome_print_dialog_get_printer (GNOME_PRINT_DIALOG (dialog));

		pji->print_first =0;
		pji->print_last =0;
		pji->range = gnome_print_dialog_get_range_page ( GNOME_PRINT_DIALOG (dialog), &pji->print_first, &pji->print_last);
		
		gnome_dialog_close (GNOME_DIALOG (dialog));
	}
	else
	{
		pji->print_first = 1;
		pji->print_last = pji->pages;
		pji->preview = TRUE;
		printer = NULL;
	}

	if (pji->preview)
	{
		GnomePrintMasterPreview *preview;
		gchar *title;

		print_document (doc, pji, NULL);
		title = g_strdup_printf (_("gedit (%s): Print Preview"), pji->filename);
		preview = gnome_print_master_preview_new (pji->master, title);
		g_free (title);
		gtk_signal_connect (GTK_OBJECT(preview), "destroy",
				    GTK_SIGNAL_FUNC(preview_destroy_cb), pji);
		gtk_widget_show(GTK_WIDGET(preview));
	}
	else
	{
		if (printer)
			gnome_print_master_set_printer(pji->master, printer);
		print_document (doc, pji, printer);
		gnome_print_master_print (pji->master);
	}
	
	print_pji_destroy (pji);
	
/*	FIXME : we need to set the parent of the dialog to be the active window,
	because of some window manager issues, tha clahey told me about*/
#if 0	
        gnome_dialog_set_parent (GNOME_DIALOG(dialog), GTK_WINDOW(mdi->active_window)); 
	gtk_widget_show_all (dialog);
#endif
}


/**
 * file_print_preview_cb:
 * @widget: 
 * @data: 
 * 
 * Callback for print previewing. We just call file_prin_cb
 * with the 3rd argument = FALSE;
 *
 **/
void
file_print_preview_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug_mess ("F:file_print_preview_cb\n", DEBUG_PRINT);

	if (!gnome_mdi_get_active_view (mdi))
		return;
	file_print_cb( NULL, NULL, FALSE);
}


/**
 * print_document:
 * @doc: the document to be printed, we need this for doc->filename
 * @pji: the PrintJobInfo struct
 * @printer: the printer to do the printing to, NULL for printpreview
 * 
 * prints *doc
 *
 **/
static void
print_document (Document *doc, PrintJobInfo *pji, GnomePrinter *printer)
{
	int current_page, current_line;
	
	gedit_debug_mess ("F:print_document\n", DEBUG_PRINT);
	
	current_line=0;
	pji->temp = g_malloc( pji->chars_per_line + 2);

	start_job (pji->pc);
	for (current_page = 1; current_page <= pji->pages; current_page++)
	{
		if (pji->range != GNOME_PRINT_RANGE_ALL)
			pji->print_this_page = (current_page>=pji->print_first &&
						current_page<=pji->print_last) ? TRUE:FALSE;
		else
			pji->print_this_page = TRUE;

		if (pji->print_this_page)
		{
			/* We need to call gnome_print_beginpage so that it adds
			   "%% Page x" comment needed for viewing postcript files (i.e. in gv)*/
			gchar * pagenumbertext;
			pagenumbertext = g_strdup_printf ("%d", current_page);
			gnome_print_beginpage (pji->pc, pagenumbertext);
			g_free (pagenumbertext);

			/* Print the header of the page */
			if (settings->printheader)
				print_header(pji, current_page);
			print_setfont (pji);
		}
		
		/* we do this when the first line in the page is a continuation
		   of the last line in the previous page. This will prevent that
		   the line number in the previous page is repeated in the next*/
		if (pji->buffer [pji->file_offset-1] != '\n' && current_page>1 && pji->wrapping)
			current_line--;
		
		for ( current_line++; current_line <= pji->total_lines; current_line++)
		{
			print_line (pji, current_line);
			if (pji->current_line % pji->lines_per_page == 0)
				break;
		}
		
		if (pji->print_this_page)
			end_page (pji);
	}
	end_job (pji->pc);
	g_free (pji->temp);
		
	gnome_print_context_close (pji->pc);
	gnome_print_master_close (pji->master);

}

static void
print_line (PrintJobInfo *pji, int line)
{
	int i;
	int temp_i; /* We need to store i before scanning for a word wrap. If we cant wrap a word
		      ( the word size > chars_per_row ) we break that line and resotore the old i value*/
	int print_line = TRUE;
	int first_line = TRUE;

	i = 0;

	/* blank line ...*/
	if (pji->buffer [pji->file_offset] == '\n' )
	{
		pji->temp[0]=(guchar) '\0';
		print_ps_line (pji, line);
		pji->file_offset++;
		return;
	}
		
	while( (pji->buffer[ pji->file_offset + i] != '\n' && (pji->file_offset+i) < pji->buffer_size))
	{
		pji->temp[i]=pji->buffer[ pji->file_offset + i];
		i++;
		
		if( i == pji->chars_per_line + 1 )
		{
			/* We need to back i so that the last word does
			   not get broken in 2 */
			if (pji->wrapping)
			{
				temp_i=i;
				i--;
				while (pji->buffer [pji->file_offset+i] != ' ' && pji->buffer [pji->file_offset+i] != '\t' && i!= 0)
					i--;
				/* if the "word" is longer than a row then we break the "word" at the line width */
				i++;
				if (i==1)
					i=temp_i;
			}
			pji->temp[i]=(guchar) '\0';
			pji->file_offset = pji->file_offset + i;
			if (print_line)
				print_ps_line (pji, (first_line)?line:0);
			if (!pji->wrapping)
				print_line = FALSE;
			first_line=FALSE;
			if (pji->current_line % pji->lines_per_page == 0)
			{
				if (!pji->wrapping)
				{
					i=0;
					/* If we are clipping lines, we need to
					   advance pji->file_offset till the next \n */
					while( (pji->buffer[ pji->file_offset + i] != '\n'
						&& (pji->file_offset+i) < pji->buffer_size))
						i++;
					pji->file_offset = pji->file_offset + i + 1;
					return;
				}
				else
				{
					/* This is a special case in which the last line
					   of a page has the same lenght as chars_per_line, so
					   we need to advance file_offset to not print a blank
					   line in the next page */
					if ( i == pji->chars_per_line + 1)
						pji->file_offset++;
					return;
				}
			}
			i=0;
		}
	}

	pji->temp[i]=(guchar) '\0';
	pji->file_offset = pji->file_offset + i + 1;
	if (print_line && i > 0)
		print_ps_line (pji, (first_line)?line:0);
}

static void
print_ps_line (PrintJobInfo * pji, int line)
{
	float y = pji->page_height -  pji->margin_top - pji->header_height -
	(pji->font_char_height*( (pji->current_line++ % pji->lines_per_page)+1 ));

	if (!pji->print_this_page)
		return;

	gnome_print_moveto (pji->pc, pji->margin_left, y);
	gnome_print_show (pji->pc, pji->temp);
	if ( pji->temp!='\0')
		gnome_print_stroke (pji->pc);

	if (line>0 && settings->printlines>0 && line%settings->printlines==0)
	{
		char * number_text = g_strdup_printf ("%i",line);
		gnome_print_setfont (pji->pc, gnome_font_new (pji->font_name, 6));
		gnome_print_moveto (pji->pc, pji->margin_left - pji->margin_numbers, y);
		gnome_print_show   (pji->pc, number_text);
		g_free (number_text);
		print_setfont (pji);
	}
}

static void
set_pji (PrintJobInfo * pji, Document *doc)
{
	pji->view = VIEW(mdi->active_view);
	pji->doc = doc;
	pji->buffer_size = gtk_text_get_length(GTK_TEXT(pji->view->text));
	pji->buffer = gtk_editable_get_chars(GTK_EDITABLE(pji->view->text),0,pji->buffer_size);

	if (doc->filename == NULL)
		pji->filename = g_strdup (_("Untitled")); 
	else
		pji->filename = g_strdup (doc->filename);

	pji->page_width  = gnome_paper_pswidth (pji->paper);
	pji->page_height = gnome_paper_psheight (pji->paper);

	pji->margin_numbers = .25 * 72;
	pji->margin_top = .75 * 72;       /* Printer margins, not page margins */
	pji->margin_bottom = .75 * 72;    /* We should "pull" this from gnome-print when */
	pji->margin_left = .75 * 72;      /* gnome-print implements them */
	if (settings->printlines > 0)
		pji->margin_left += pji->margin_numbers;
	pji->margin_right = .75 * 72;
	pji->header_height = settings->printheader * 72;
	pji->printable_width  = pji->page_width -
		                pji->margin_left -
		                pji->margin_right -
		                ((settings->printlines>0)?pji->margin_numbers:0);
	pji->printable_height = pji->page_height -
		                pji->margin_top -
		                pji->margin_bottom;
	pji->font_name = "Courier"; /* FIXME: Use courier 10 for now but set to actual font */
	pji->font_size = 10;
	pji->font_char_width = 0.0808 * 72;
	pji->font_char_height = .14 * 72;
	pji->wrapping = settings->printwrap;
	pji->chars_per_line = (gint)(pji->printable_width / pji->font_char_width);
	pji->total_lines = print_determine_lines(pji, FALSE);
	pji->total_lines_real = print_determine_lines(pji, TRUE);
	pji->lines_per_page = (pji->printable_height -
			      pji->header_height)/pji->font_char_height
		              - 1 ;
	pji->pages = ((int) (pji->total_lines_real-1)/pji->lines_per_page)+1;

	pji->file_offset = 0;
	pji->current_line = 0;
}

/**
 * print_determine_lines: 
 * @pji: PrintJobInfo struct
 * @real: this flag determines if we count rows of text or lines
 * of rows splitted by wrapping.
 *
 * Determine the lines in the document so that we can calculate the pages
 * needed to print it. We need this in order for us to do page/pages
 *
 * Return Value: number of lines in the document
 **/
static int
print_determine_lines (PrintJobInfo *pji, int real)
{
	int lines=0;
	int i, temp_i;
	int character = 0;

	for (i=0; i <= pji->buffer_size; i++)
	{
		if (pji->buffer[i] == '\n' || i==pji->buffer_size)
		{
			if ((character>0 || !real || !pji->wrapping)||(character==0&&pji->wrapping))
				lines++;
			character=0;
		}
		else
		{
			if( pji->wrapping && real)
			{
				character++;
				if (character == pji->chars_per_line + 2)
				{
					/* We do word wrapping here */ 
					if (pji->buffer[i+1] != ' ' || pji->buffer[i+1] != '\t') 
					{
						temp_i = i;
						i--;
						while (pji->buffer [i] != ' ' && pji->buffer [i] != '\t' && i!= temp_i-pji->chars_per_line+2)
						{
							i--;
						}
						/* if the "word" is longer than a row then we break the word at the line width */
						if (i == temp_i-pji->chars_per_line+2)
							i = temp_i;
						i++;
					}
					
					lines++;
					character=1;
				}
			}
		}
	}

        /* After counting, scan the doc backwards to determine how many
	   blanks lines there are (at the bottom),substract that from lines */
	for ( i=pji->buffer_size-1; i>0; i--)
		if (pji->buffer[i] != '\n' && pji->buffer[i] != ' ' )
			break;
		else
			if (pji->buffer[i] == '\n')
				lines--;

	return lines;
}

static void
start_job (GnomePrintContext *pc)
{

}

static void
print_header (PrintJobInfo *pji, unsigned int page)
{
	guchar* text1 = g_strdup (pji->filename);
	guchar* text2 = g_strdup_printf (_("Page: %i/%i"),page,pji->pages);
	GnomeFont *font;
	float x,y,len;
	
	font = gnome_font_new ("Helvetica", 12);
	gnome_print_setfont (pji->pc, font);

	/* Print the file name */
	y = pji->page_height - pji->margin_top - pji->header_height/2;
	len = gnome_font_get_width_string (font, text1);
	x = pji->page_width/2 - len/2;
	gnome_print_moveto(pji->pc, x, y);
	gnome_print_show(pji->pc, text1);
	gnome_print_stroke(pji->pc);

	/* Print the page/pages  */
	y = pji->page_height - pji->margin_top - pji->header_height/4;
	len = gnome_font_get_width_string (font, text2);
	x = pji->page_width - len - 36;
	gnome_print_moveto (pji->pc, x, y);
	gnome_print_show (pji->pc, text2);
	gnome_print_stroke (pji->pc); 

}

static void
print_setfont (PrintJobInfo *pji)
{
	GnomeFont *font;
	/* Set the font for the rest of the page */
	font = gnome_font_new (pji->font_name, pji->font_size);
	gnome_print_setfont (pji->pc, font);
}
	

static void
end_page (PrintJobInfo *pji)
{
	gnome_print_showpage (pji->pc);
}

static void
end_job (GnomePrintContext *pc)
{
	
}

static void
print_pji_destroy (PrintJobInfo *pji)
{
	gtk_object_unref (GTK_OBJECT (pji->master));
	g_free (pji->buffer);
	g_free (pji->filename);
	g_free (pji);
}

static void
preview_destroy_cb (GtkObject *obj, PrintJobInfo *pji)
{
	gedit_debug_mess ("F:preview_destroy_cb\n", DEBUG_PRINT);
}






