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
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "view.h"
#include "commands.h"
#include "document.h"
#include "print.h"
#include "menus.h"
#include "prefs.h"
#include "search.h"
#include "utils.h"
#include "plugin.h"
#include "recent.h"
#include "../pixmaps/gedit-icon.xpm"

extern GtkWidget *col_label;

GList *window_list = NULL;

/* I am rewriting the search option, and search results from all
   documents will not get imlemented initially */
#define ENABLE_SEARCH_RESULT_CLIST
#undef  ENABLE_SEARCH_RESULT_CLIST


/* Window *window; */

/* Prototype for setting the window icon */
static void gedit_window_set_icon (GtkWidget *window, char *icon);
GtkWindow * gedit_window_active (void);
GnomeApp  * gedit_window_active_app (void);


GtkWindow *
gedit_window_active (void)
{
	return GTK_WINDOW (mdi->active_window);
}

GnomeApp *
gedit_window_active_app (void)
{
	return mdi->active_window;
}


#ifdef ENABLE_SEARCH_RESULT_CLIST

/* Create a find in all files search result window but don't show it. */
static GtkWidget*
create_find_in_files_result_window (void)
{
	GtkWidget *wnd, *btn, *top, *frame, *image, *label, *hsep;
	gchar *titles[] = {
		N_("Filename"),
		N_("Line"),
		N_("Contents") 
	};
	int i;
	
	search_result_clist = gtk_clist_new_with_titles (3, titles);
	gtk_signal_connect (GTK_OBJECT(search_result_clist), 
			    "select_row",
			    GTK_SIGNAL_FUNC(search_result_clist_cb),
			    NULL);

	for (i = 0; i < 3; i++) 
		gtk_clist_set_column_auto_resize ((GtkCList *)search_result_clist, i, TRUE);

	wnd = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy ((GtkScrolledWindow *)wnd,
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(wnd),
					       search_result_clist); 
	gtk_widget_show (search_result_clist);
	gtk_widget_show (wnd);

	btn = gtk_button_new ();

	/*image = gnome_pixmap_new_from_file_at_size ("../xpm/tb_cancel.xpm", 15, 15);*/
	image = gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_CLOSE);
	
	gtk_container_add (GTK_CONTAINER (btn), image);

	gtk_button_set_relief (GTK_BUTTON(btn), GTK_RELIEF_NONE);
	
	gtk_signal_connect (GTK_OBJECT(btn), "clicked",
			    GTK_SIGNAL_FUNC(remove_search_result_cb),
			    NULL);

	gtk_widget_show (image);
	gtk_widget_show (btn);
	
	frame = gtk_vbox_new (FALSE, 0);
	top = gtk_hbox_new (FALSE, 0);	
	
	label = gtk_label_new ("Search Results");

	gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	hsep = gtk_hseparator_new ();

	gtk_box_pack_start (GTK_BOX (top), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (top), hsep, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (top), btn, FALSE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX (frame), top, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (frame), wnd, TRUE, TRUE, 0);

	gtk_widget_show (hsep);
	gtk_widget_show (label);
	gtk_widget_show (top);

	return frame;
}
#endif

void
gedit_window_new (GnomeMDI *mdi, GnomeApp *app)
{
	GtkWidget *statusbar;
	
	static GtkTargetEntry drag_types[] =
	{
		{ "text/uri-list", 0, 0 },
	};

	static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);

	gtk_drag_dest_set (GTK_WIDGET(app),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drag_types, n_drag_types,
			   GDK_ACTION_COPY);
		
	gtk_signal_connect (GTK_OBJECT (app), "drag_data_received",
			    GTK_SIGNAL_FUNC (filenames_dropped), NULL);

	gedit_window_set_icon (GTK_WIDGET (app), "gedit_icon");

	gtk_window_set_default_size (GTK_WINDOW(app), settings->width, settings->height);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	/*gedit_load_settings ();*/
	
	/* find in files result window  dont show it.*/
#ifdef ENABLE_SEARCH_RESULT_CLIST
	
	search_result_window = create_find_in_files_result_window ();

	gtk_box_pack_start (GTK_BOX (app->vbox), search_result_window,
			    TRUE, TRUE, 0);
