/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-print.c
 * This file is part of gedit
 *
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002  Paolo Maggi  
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/libgnome.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-master-preview.h>
#include <eel/eel-string.h>

#include <string.h>	/* For strlen */

#include "gedit2.h"
#include "gedit-print.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-prefs-manager.h"

#define CM(v) ((v) * 72.0 / 2.54)
#define A4_WIDTH (210.0 * 72 / 25.4)
#define A4_HEIGHT (297.0 * 72 / 25.4)


static GnomePrintConfig *gedit_print_config = NULL;

typedef struct _GeditPrintJobInfo	GeditPrintJobInfo;

struct _GeditPrintJobInfo {

	GeditDocument 		*doc;
	
	gboolean 		 preview;

	GnomePrintConfig 	*config;
	gint			 range_type;

	GnomePrintMaster	*print_master;
	GnomePrintContext	*print_ctx;

	gint			 page_num;
	/* Page stuff*/
	gdouble			 page_width;
	gdouble			 page_height;
	gdouble			 margin_top;
	gdouble			 margin_left;
	gdouble			 margin_right;
	gdouble			 margin_bottom;
	gdouble			 header_height;
	gdouble			 numbers_width;
	
	/* Fonts */
	GnomeFont		*font_body;
	GnomeFont		*font_header;
	GnomeFont		*font_numbers;	

	gint 			 first_line_to_print;
	gint			 printed_lines;

	gint 			 print_line_numbers;
	gboolean		 print_header;
	gint			 tabs_size;
	GtkWrapMode		 print_wrap_mode;
};


/* FIXME: remove when libgnomeprint well define it -- Paolo */
GnomeGlyphList *gnome_glyphlist_unref (GnomeGlyphList *gl);

static GQuark gedit_print_error_quark (void);
#define GEDIT_PRINT_ERROR gedit_print_error_quark ()	

static GeditPrintJobInfo* gedit_print_job_info_new (GeditDocument* doc, GError **error);
static void gedit_print_job_info_destroy (GeditPrintJobInfo *pji);

static void gedit_print_preview_real 	(GeditPrintJobInfo *pji);
static void 	gedit_print_real 	(GeditDocument* doc, gboolean preview, GError **error);
static gboolean	gedit_print_run_dialog 	(GeditPrintJobInfo *pji);

static void gedit_print_error_dialog (GError *error);

static const gchar* gedit_print_get_next_line_to_print_delimiter (GeditPrintJobInfo *pji, 
					const gchar *start, const gchar *end, gboolean first_line_of_par);

static void gedit_print_line 		(GeditPrintJobInfo *pji, const gchar *start, 
		                         const gchar *end, gdouble y, gboolean first_line_of_par);
static gdouble gedit_print_paragraph (GeditPrintJobInfo *pji, const gchar *start, 
		  			 const gchar *end, gdouble y);
static void gedit_print_line_number (GeditPrintJobInfo *pji, gdouble y);
static gdouble gedit_print_create_new_page (GeditPrintJobInfo *pji);
static void gedit_print_end_page (GeditPrintJobInfo *pji);



GQuark 
gedit_print_error_quark ()
{
  static GQuark quark;
  if (!quark)
    quark = g_quark_from_static_string ("gedit_print_error");

  return quark;
}


