/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
 *
 * Printing code by : Chema Celorio <chema@celorio.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <time.h>
#include "main.h"
#include "gedit-print.h"
#include "gedit-file-io.h"
#include "gE_window.h"
#include "gE_view.h"
#include "gE_mdi.h"
#include "commands.h"
#include "gE_prefs.h"


#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print-preview.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include <libgnomeprint/gnome-printer-dialog.h>

typedef struct {
	/* gnome print stuff */
        GnomePrintMaster *master;
	GnomePrintContext *pc;
	const GnomePaper *paper;

	/* document stuff */
	gedit_document *doc;
	guchar *buffer;
	gedit_view *view;
	guint buffer_size;
	guchar *filename;
	
	/* Font stuff */ 
	guchar *font_name;
	int   font_size;
	float font_char_width;
	float font_char_height;

	/* Page stuff */ 
	int   pages;
	float page_width, page_height;
	float margin_top, margin_bottom, margin_left, margin_right;
	float printable_width, printable_height;
	float header_height;
	int   total_lines;
	int   lines_per_page;

	/* Variables */
	int   file_offset;
	int   current_line; 
} gedit_PrintJobInfo;


       void file_print_cb (GtkWidget *widget, gpointer cbdata);
       void file_print_preview_cb (GtkWidget *widget, gpointer data);
static void print_document (gedit_document *doc, GnomePrinter *printer);
static void print_dialog_clicked_cb (GtkWidget *widget, gint button, gpointer data);
static void print_line (gedit_PrintJobInfo *pji);
static int  print_determine_lines (gedit_PrintJobInfo *pji);
static void print_header (gedit_PrintJobInfo *pji, unsigned int page);
static void start_job (GnomePrintContext *pc);
static void print_header (gedit_PrintJobInfo *pji, unsigned int page);
static void end_page (GnomePrintContext *pc);
static void end_job (GnomePrintContext *pc);
static void preview_destroy_cb (GtkObject *obj, gedit_PrintJobInfo *pji);
static void set_pji ( gedit_PrintJobInfo * pji, gedit_document *doc, GnomePrinter *printer);

/*
 * PUBLIC: file_print_cb
 *
 * calls gnome-print to create a print dialog box.
 * This should be the only routine global to
 * the world. ( and print preview )
 */
void
file_print_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_document *doc;
	GtkWidget   *dialog;
	
	doc = gedit_document_current();
	dialog = gnome_printer_dialog_new ();

	gnome_dialog_set_parent(GNOME_DIALOG(dialog),
				GTK_WINDOW(mdi->active_window));
	gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
			   (GtkSignalFunc)print_dialog_clicked_cb, doc);
	gtk_widget_show_all(dialog);
}

void
file_print_preview_cb (GtkWidget *widget, gpointer data)
{
	if(!gnome_mdi_get_active_view(mdi))
		return;
	print_document(gedit_document_current(), NULL);
}


static void
print_dialog_clicked_cb (GtkWidget *widget, gint button, gpointer data)
{
	if(button == 0) {
		GnomePrinter *printer;
		GnomePrinterDialog *dialog = GNOME_PRINTER_DIALOG(widget);
		gedit_document *doc = (gedit_document *)data;

		printer = gnome_printer_dialog_get_printer(dialog);

		if(printer){
			print_document(doc, printer);
		}
	}
	gnome_dialog_close(GNOME_DIALOG(widget));
}

static void
print_document (gedit_document *doc, GnomePrinter *printer)
{
	gedit_PrintJobInfo *pji;
	int i;

	pji = g_new0( gedit_PrintJobInfo, 1);
	set_pji( pji, doc, printer);

	start_job(pji->pc);
	for(i = 1; i <= pji->pages; i++)
	{
		print_header(pji, i);
		while( pji->file_offset < pji->buffer_size )
		{
			print_line(pji);
			if(pji->current_line % pji->lines_per_page == 0)
				break;
		}
		end_page(pji->pc);
	}
	end_job(pji->pc);

	gnome_print_context_close(pji->pc);
	gnome_print_master_close(pji->master);

	if(printer)
	{
		gnome_print_master_print(pji->master);
  		gtk_object_unref(GTK_OBJECT(pji->master));
		g_free(pji);
	}
	else
	{
		GnomePrintMasterPreview *preview;
		gchar *title;

		title = g_strdup_printf(_("gEdit (%s): Print Preview"), pji->filename);
		preview = gnome_print_master_preview_new(pji->master, title);
		g_free(title);
		gtk_signal_connect(GTK_OBJECT(preview), "destroy",
				   GTK_SIGNAL_FUNC(preview_destroy_cb), pji);
		gtk_widget_show(GTK_WIDGET(preview));
	}
	g_free( pji->buffer);
	g_free( pji->filename);
}

