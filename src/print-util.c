/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit : print-util.c . Utility functions for printing.
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
#include <locale.h>

#include <libgnomeprint/gnome-print.h>

#include "document.h" /* utils.h needs it, fix */
#include "utils.h"
#include "window.h"
#include "print.h" /* for the defined fontnames */
#include "print-util.h"

#define _PROPER_I18N

/* Taken from gnumeric */
/**
 * g_unichar_to_utf8:
 * @c: a ISO10646 character code
 * @outbuf: output buffer, must have at least 6 bytes of space.
 *       If %NULL, the length will be computed and returned
 *       and nothing will be written to @out.
 * 
 * Convert a single character to utf8
 * 
 * Return value: number of bytes written
 **/
static int
g_unichar_to_utf8 (gint c, gchar *outbuf)
{
  size_t len = 0;
  int first;
  int i;

  if (c < 0x80)
    {
      first = 0;
      len = 1;
    }
  else if (c < 0x800)
    {
      first = 0xc0;
      len = 2;
    }
  else if (c < 0x10000)
    {
      first = 0xe0;
      len = 3;
    }
   else if (c < 0x200000)
    {
      first = 0xf0;
      len = 4;
    }
  else if (c < 0x4000000)
    {
      first = 0xf8;
      len = 5;
    }
  else
    {
      first = 0xfc;
      len = 6;
    }

  if (outbuf)
    {
      for (i = len - 1; i > 0; --i)
	{
	  outbuf[i] = (c & 0x3f) | 0x80;
	  c >>= 6;
	}
      outbuf[0] = c | first;
    }

  return len;
}

#ifndef _PROPER_I18N
/*
 * print_show_iso8859_1
 *
 * Like gnome_print_show, but expects an ISO 8859.1 string.
 *
 * NOTE: This function got introduced when gnome-print switched to UTF-8,
 * and will disappear again once Gnumeric makes the switch. Deprecated at
 * birth! 
 */
static int
gedit_print_show_iso8859_1 (GnomePrintContext *pc, char const *text)
{
	gchar *p, *utf, *udyn, ubuf[4096];
	gint len, ret, i;

	g_return_val_if_fail (pc && text, -1);

	if (!*text)
		return 0;

	/* We need only length * 2, because iso-8859-1 is encoded in 1-2 bytes */
	len = strlen (text);
	if (len * 2 > sizeof (ubuf)) {
		udyn = g_new (gchar, len * 2);
		utf = udyn;
	} else {
		udyn = NULL;
		utf = ubuf;
	}
	p = utf;

	for (i = 0; i < len; i++) {
		p += g_unichar_to_utf8 (((guchar *) text)[i], p);
	}

	ret = gnome_print_show_sized (pc, utf, p - utf);

	if (udyn)
		g_free (udyn);

	return ret;
}
#endif

/** 
 * This code is also taken from gnumeric, but from version 0.63. It's fully 
 * i18n-wise correct - so feel free to copy it from here everywhere you can 
 * (but please copy it from gnumeric, since it also has a routine for 
 * measuring string width properly). 
 *  Argument 'text' is text that is in current locale's encoding.
 *	Code is (c) 2001 Vlad Harchev <hvv@hippo.ru>
 */
int
gedit_print_show (GnomePrintContext *pc, char const *text)
{
#ifdef _PROPER_I18N
	wchar_t* wcs,wcbuf[4096];
	char* utf8,utf8buf[4096];
	
	int conv_status;
	int n = strlen(text);
	int retval;

	g_return_val_if_fail (pc && text, -1);	
	
	if ( n > (sizeof(wcbuf)/sizeof(wcbuf[0])))
		wcs = g_new(wchar_t,n);
	else
		wcs = wcbuf;

	conv_status = mbstowcs(wcs, text, n);

	if (conv_status == (size_t)(-1)){
		if (wcs != wcbuf)
			g_free (wcs);
		return 0;
	};
	if (conv_status * 6 > sizeof(utf8buf))
		utf8 = g_new(gchar, conv_status * 6);
	else
		utf8 = utf8buf;
	{
		int i;
		char* p = utf8;
		for(i = 0; i < conv_status; ++i)
			p += g_unichar_to_utf8 ( (gint) wcs[i], p);
		if (wcs != wcbuf)
			g_free(wcs);			
		retval = gnome_print_show_sized (pc, utf8, p - utf8);			
	}	

	if (utf8 != utf8buf)
		g_free(utf8);
	return retval;		
#else
	return gedit_print_show_iso8859_1 (pc, text);
#endif
};