static GeditPrintJobInfo* 
gedit_print_job_info_new (GeditDocument* doc, GError **error)
{	
	GeditPrintJobInfo *pji;
	
	gchar *print_font_body;
	gchar *print_font_header;
	gchar *print_font_numbers;
	
	gedit_debug (DEBUG_PRINT, "");
	
	g_return_val_if_fail (doc != NULL, NULL);

	if (gedit_print_config == NULL)
	{
		gedit_print_config = gnome_print_config_default ();
		g_return_val_if_fail (gedit_print_config != NULL, NULL);

		gnome_print_config_set (gedit_print_config, "Settings.Transport.Backend", "lpr");
		gnome_print_config_set (gedit_print_config, "Printer", "GENERIC");
	}

	pji = g_new0 (GeditPrintJobInfo, 1);

	pji->doc = doc;
	pji->preview = FALSE;
	pji->range_type = GNOME_PRINT_RANGE_ALL;

	pji->page_num = 0;

	pji->page_width = A4_WIDTH;
	pji->page_height = A4_HEIGHT;
	pji->margin_top = CM (1);
	pji->margin_left = CM (1);
	pji->margin_right = CM (1);
	pji->margin_bottom = CM (1);
	pji->header_height = CM (0);

	print_font_body = gedit_prefs_manager_get_print_font_body ();
	print_font_header = gedit_prefs_manager_get_print_font_header ();
	print_font_numbers = gedit_prefs_manager_get_print_font_numbers ();
	
	pji->font_body = gnome_font_find_closest_from_full_name (print_font_body);
	pji->font_header = gnome_font_find_closest_from_full_name (print_font_header);
	pji->font_numbers = gnome_font_find_closest_from_full_name (print_font_numbers);

	g_free (print_font_body);
	g_free (print_font_header);
	g_free (print_font_numbers);

	if ((pji->font_body == NULL) || 
	    (pji->font_header == NULL) ||
	    (pji->font_numbers == NULL))
	{
		g_set_error (error, GEDIT_PRINT_ERROR, 1,
			_("gedit is unable to print since it cannot find\n"
			  "one of the required fonts."));
		
		gedit_print_job_info_destroy (pji);
		
		return NULL;
	}
	
	if (gedit_prefs_manager_get_print_header ())
		pji->header_height = 2.5 * gnome_font_get_size (pji->font_header);
	else
		pji->header_height = 0;

	pji->numbers_width = CM(0);
	
	pji->first_line_to_print = 1;
	pji->printed_lines = 0;

	pji->print_line_numbers = gedit_prefs_manager_get_print_line_numbers ();
	pji->print_header = gedit_prefs_manager_get_print_header ();
	pji->tabs_size = gedit_prefs_manager_get_tabs_size ();
	pji->print_wrap_mode = gedit_prefs_manager_get_print_wrap_mode ();
		
	pji->config = gedit_print_config;
	gnome_print_config_ref (pji->config);

	return pji;
}	

static void
gedit_print_job_info_destroy (GeditPrintJobInfo *pji)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (pji != NULL);
	
	if (pji->config != NULL)
		gnome_print_config_unref (pji->config);

	if (pji->print_ctx != NULL)
		g_object_unref (pji->print_ctx);

	if (pji->print_master != NULL)
		g_object_unref (pji->print_master);

	g_free (pji);
}

static void
gedit_print_update_page_size_and_margins (GeditPrintJobInfo *pji)
{
	const GnomePrintUnit *unit;
		
	gedit_debug (DEBUG_PRINT, "");

	gnome_print_master_get_page_size_from_config (pji->config, 
			&pji->page_width, &pji->page_height);

	if (gnome_print_config_get_length (pji->config, GNOME_PRINT_KEY_PAGE_MARGIN_LEFT, 
				&pji->margin_left, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_left, unit, GNOME_PRINT_PS_UNIT);
	}
	
	if (gnome_print_config_get_length (pji->config, GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT, 
				&pji->margin_right, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_right, unit, GNOME_PRINT_PS_UNIT);
	}
	
	if (gnome_print_config_get_length (pji->config, GNOME_PRINT_KEY_PAGE_MARGIN_TOP, 
				&pji->margin_top, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_top, unit, GNOME_PRINT_PS_UNIT);
	}
	if (gnome_print_config_get_length (pji->config, GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM, 
				&pji->margin_bottom, &unit)) 
	{
		gnome_print_convert_distance (&pji->margin_bottom, unit, GNOME_PRINT_PS_UNIT);
	}

	if (pji->print_line_numbers  > 0)
	{
		gchar* num_str;

		num_str = g_strdup_printf ("%d", gedit_document_get_line_count (pji->doc));
		pji->numbers_width = gnome_font_get_width_utf8 (pji->font_numbers, num_str);
		g_free (num_str);
	}
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
gedit_print_run_dialog (GeditPrintJobInfo *pji)
{
	GtkWidget *dialog;
	gint selection_flag;
	gint res;
	gint lines;

	gedit_debug (DEBUG_PRINT, "");

	g_return_val_if_fail (pji != NULL, TRUE);
	
	if (!gedit_document_has_selected_text (pji->doc))
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	else
		selection_flag = GNOME_PRINT_RANGE_SELECTION;
		
	dialog = g_object_new (GNOME_TYPE_PRINT_DIALOG, "print_config", pji->config, NULL);
	
	gnome_print_dialog_construct (GNOME_PRINT_DIALOG (dialog), 
				      _("Print"),
			              GNOME_PRINT_DIALOG_RANGE | GNOME_PRINT_DIALOG_COPIES);
	
	lines = gedit_document_get_line_count (pji->doc);
	
	gnome_print_dialog_construct_range_page ( GNOME_PRINT_DIALOG (dialog),
						  GNOME_PRINT_RANGE_ALL |
						  /*GNOME_PRINT_RANGE_RANGE |*/
						  selection_flag,
						  1, lines, "A", _("Lines"));

	gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));

	res = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (res) 
	{
		case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
			gedit_debug (DEBUG_PRINT, "Print button pressed.");
			break;
		case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
			gedit_debug (DEBUG_PRINT, "Preview button pressed.");
			pji->preview = TRUE;
			break;
		default:
			gtk_widget_destroy (dialog);
			return TRUE;
	}
	
	pji->range_type = gnome_print_dialog_get_range (GNOME_PRINT_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	gedit_debug (DEBUG_PRINT, "Print dialog destroyed.");

	return FALSE;
}

