/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-utils.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include "gedit-utils.h"
#include "gedit2.h"

#include "bonobo-mdi.h"
#include <libgnomeui/libgnomeui.h>
#include "gnome-vfs-helpers.h"

/* =================================================== */
/* Flash */

struct _MessageInfo {
  BonoboWindow * win;
  guint timeoutid;
  guint handlerid;
};

typedef struct _MessageInfo MessageInfo;

static gint
remove_message_timeout (MessageInfo * mi) 
{
	BonoboUIEngine *ui_engine;

	GDK_THREADS_ENTER ();	
	
  	ui_engine = bonobo_window_get_ui_engine (mi->win);
	g_return_val_if_fail (ui_engine != NULL, FALSE);

	bonobo_ui_engine_remove_hint (ui_engine);

	g_free (mi);

  	GDK_THREADS_LEAVE ();

  	return FALSE; /* removes the timeout */
}

/* Called if the win is destroyed before the timeout occurs. */
static void
remove_timeout_cb (GtkWidget *win, MessageInfo *mi ) 
{
 	gtk_timeout_remove (mi->timeoutid);
  	g_free (mi);
}

static const guint32 flash_length = 3000; /* 3 seconds, I hope */

/**
 * bonobo_win_flash
 * @app: Pointer a Bonobo window object
 * @flash: Text of message to be flashed
 *
 * Description:
 * Flash the message in the statusbar for a few moments; if no
 * statusbar, do nothing. For trivial little status messages,
 * e.g. "Auto saving..."
 **/

void 
bonobo_window_flash (BonoboWindow * win, const gchar * flash)
{
	BonoboUIEngine *ui_engine;
  	g_return_if_fail (win != NULL);
  	g_return_if_fail (BONOBO_IS_WINDOW (win));
  	g_return_if_fail (flash != NULL);
  	
	ui_engine = bonobo_window_get_ui_engine (win);
	g_return_if_fail (ui_engine != NULL);
	
	if (bonobo_ui_engine_xml_node_exists (ui_engine, "/status"))
	{
    		MessageInfo * mi;

		bonobo_ui_engine_remove_hint (ui_engine);
		bonobo_ui_engine_add_hint (ui_engine, flash);
    		
		mi = g_new(MessageInfo, 1);

    		mi->timeoutid = 
      			gtk_timeout_add (flash_length,
				(GtkFunction) remove_message_timeout,
				mi);
    
    		mi->handlerid = 
      			gtk_signal_connect (GTK_OBJECT(win),
				"destroy",
			   	GTK_SIGNAL_FUNC (remove_timeout_cb),
			   	mi );

    		mi->win       = win;
  	}   
}

/* ========================================================== */

/**
 * gedit_utils_flash:
 * @msg: Message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar of gedit.
 **/
void
gedit_utils_flash (gchar *msg)
{
	g_return_if_fail (msg != NULL);
	
	bonobo_window_flash (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi)), msg);

	/* Update UI */
	while (gtk_events_pending ())
		  gtk_main_iteration ();
}

/**
 * gedit_utils_flash_va:
 * @format:
 **/
void
gedit_utils_flash_va (gchar *format, ...)
{
	va_list args;
	gchar *msg;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	gedit_utils_flash (msg);
	g_free (msg);
}

static gboolean
istr_has_prefix (const char *haystack, const char *needle)
{
	const char *h, *n;
	char hc, nc;

	/* Eat one character at a time. */
	h = haystack == NULL ? "" : haystack;
	n = needle == NULL ? "" : needle;
	do {
		if (*n == '\0') {
			return TRUE;
		}
		if (*h == '\0') {
			return FALSE;
		}
		hc = *h++;
		nc = *n++;
		hc = g_ascii_tolower (hc);
		nc = g_ascii_tolower (nc);
	} while (hc == nc);
	return FALSE;
}

gboolean
gedit_utils_uri_has_file_scheme (const gchar *uri)
{
	gchar* canonical_uri = NULL;
	gboolean res;
	
	canonical_uri = gnome_vfs_x_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, FALSE);

	res = istr_has_prefix (canonical_uri, "file");
	
	g_free (canonical_uri);

	return res;
}

gboolean
gedit_utils_is_uri_read_only (const gchar* uri)
{
	gchar* file_uri = NULL;
	gint res;

	/* FIXME: all remote files are marked as readonly */
	if (!gedit_utils_uri_has_file_scheme (uri))
		return TRUE;
			
	file_uri = gnome_vfs_x_format_uri_for_display (uri);
		
	res = access (file_uri, W_OK);

	g_free (file_uri);

	return res;	
}

GtkWidget* 
gedit_button_new_with_stock_image (const gchar* text, const gchar* stock_id)
{
	GtkWidget *button;
	GtkStockItem item;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkWidget *align;

	button = gtk_button_new ();

 	if (GTK_BIN (button)->child)
    		gtk_container_remove (GTK_CONTAINER (button),
				      GTK_BIN (button)->child);

  	if (gtk_stock_lookup (stock_id, &item))
    	{
      		label = gtk_label_new_with_mnemonic (text);

		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
      
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      		hbox = gtk_hbox_new (FALSE, 2);

      		align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
      		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      		gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      
      		gtk_container_add (GTK_CONTAINER (button), align);
      		gtk_container_add (GTK_CONTAINER (align), hbox);
      		gtk_widget_show_all (align);

      		return button;
    	}

      	label = gtk_label_new_with_mnemonic (text);
      	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
  
  	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  	gtk_widget_show (label);
  	gtk_container_add (GTK_CONTAINER (button), label);

	return button;
}