#endif
	

	/* gtk_widget_hide(search_result_window); */


	gedit_plugins_window_add (app);
	
	settings->num_recent = 0;
	recent_update (GNOME_APP (app));

	/* statusbar */
	if (settings->show_status)
	{
		statusbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
		gnome_app_set_statusbar (GNOME_APP(app), GTK_WIDGET (statusbar));
		gnome_app_install_menu_hints (app, gnome_mdi_get_menubar_info(app));
	}
}

void
gedit_window_set_auto_indent (gint auto_indent)
{
	settings->auto_indent = auto_indent;
}

/* set the a window icon */
static void
gedit_window_set_icon (GtkWidget *window, char *icon)
{
	GdkPixmap *pixmap;
        GdkBitmap *mask;

	gtk_widget_realize (window);
	
	pixmap = gdk_pixmap_create_from_xpm_d (window->window, &mask,
					       &window->style->bg[GTK_STATE_NORMAL],
					       (char **)gedit_icon_xpm);
	
	gdk_window_set_icon (window->window, NULL, pixmap, mask);
	
	/* Not sure about this.. need to test in E 
	gtk_widget_unrealize (window);*/
}

void
gedit_window_set_status_bar (gint show_status)
{
	if (show_status && GTK_WIDGET_MAPPED (mdi->active_window->statusbar))
		return;

	if (!mdi->active_window->statusbar)
	{
		GtkWidget *statusbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
		
		gnome_app_set_statusbar (GNOME_APP (mdi->active_window),
					 GTK_WIDGET (statusbar));
		gnome_app_install_menu_hints (GNOME_APP (mdi->active_window),
					      gnome_mdi_get_menubar_info (mdi->active_window));

		mdi->active_window->statusbar = statusbar;
	}
	else if (mdi->active_window->statusbar->parent)
	{
		gtk_widget_ref (mdi->active_window->statusbar);
		gtk_container_remove (GTK_CONTAINER (mdi->active_window->statusbar->parent),
				      mdi->active_window->statusbar);
	}
	else
	{
		gtk_box_pack_start (GTK_BOX (mdi->active_window->vbox),
				    mdi->active_window->statusbar,
				    FALSE, FALSE, 0);
		gtk_widget_unref (GNOME_APP (mdi->active_window)->statusbar);
	}
}


/**
 * doc_swaphc_cb:
 * 
 * if .c file is open, then open the related .h file
 *
 * TODO: if a .h file is open, do we swap to a .c or a .cpp?  we
 * should put a check in there.  if both exist, then probably open
 * both files.
 **/
void
doc_swaphc_cb (GtkWidget *widget, gpointer data)
{
	size_t len;
	char *newfname;
	Document *doc;

	gedit_debug ("NO MAMES !", DEBUG_FILE);
	
	doc = gedit_document_current();
	if (!doc || !doc->filename)
		return;

	newfname = NULL;
	len = strlen (doc->filename);
	
	while (len)
	{
		if (doc->filename[len] == '.')
			break;
		len--;
	}

	len++;
	if (doc->filename[len] == 'h')
	{
		newfname = g_strdup (doc->filename);
		newfname[len] = 'c';
	}
	else if (doc->filename[len] == 'H')
	{
		newfname = g_strdup (doc->filename);
		newfname[len] = 'C';
	}
	else if (doc->filename[len] == 'c')
	{
		newfname = g_strdup (doc->filename);
		newfname[len] = 'h';

		if (len < strlen(doc->filename) && strcmp(doc->filename + len, "cpp") == 0)
			newfname[len+1] = '\0';
	}
	else if (doc->filename[len] == 'C')
	{
		newfname = g_strdup (doc->filename);

		if (len < strlen(doc->filename) && strcmp(doc->filename + len, "CPP") == 0)
		{
			newfname[len] = 'H';
			newfname[len+1] = '\0';
		}
		else
			newfname[len] = 'H';
	}

	if (!newfname)
		return;

	/* FIXME: Give a warning dialog that says the file doesn't
	 *        exist */
	if (!g_file_exists (newfname))
		return;

	/* hmm maybe whe should check if the file exist before we try
	 * to open.  this will be fixed later....  */
	doc = gedit_document_new_with_file (newfname);
}


/* Unused functions */
#if 0

static gint
gedit_destroy_window (GtkWidget *widget, GdkEvent *event, gedit_data *data)
{
	window_close_cb (widget, data);
	return TRUE;
}

#endif 