static void
gedit_print_header (GeditPrintJobInfo *pji, gint page_number)
{
	gdouble y, x, len;
	gchar* l_text;
	gchar* r_text;
	gchar* uri;
	gchar* uri_to_print;
	
	gedit_debug (DEBUG_PRINT, "");	

	g_return_if_fail (pji != NULL);
	g_return_if_fail (pji->font_header != NULL);
	g_return_if_fail (pji->doc != NULL);

	gnome_print_setfont (pji->print_ctx, pji->font_header);

	y = pji->page_height - pji->margin_top -
		gnome_font_get_ascender (pji->font_header);

	/* Print left text */
	x = pji->margin_right;
	
	uri = gedit_document_get_uri (pji->doc);
	uri_to_print = eel_str_middle_truncate (uri, 60);
	g_free (uri);

	l_text = g_strdup_printf (_("File: %s"), uri_to_print);
	g_free (uri_to_print);

	gnome_print_moveto (pji->print_ctx, x, y);
	gnome_print_show (pji->print_ctx, l_text);

	/* Print right text */
	r_text = g_strdup_printf (_("Page: %d"), page_number);
	len = gnome_font_get_width_utf8 (pji->font_header, r_text);

	x = pji->page_width - pji->margin_right - len;

	gnome_print_moveto (pji->print_ctx, x, y);
	gnome_print_show (pji->print_ctx, r_text);

	/* Print line */
	y = pji->page_height - pji->margin_top -
		(1.5 * gnome_font_get_size (pji->font_header));

	gnome_print_setlinewidth (pji->print_ctx, 1.0);
	gnome_print_moveto (pji->print_ctx,  pji->margin_right, y);
	gnome_print_lineto (pji->print_ctx, pji->page_width - pji->margin_right, y);
	gnome_print_stroke (pji->print_ctx);

	g_free (l_text);
	g_free (r_text);

	gnome_print_setfont (pji->print_ctx, pji->font_body);

}

