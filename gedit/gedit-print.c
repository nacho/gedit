/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-print.c
 * This file is part of gedit
 *
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
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <libgnome/libgnome.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprintui/gnome-print-dialog.h>

#include "gedit2.h"
#include "gedit-print.h"
#include "gedit-debug.h"


static void 	gedit_print_real 	(GeditDocument* doc, gboolean preview);
static gboolean	gedit_print_run_dialog 	(/*PrintJobInfo *pji*/);

/**
 * gedit_print_run_dialog:
 * @pji: 
 * 
 * Run the print dialog 
 * 
 * Return Value: TRUE if the printing was canceled by the user
 **/
static gboolean
gedit_print_run_dialog (/*PrintJobInfo *pji*/)
{
	GtkWidget *dialog;
	guint start_pos;
	guint end_pos;
	gint selection_flag;
/*
	if (!gedit_view_get_selection (pji->view, &start_pos, &end_pos))
		selection_flag = GNOME_PRINT_RANGE_SELECTION_UNSENSITIVE;
	else*/
		selection_flag = GNOME_PRINT_RANGE_SELECTION;
		
	dialog =  gnome_print_dialog_new ((const char *) _("Print Document"),
			          GNOME_PRINT_DIALOG_RANGE | GNOME_PRINT_DIALOG_COPIES);
	
	gnome_print_dialog_construct_range_page ( GNOME_PRINT_DIALOG (dialog),
						  GNOME_PRINT_RANGE_ALL |
						  GNOME_PRINT_RANGE_RANGE |
						  selection_flag,
						  1, /*pji->pages*/5, "A",
							  _("Pages"));

	gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      GTK_WINDOW
				      (bonobo_mdi_get_active_window
				       (BONOBO_MDI (gedit_mdi))));


	gtk_dialog_run (GTK_DIALOG (dialog));
#if 0
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
#endif
	gtk_widget_destroy (dialog);

	return FALSE;
}

/**
 * gedit_print_real:
 * @preview: 
 * 
 * The main printing function
 **/
static void
gedit_print_real (GeditDocument* doc, gboolean preview)
{
	
#if 0
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
#endif
	gedit_print_run_dialog ();
}


void 
gedit_print (GeditMDIChild* active_child)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PRINT, "");
	
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	gedit_print_real (doc, FALSE);
}

void 
gedit_print_preview (GeditMDIChild* active_child)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PRINT, "");
	
	g_return_if_fail (active_child != NULL);

	doc = active_child->document;
	g_return_if_fail (doc != NULL);

	gedit_print_real (doc, TRUE);
}

