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

#include <libgnomeprint/gnome-print.h>

#include "document.h" /* utils.h needs it, fix */
#include "utils.h"
#include "window.h"
#include "print.h" /* for the defined fontnames */
#include "print-util.h"

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


/*
 * print_show_iso8859_1
 *
 * Like gnome_print_show, but expects an ISO 8859.1 string.
 *
 * NOTE: This function got introduced when gnome-print switched to UTF-8,
 * and will disappear again once Gnumeric makes the switch. Deprecated at
 * birth! 
 */
int
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



void
gedit_print_progress_start (PrintJobInfo *pji)
{
	GnomeDialog *dialog;
	GtkWidget *label;
	GtkWidget *progress_bar;

	progress_bar = gtk_progress_bar_new ();
	
	dialog = GNOME_DIALOG (gnome_dialog_new (_("Printing .."),
						 GNOME_STOCK_BUTTON_CANCEL,
						 NULL));
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

	percentage = (gfloat ) (page - pji->page_first) /
		(gfloat) (pji->page_last - pji->page_first);

	if (percentage > 0.0 && percentage < 1.0 )
		gtk_progress_set_percentage (GTK_PROGRESS (pji->progress_bar),
					     percentage );

}

void
gedit_print_progress_end (PrintJobInfo *pji)
{
	gnome_dialog_close (pji->progress_dialog);
}