static void
print_line (gedit_PrintJobInfo *pji)
{
	float x, y;
	char * temp = g_malloc(265);
	int i;

	i = 0;
	while( pji->buffer[ pji->file_offset + i] != '\n' && (pji->file_offset+i < pji->buffer_size) )
	{
		temp[i]=pji->buffer[ pji->file_offset + i];
		i++;
		if( i > 254)
			break;
	}
	temp[i]=(guchar) '\0';	
	pji->file_offset = pji->file_offset + i + 1;

	y = pji->page_height -
	    pji->margin_top -
 	    pji->header_height -
	    (pji->font_char_height*( (pji->current_line++ % pji->lines_per_page)+1 ));
	x = pji->margin_left;
	gnome_print_moveto(pji->pc, x , y);
	gnome_print_show(pji->pc, temp);
	gnome_print_stroke(pji->pc);
	g_free(temp);
}

static void
set_pji (gedit_PrintJobInfo * pji, gedit_document *doc, GnomePrinter *printer)
{
	pji->master = gnome_print_master_new();
	pji->paper = gnome_paper_with_name( gnome_paper_name_default());
	gnome_print_master_set_paper( pji->master, pji->paper);
	if(printer)
		gnome_print_master_set_printer(pji->master, printer);
	pji->pc = gnome_print_master_get_context(pji->master);
	g_return_if_fail(pji->pc != NULL);
	pji->view = GE_VIEW(mdi->active_view);
	pji->doc = doc;
	pji->buffer_size = gtk_text_get_length(GTK_TEXT(pji->view->text));
	pji->buffer = gtk_editable_get_chars(GTK_EDITABLE(pji->view->text),0,pji->buffer_size);
	if( doc->filename == NULL )
		pji->filename = g_strdup("Untitled"); /* I don't know if this is 
							appropiate for translations */
	else
		pji->filename = g_strdup(doc->filename);
	pji->page_width  = gnome_paper_pswidth( pji->paper);
	pji->page_height = gnome_paper_psheight( pji->paper);
	pji->margin_top = .75 * 72;       /* Printer margins, not page margins */
	pji->margin_bottom = .75 * 72;    /* We should "pull" this from gnome-print when */
	pji->margin_left = .75 * 72;      /* gnome-print implements them */ 
	pji->margin_right = .75 * 72;
	pji->header_height = 1 * 72;
	pji->printable_width  = pji->page_width -
		                pji->margin_left -
		                pji->margin_right;
	pji->printable_height = pji->page_height -
		                pji->margin_top -
		                pji->margin_bottom;
	pji->font_name = "Courier"; /* FIXME: Use courier 10 for now but set to actual font */
	pji->font_size = 10;
	pji->font_char_width = 0.0808 * 72;
	pji->font_char_height = .14 * 72;
	pji->total_lines = print_determine_lines(pji);
	pji->lines_per_page = (pji->printable_height -
			      pji->header_height)/pji->font_char_height
		              - 1 ;
	pji->pages = ((int) (pji->total_lines-1)/pji->lines_per_page)+1;
	pji->file_offset = 0;
	pji->current_line = 0;
}

static int
print_determine_lines (gedit_PrintJobInfo *pji)
{
	/* For now count the number of /n 's*/
	int lines=1;
	int i;

	for(i=0;i<pji->buffer_size;i++)
		if( pji->buffer[i] == '\n')
			lines++;
	return lines;
}

static void
start_job (GnomePrintContext *pc)
{
}

static void
print_header (gedit_PrintJobInfo *pji, unsigned int page)
{
	guchar* text1 = g_strdup(pji->filename);
	guchar* text2 = g_strdup_printf(_("Page: %i/%i"),page,pji->pages);
	GnomeFont *font;
	float x,y,len;
	
	font = gnome_font_new("Helvetica", 12);
	gnome_print_setfont(pji->pc, font);

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
	gnome_print_moveto(pji->pc, x, y);
	gnome_print_show(pji->pc, text2);
	gnome_print_stroke(pji->pc); 
	/* Set the font for the rest of the page */
	font = gnome_font_new (pji->font_name, pji->font_size);
	gnome_print_setfont (pji->pc, font);
}

static void
end_page (GnomePrintContext *pc)
{
	gnome_print_showpage (pc);
}

static void
end_job (GnomePrintContext *pc)
{
}

static void
preview_destroy_cb (GtkObject *obj, gedit_PrintJobInfo *pji)
{
	gtk_object_unref(GTK_OBJECT(pji->master));
	g_free (pji);
}