static void
gedit_print_document (GeditPrintJobInfo *pji)
{
	gchar *text_to_print;
	gchar *text_end;
	const gchar *end;
	const gchar *p;
	gdouble y;
	gint paragraph_delimiter_index;
	gint next_paragraph_start;
	gdouble fontheight;
	gint start;
	
	gedit_debug (DEBUG_PRINT, "");	
	
	switch (pji->range_type)
	{
		case GNOME_PRINT_RANGE_ALL:
			text_to_print = gedit_document_get_buffer (pji->doc);
			text_end = text_to_print + strlen (text_to_print);
			break;
		case GNOME_PRINT_RANGE_SELECTION:
			text_to_print = gedit_document_get_selected_text (pji->doc, &start, NULL);
			text_end = text_to_print + strlen (text_to_print);

			pji->first_line_to_print =  
				gedit_document_get_line_at_offset (pji->doc, start) + 1;
			break;
		default:
			g_return_if_fail (FALSE);
	}

	y = gedit_print_create_new_page (pji);	

	gnome_print_setfont (pji->print_ctx, pji->font_body);

	pango_find_paragraph_boundary (text_to_print, -1, 
			&paragraph_delimiter_index, &next_paragraph_start);

	p = text_to_print;
	end = text_to_print + paragraph_delimiter_index;

	fontheight = gnome_font_get_ascender (pji->font_body) + 
				gnome_font_get_descender (pji->font_body);

	while (p < text_end)
	{
		gunichar wc = g_utf8_get_char (p);
  		/* Only one character has type G_UNICODE_PARAGRAPH_SEPARATOR in
		 * Unicode 3.0; update this if that changes.
   		 */
#define PARAGRAPH_SEPARATOR 0x2029

		if ((wc == '\n' ||
           	     wc == '\r' ||
                     wc == PARAGRAPH_SEPARATOR))
		{
			if (pji->numbers_width > 0.0)
				gedit_print_line_number (pji, y);
	
			y -= 1.2 * gnome_font_get_size (pji->font_body);

			if ((y - pji->margin_bottom) < fontheight)
			{
				gedit_print_end_page (pji);
			
				y = gedit_print_create_new_page (pji);
			}
		}
		else
		{	
			y = gedit_print_paragraph (pji, p, end, y);
		}
		
		++pji->printed_lines;
		
		p = p + next_paragraph_start;
		
		if (p < text_end)
		{
			pango_find_paragraph_boundary (p, -1, 
				&paragraph_delimiter_index, &next_paragraph_start);
			end = p + paragraph_delimiter_index;
		}
		
	}

	gnome_print_showpage (pji->print_ctx);

	g_free (text_to_print);
}

/**
 * gedit_print_preview_real:
 * @pji: 
 * 
 * Create and show the print preview window
 **/
static void
gedit_print_preview_real (GeditPrintJobInfo *pji)
{
	GtkWidget *gpmp;
	gchar *title;
	
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (pji != NULL);
	g_return_if_fail (pji->print_master != NULL);

	title = g_strdup_printf (_("gedit - Print Preview"));
	gpmp = gnome_print_master_preview_new (pji->print_master, title);
	g_free (title);

	gtk_widget_show (gpmp);
}

/**
 * gedit_print_real:
 * @preview: 
 * 
 * The main printing function
 **/
static void
gedit_print_real (GeditDocument* doc, gboolean preview, GError **error)
{
	GeditPrintJobInfo *pji;
	gboolean cancel = FALSE;

	gedit_debug (DEBUG_PRINT, "");
		
	pji = gedit_print_job_info_new (doc, error);
	
	if (pji == NULL)
		return;
	
	pji->preview = preview;

	if (!pji->preview)
		/* 
		 * Open the print dialog and initialize
		 * pji->config 
		 */
		cancel = gedit_print_run_dialog (pji);

	/* The canceled button on the dialog was clicked */
	if (cancel) {
		gedit_print_job_info_destroy (pji);
		return;
	}

	g_return_if_fail (pji->config != NULL);

	pji->print_master = gnome_print_master_new_from_config (pji->config);
	g_return_if_fail (pji->print_master != NULL);

	pji->print_ctx = gnome_print_master_get_context (pji->print_master);
	g_return_if_fail (pji->print_ctx != NULL);

	gedit_print_update_page_size_and_margins (pji);
	gedit_print_document (pji);
#if 0
	/* The printing was canceled while in progress */
	if (pji->canceled) {
		gedit_print_job_info_destroy (pji);
		return;
	}
#endif	
	gnome_print_master_close (pji->print_master);

	if (pji->preview)
		gedit_print_preview_real (pji);
	else
		gnome_print_master_print (pji->print_master);
	
	gedit_print_job_info_destroy (pji);
}

static void 
gedit_print_error_dialog (GError *error)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			_("An error occurred while printing.\n\n%s,"),
			error->message);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}