/**
 * gedit_print_verify_fonts:
 * @void: 
 * 
 * verify that the fonts that we are going to use are available
 * 
 * Return Value: 
 **/
gboolean
gedit_print_verify_fonts (void)
{
	GnomeFont *test_font;
	guchar * test_font_name;

	gedit_debug (DEBUG_PRINT, "");

	/* Courier */
	test_font_name = g_strdup (GEDIT_PRINT_FONT_BODY);
	test_font = gnome_font_new (test_font_name, 10);
	if (test_font==NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit could not find the font \"%s\".\n"
						   "gedit is unable to print without this font installed."),
						 test_font_name);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return FALSE;
	}
	gnome_font_unref (test_font);
	g_free (test_font_name);
	
	/* Helvetica  */
	test_font_name = g_strdup (GEDIT_PRINT_FONT_HEADER);
	test_font = gnome_font_new (test_font_name, 10);
	if (test_font==NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit could not find the font \"%s\".\n"
						   "gedit is unable to print without this font installed."),
						 test_font_name);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return FALSE;
	}
	gnome_font_unref (test_font);
	g_free (test_font_name);	
 
	return TRUE;
}


static void
gedit_print_progress_clicked (GtkWidget *button, gpointer data)
{
	PrintJobInfo *pji = (PrintJobInfo *) data;

	/* Now, i dunno how you could click a button inside a dialog
	 * that does not exits, but I could not resist to add this
	 * lines */
	if (pji->progress_dialog == NULL)
		return;
	
	pji->canceled = TRUE;
}

void
gedit_print_progress_start (PrintJobInfo *pji)
{
	GnomeDialog *dialog;
	GtkWidget *label;
	GtkWidget *progress_bar;

	/* For small docs, the dialog looks lame */
	if (pji->page_last - pji->page_first < 31)
		return;

	progress_bar = gtk_progress_bar_new ();
	
	dialog = GNOME_DIALOG (gnome_dialog_new (_("Printing .."),
						 GNOME_STOCK_BUTTON_CANCEL,
						 NULL));

	gnome_dialog_button_connect (dialog, 0,
				     GTK_SIGNAL_FUNC (gedit_print_progress_clicked),
				     pji);

	pji->progress_dialog = dialog;
	pji->progress_bar    = progress_bar;
	
	label = gtk_label_new (_("Printing ..."));
	
	gtk_box_pack_start (GTK_BOX (dialog->vbox),
			    label,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox),
			    progress_bar,
			    FALSE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET (dialog));

}

void
gedit_print_progress_tick (PrintJobInfo *pji, gint page)
{
	gfloat percentage;

	while (gtk_events_pending ())
		gtk_main_iteration ();

	if (pji->progress_dialog == NULL)
		return;
	
	percentage = (gfloat ) (page - pji->page_first) /
		(gfloat) (pji->page_last - pji->page_first);

	if (percentage > 0.0 && percentage < 1.0 )
		gtk_progress_set_percentage (GTK_PROGRESS (pji->progress_bar),
					     percentage );

}

void
gedit_print_progress_end (PrintJobInfo *pji)
{
	if (pji->progress_dialog == NULL)
		return;
		       
	gnome_dialog_close (pji->progress_dialog);

	pji->progress_dialog = NULL;
	pji->progress_bar = NULL;
}

