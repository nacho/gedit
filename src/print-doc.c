/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit. Code to print a gedit document. This code needs rewriting, it is
 *        a mess right now, but it works and has been stabilized
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

#include <libgnomeprint/gnome-print.h>

#include "print.h"
#include "print-doc.h"
#include "print-util.h"


static  void print_line (PrintJobInfo *pji, int line);
static  void print_ps_line(PrintJobInfo * pji, gint line, gint first_line);
static  void print_header (PrintJobInfo *pji, unsigned int page);
static  void print_start_job (PrintJobInfo *pji);
static  void print_set_orientation (PrintJobInfo *pji);
static  void print_header (PrintJobInfo *pji, unsigned int page);
static  void print_end_page (PrintJobInfo *pji);
static  void print_end_job (GnomePrintContext *pc);

/**
 * print_document:
 * @doc: the document to be printed, we need this for doc->filename
 * @pji: the PrintJobInfo struct
 * @printer: the printer to do the printing to, NULL for printpreview
 * 
 * prints *doc
 *
 **/
void
gedit_print_document (PrintJobInfo *pji)
{
	int current_page, current_line;
	
	gedit_debug (DEBUG_PRINT, "");
	
	pji->temp = g_malloc( pji->chars_per_line + 2);

	current_line = 0;

	gedit_print_progress_start (pji);
	
	print_start_job (pji);
	
	for (current_page = 1; current_page <= pji->pages; current_page++)
	{
		if (pji->range != GNOME_PRINT_RANGE_ALL)
			pji->print_this_page = (current_page>=pji->page_first &&
						current_page<=pji->page_last) ? TRUE:FALSE;
		else
			pji->print_this_page = TRUE;

		/* FIXME : This pji->print_this_page is not top hacking. I need
		   to advance the pointer to buffer to the rigth place not to
		   semi-print the non-printable pages. Chema */
		if (pji->print_this_page)
		{
			/* We need to call gnome_print_beginpage so that it adds
			   "%% Page x" comment needed for viewing postcript files (i.e. in gv)*/
			gchar * pagenumbertext;
			pagenumbertext = g_strdup_printf ("%d", current_page);
			gnome_print_beginpage (pji->pc, pagenumbertext);
			g_free (pagenumbertext);

			/* Print the header of the page */
			if (pji->print_header)
				print_header(pji, current_page);
			gnome_print_setfont (pji->pc, pji->font_body);
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
			print_end_page (pji);

		gedit_print_progress_tick (pji, current_page);
		if (pji->canceled)
			break;
	}
	print_end_job (pji->pc);
	g_free (pji->temp);

	gnome_print_context_close (pji->pc);

	gedit_print_progress_end (pji);
}


/**
 * print_line:
 * @pji: 
 * @line: 
 * 
 * 
 **/
static void
print_line (PrintJobInfo *pji, int line)
{
	gint dump_info = FALSE;
	gint chars_in_this_line = 0;
	gint i, temp;
	gint first_line = TRUE;

	while ( pji->buffer [pji->file_offset ] != '\n' && pji->file_offset < pji->buffer_size)
	{
		chars_in_this_line++;

		/* Take care of tabs */
		if (pji->buffer [pji->file_offset] != '\t') {
			/* Copy one character */
			pji->temp  [chars_in_this_line-1] = pji->buffer [pji->file_offset];
		} else {

			temp = chars_in_this_line;

			/* chars in this line is added tab size, but if the spaces are going to
			 * end up in the next line, we don't insert them. This means tabs are not
			 * carried over to the next line */			   
			chars_in_this_line += pji->tab_size - ( (chars_in_this_line-1) % pji->tab_size) - 1;

			if (chars_in_this_line > pji->chars_per_line + 1)
			    chars_in_this_line = pji->chars_per_line + 1;

			for (i=temp; i<chars_in_this_line+1; i++)
			{
				pji->temp [i-1] = ' ';
			}
		}


		/* Is this line "full" ? If not, continue */
		if (chars_in_this_line < pji->chars_per_line + 1) {
			/* next char please */
			pji->file_offset++;
			continue;
		}

		/* if we are not doing word wrapping, this is easy */
		if (!pji->wrapping)
		{
			/* We need to advance pji->file_offset until the next NL char */
			while( pji->buffer [pji->file_offset ] != '\n')
				pji->file_offset++;
			pji->file_offset--;

			pji->temp [chars_in_this_line] = (guchar) '\0';
			print_ps_line (pji, line, TRUE);
			pji->current_line++;
			chars_in_this_line = 0;

			/* Ok, next char please */
			pji->file_offset++;
			
			continue;
		}
		
		if (dump_info)
			g_print ("\nThis lines needs breaking\n");
			
		temp = pji->file_offset; /* We need to save the value of file_offset in case we have to break the line */
			
		/* We back up till we find a space that we can break the line at */
		while (pji->buffer [pji->file_offset] != ' '  &&
		       pji->buffer [pji->file_offset] != '\t' &&
		       pji->file_offset > temp - pji->chars_per_line - 1 )
		{
			pji->file_offset--;
		}
		
		if (dump_info)
			g_print("file offset got backed up [%i] times\n", temp - pji->file_offset);
		
		/* If this line was "unbreakable" beacuse it contained a word longer than
		 * chars per line, we need to break it at chars_per_line */
		if (pji->file_offset == temp - pji->chars_per_line - 1)
		{
			pji->file_offset = temp;
			if (dump_info)
				g_print ("We are breaking the line\n");
		}
		
		if (dump_info)
		{
			g_print ("Breaking temp at : %i\n", chars_in_this_line + pji->file_offset - temp - 1);
			g_print ("Chars_in_this_line %i File Offset %i Temp %i\n",
				 chars_in_this_line,
				 pji->file_offset,
				 temp);
		}
		pji->temp [ chars_in_this_line + pji->file_offset - temp - 1] = (guchar) '\0';
		print_ps_line (pji, line, first_line);
		first_line = FALSE;
		pji->current_line++;
		chars_in_this_line = 0;
		
		/* We need to remove the trailing blanks so that the next line does not start with
		 *  a space or a tab char
		 */
		while (pji->buffer [pji->file_offset] == ' ' || pji->buffer [pji->file_offset] == '\t')
			pji->file_offset++;
		
		/* If this is the last line of the page return */
		if (pji->current_line%pji->lines_per_page == 0)
			return;
	}

	/* We need to terminate the string and send it to gnome-print */ 
	pji->temp [chars_in_this_line] = (guchar) '\0';
	print_ps_line (pji, line, first_line);
	pji->current_line++;

	/* We need to skip the newline char for the new line character */
	pji->file_offset++;

}

/**
 * print_ps_line:
 * @pji: 
 * @line: 
 * @first_line: 
 * 
 * print line leaves the chars to be printed in pji->temp.
 * this function performs the actual printing of that line.
 **/
static void
print_ps_line (PrintJobInfo * pji, gint line, gint first_line)
{
	gfloat y;

	gedit_debug (DEBUG_PRINT, "");

	/* Calculate the y position */
	y = pji->page_height -  pji->margin_top - pji->header_height -
		(pji->font_char_height*( (pji->current_line % pji->lines_per_page)+1 ));

	if (!pji->print_this_page)
		return;
	
	gnome_print_moveto (pji->pc, pji->margin_left, y);
	gedit_print_show_iso8859_1 (pji->pc, pji->temp);

	/* Print the line number */
	if (pji->print_line_numbers >0 &&
	    line % pji->print_line_numbers == 0 &&
	    first_line)
	{
		char * number_text = g_strdup_printf ("%i",line);

		gnome_print_setfont (pji->pc, pji->font_numbers);
		gnome_print_moveto (pji->pc, pji->margin_left - pji->margin_numbers, y);
		gedit_print_show_iso8859_1 (pji->pc, number_text);
		g_free (number_text);
		gnome_print_setfont (pji->pc, pji->font_body);
	}
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
 *
 * The code for this function is small, but it is has a lot
 * of debuging code, remove later. Chema
 *
 **/
guint
gedit_print_document_determine_lines (PrintJobInfo *pji, gboolean real)
{
	gint lines = 0;
	gint i, temp_i, j;
	gint chars_in_this_line = 0;

	/* use local variables so that this code can be reused */
	guchar * buffer = pji->buffer;
	gint chars_per_line = pji->chars_per_line;
	gint tab_size = pji->tab_size;
	gint wrapping = pji->wrapping;
	gint buffer_size = pji->buffer_size;
	gint lines_per_page = pji->lines_per_page; /* Needed for dump_text stuff */ 

	int dump_info = FALSE;
	int dump_info_basic = FALSE;
	int dump_text = FALSE;

	gedit_debug (DEBUG_PRINT, "");

	/* here we modify real if !pji->wrapping */
	if (real && !wrapping)
		real = FALSE;

	if (!real)
	{
		dump_info = FALSE;
		dump_info_basic = FALSE;
		dump_text = FALSE;
	}
		
	if (dump_info_basic)
	{
		if (real)
			g_print ("Determining lines in REAL mode. Lines Per page =%i\n", lines_per_page);
		else
			g_print ("Determining lines in WRAPPING mode. Lines Per page =%i\n", lines_per_page);
	}
	
	if (dump_text && lines%lines_per_page == 0)
		g_print("\n\n-Page %i-\n\n", lines / lines_per_page + 1);
	
	for (i=0; i < buffer_size; i++)
	{
		chars_in_this_line++;

		if (buffer[i] != '\t' && dump_text)
			g_print("%c", buffer[i]);

		if (buffer[i] == '\n')
		{
			lines++;
			if (dump_text && lines%lines_per_page == 0)
				g_print("\n\n-Page %i-\n\n", lines/lines_per_page + 1);
			
			chars_in_this_line=0;
			continue;
		}

		if (buffer[i] == '\t')
		{
			temp_i = chars_in_this_line;

			chars_in_this_line += tab_size - ((chars_in_this_line-1) % tab_size) - 1;

			if (chars_in_this_line > chars_per_line + 1)
			    chars_in_this_line = chars_per_line + 1;

			if (dump_text)
				for (j=temp_i;j<chars_in_this_line+1;j++)
					g_print(".");
			/*
			g_print("\ntabs agregados = %i\n", chars_in_this_line - temp_i);
			*/
		}


		/* Do word wapping here */ 
		if (chars_in_this_line == chars_per_line + 1 && real)
		{
			if (dump_info)
				g_print ("\nThis lines needs breaking\n");

			temp_i = i; /* We need to save the value of i in case we have to break the line */

			/* We back i till we find a space that we can break the line at */
			while (buffer[i] != ' ' && buffer[i] != '\t' && i > temp_i - chars_per_line - 1 )
			{
				i--;
				if (dump_text)
					g_print("\b");
			}

			if (dump_info)
				g_print("i got backed up [%i] times\n", temp_i - i);

			/* If this line was "unbreakable" break it at chars_per_line width */
			if (i == temp_i - chars_per_line - 1)
			{
				i = temp_i;
				if (dump_info)
					g_print ("We are breaking the line\n");
			}

			/* We need to remove the trailing blanks so that the next line does not start with
			   a space or a tab char */
			temp_i = i; /* Need to be able to determine who many spaces/tabs where removed */
			while (buffer[i] == ' ' || buffer[i] == '\t')
				i++;
			if (dump_info && i!=temp_i)
				g_print("We removed %i trailing spaces/tabs", i - temp_i);

			/* We need to back i 1 time because this is a for loop and we did not processed the
			   last character */
			i--;
			lines++;
			if (dump_text && lines%lines_per_page == 0)
				g_print("\n\n-Page %i-\n\n", lines / lines_per_page + 1);

			chars_in_this_line = 0;

			if (dump_text)
				g_print("\n");
		}

	}

	/* If the last line did not finished with a '\n' increment lines */
	if (buffer[i]!='\n')
	{
		lines++;
		if (dump_info_basic)
			g_print("\nAdding one line because it was not terminated with a slash+n\n");
	}

	if (dump_info_basic)
	{
		g_print("determine_lines found %i lines.\n", lines);
	}
	
	temp_i = lines;
	
        /* After counting, scan the doc backwards to determine how many
	   blanks lines there are (at the bottom),substract that from lines */
	for ( i = buffer_size-1; i>0; i--)
	{
		if ( buffer[i] != '\n' && buffer[i] != ' ' && buffer[i] != '\t')
			break;
		else
			if (buffer[i] == '\n')
				lines--;
	}

	if (dump_info_basic && lines != temp_i)
	{
		g_print("We removed %i line(s) because they contained no text\n", temp_i - lines);
	}

	if (dump_text)
		g_print(".\n.\n.\n");

	return lines;
}

static void
print_start_job (PrintJobInfo *pji)
{
	gedit_debug (DEBUG_PRINT, "");

	print_set_orientation(pji);
}

static void
print_set_orientation (PrintJobInfo *pji)
{
	double affine [6];

	if (pji->orientation == PRINT_ORIENT_PORTRAIT)
		return;

	art_affine_rotate (affine, 90.0);
	gnome_print_concat (pji->pc, affine);

	art_affine_translate (affine, 0, -pji->page_height);
	gnome_print_concat (pji->pc, affine);

}


static void
print_header (PrintJobInfo *pji, unsigned int page)
{
	guchar* text1 = g_strdup (pji->filename);
	guchar* text2 = g_strdup_printf (_("Page: %i/%i"), page, pji->pages);
	float x,y,len;
	
	gedit_debug (DEBUG_PRINT, "");

	gnome_print_setfont (pji->pc, pji->font_header);

	/* Print the file name */
	y = pji->page_height - pji->margin_top - pji->header_height/2;
	len = gnome_font_get_width_string (pji->font_header, text1);
	x = pji->page_width/2 - len/2;
	gnome_print_moveto(pji->pc, x, y);
	gedit_print_show_iso8859_1 (pji->pc, text1);

	/* Print the page/pages  */
	y = pji->page_height - pji->margin_top - pji->header_height/4;
	len = gnome_font_get_width_string (pji->font_header, text2);
	x = pji->page_width - len - 36;
	gnome_print_moveto (pji->pc, x, y);
	gedit_print_show_iso8859_1 (pji->pc, text2);

	g_free (text1);
	g_free (text2);
}

	

static void
print_end_page (PrintJobInfo *pji)
{
	gedit_debug (DEBUG_PRINT, "");

	gnome_print_showpage (pji->pc);
	print_set_orientation (pji);

}

static void
print_end_job (GnomePrintContext *pc)
{
	gedit_debug (DEBUG_PRINT, "");


}