void 
gedit_print (GeditMDIChild* active_child)
{
	GeditDocument *doc;
	GError *error = NULL;
	
	gedit_debug (DEBUG_PRINT, "");
	
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	gedit_print_real (doc, FALSE, &error);

	if (error)
	{
		gedit_print_error_dialog (error);
		g_error_free (error);
	}
}

void 
gedit_print_preview (GeditMDIChild* active_child)
{
	GeditDocument *doc;
	GError *error = NULL;
	
	gedit_debug (DEBUG_PRINT, "");
	
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	gedit_print_real (doc, TRUE, &error);

	if (error)
	{
		gedit_print_error_dialog (error);
		g_error_free (error);
	}
}

static const gchar*
gedit_print_get_next_line_to_print_delimiter (GeditPrintJobInfo *pji, 
		const gchar *start, const gchar *end, gboolean first_line_of_par)
{
	const gchar* p;
	gdouble line_width = 0.0;
	gdouble printable_page_width;
	ArtPoint space_advance;
	gint space;
	gint chars_in_this_line = 0;
	
	gedit_debug (DEBUG_PRINT, "");

	if (pji->numbers_width > 0.0)
		printable_page_width = pji->page_width - 
			(pji->margin_right + pji->margin_left) - pji->numbers_width - CM(0.5);
	else
		printable_page_width = pji->page_width - 
			(pji->margin_right + pji->margin_left);
	
	p = start;

	/* Find space advance */
	space = gnome_font_lookup_default (pji->font_body, ' ');
	gnome_font_get_glyph_stdadvance (pji->font_body, space, &space_advance);

	if (!first_line_of_par)
	{
		gunichar ch = g_utf8_get_char (p);
		
		while (ch == ' ' || ch == '\t')
		{
			p = g_utf8_next_char (p);
			ch = g_utf8_get_char (p);
		}
	}

	while (p < end)	
	{
		gunichar ch;
	       	gint glyph;
		
		ch = g_utf8_get_char (p);
		++chars_in_this_line;
		
		if (ch == '\t')
		{
			/* FIXME: use a tabs array */
			gint num_of_equivalent_spaces;
			
			num_of_equivalent_spaces = pji->tabs_size - ((chars_in_this_line - 1) % pji->tabs_size); 
			chars_in_this_line += num_of_equivalent_spaces - 1;
			
			line_width += (num_of_equivalent_spaces * space_advance.x); 
		}
		else
		{
			glyph = gnome_font_lookup_default (pji->font_body, ch);

			/* FIXME */
			if (glyph == space)
				line_width += space_advance.x;
			else
			/*
			if ((glyph < 0) || (glyph >= 256))
			*/
			{
				ArtPoint adv;
				gnome_font_get_glyph_stdadvance (pji->font_body, glyph, &adv);
				if (adv.x > 0)
					line_width += adv.x;
				else
					/* To be as conservative as possible */
					line_width += (2 * space_advance.x);

			}
			/*
			}
			else
			line_width += gnome_font_get_glyph_width (pji->font_body, glyph);
			*/
		}

		if (line_width > printable_page_width)
		{
			/* FIXME: take care of lines wrapping at word boundaries too */
			return p;
		}

		p = g_utf8_next_char (p);
	}

	return end;
}

static void
gedit_print_line_number (GeditPrintJobInfo *pji, gdouble y)
{
	gchar *num_str;
	gdouble len;
	gdouble x;
		
	gint line_num = pji->first_line_to_print + pji->printed_lines;

	gedit_debug (DEBUG_PRINT, "");

	if ((pji->printed_lines + 1) % pji->print_line_numbers != 0)
	       return;	
		
	num_str = g_strdup_printf ("%d", line_num);

	gnome_print_setfont (pji->print_ctx, pji->font_numbers);

	len = gnome_font_get_width_utf8 (pji->font_numbers, num_str);

	x = pji->margin_left + pji->numbers_width - len;

	gnome_print_moveto (pji->print_ctx, x, y - 
				gnome_font_get_ascender (pji->font_numbers));
	gnome_print_show (pji->print_ctx, num_str);

	g_free (num_str);

	/* Restore body font */
	gnome_print_setfont (pji->print_ctx, pji->font_body);
}	

static void
gedit_print_line (GeditPrintJobInfo *pji, const gchar *start, const gchar *end, gdouble y, 
		gboolean first_line_of_par)
{
	GnomeGlyphList * gl;
	gdouble x;
	const gchar* p;
	gint chars_in_this_line = 0;
	
	gedit_debug (DEBUG_PRINT, "");

	p = start;
	
	gl = gnome_glyphlist_from_text_dumb (pji->font_body, 0x000000ff, 0.0, 0.0, "");
	
	gnome_glyphlist_advance (gl, TRUE);
	
	if (pji->numbers_width > 0.0)
	{
		if (first_line_of_par)
			gedit_print_line_number (pji, y);

		x = pji->margin_left + pji->numbers_width + CM(0.5);
	}
	else
		x = pji->margin_left;
	
	gnome_glyphlist_moveto (gl, x, y - gnome_font_get_ascender (pji->font_body));
	
	if (!first_line_of_par)
	{
		gunichar ch = g_utf8_get_char (p);
		
		while (ch == ' ' || ch == '\t')
		{
			p = g_utf8_next_char (p);
			ch = g_utf8_get_char (p);
		}
	}
		
	while (p < end)
	{
		gunichar ch;
	       	gint glyph;
		
		ch = g_utf8_get_char (p);
		++chars_in_this_line;
		
		if (ch == '\t')
		{
			/* FIXME: use a tabs array */
			gint i;
			gint num_of_equivalent_spaces;
			
			glyph = gnome_font_lookup_default (pji->font_body, ' ');
			
			num_of_equivalent_spaces = pji->tabs_size - 
				((chars_in_this_line - 1) % pji->tabs_size); 

			chars_in_this_line += num_of_equivalent_spaces - 1;
			
			for (i = 0; i < num_of_equivalent_spaces; ++i)
				gnome_glyphlist_glyph (gl, glyph);
	
		}
		else
		{
			glyph = gnome_font_lookup_default (pji->font_body, ch);
			gnome_glyphlist_glyph (gl, glyph);
		}

		p = g_utf8_next_char (p);
	}

	gnome_print_moveto (pji->print_ctx, 0.0, 0.0);
	gnome_print_glyphlist (pji->print_ctx, gl);
	gnome_glyphlist_unref (gl);
}

static gdouble
gedit_print_paragraph (GeditPrintJobInfo *pji, const gchar *start, const gchar *end, gdouble y)
{
	const gchar *p, *s;
	gdouble fontheight;
	gboolean fl;

	gedit_debug (DEBUG_PRINT, "");
	
	p = start;

	fl = TRUE;

	fontheight = gnome_font_get_ascender (pji->font_body) + 
				gnome_font_get_descender (pji->font_body);

	while (p < end)
	{
		s = p;
		p = gedit_print_get_next_line_to_print_delimiter (pji, s, end, fl);
		gedit_print_line (pji, s, p, y, fl);
		y -= 1.2 * gnome_font_get_size (pji->font_body);
		fl = FALSE;
		
		if ((y - pji->margin_bottom) < fontheight)
		{
			gedit_print_end_page (pji);
			
			y = gedit_print_create_new_page (pji);
		}

		/* FIXME: "word wrap" - Paolo */
		if (pji->print_wrap_mode == GTK_WRAP_NONE)
			return y;

	}
	
	return y;
}

static gdouble
gedit_print_create_new_page (GeditPrintJobInfo *pji)
{
	gchar *pn_str;

	gedit_debug (DEBUG_PRINT, "");

	++pji->page_num;
	pn_str = g_strdup_printf ("%d", pji->page_num);
	gnome_print_beginpage (pji->print_ctx, pn_str);

	g_free (pn_str);

	if (pji->print_header)
	{	
		gedit_print_header (pji, +pji->page_num);
		return pji->page_height - pji->margin_top - pji->header_height;
	}
	else
		return pji->page_height - pji->margin_top;
}

static void
gedit_print_end_page (GeditPrintJobInfo *pji)
{
	gnome_print_showpage (pji->print_ctx);
}
	
