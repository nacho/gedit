/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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
 */

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "main.h"
#include "gE_window.h"
#include "gE_view.h"
#include "gE_files.h"
#include "gE_prefs_box.h"
#include "gE_plugin_api.h"
#include "commands.h"
#include "gE_mdi.h"
#include "gE_print.h"
#include "menus.h"
/*#include "toolbar.h"*/
#include "gE_prefs.h"
/*#include "search.h"*/
#include "gE_icon.xpm"

extern GList *plugins;
gE_window *window;
extern GtkWidget  *col_label;

/* Prototype for setting the window icon */
void gE_window_set_icon(GtkWidget *window, char *icon);


/*gE_window */
void gE_window_new(GnomeMDI *mdi, GnomeApp *app)
{
	GtkWidget *statusbar;
	
	static GtkTargetEntry drag_types[] =
	{
		{ "text/uri-list", 0, 0 },
	};
	static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);


	gtk_window_set_default_size (GTK_WINDOW(app), settings->width, settings->height);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	gE_get_settings ();

	settings->num_recent = 0;
	recent_update(GNOME_APP(app));

	gtk_drag_dest_set (GTK_WIDGET(app),
		GTK_DEST_DEFAULT_ALL,
		drag_types, n_drag_types,
		GDK_ACTION_COPY);
		
	gtk_signal_connect (GTK_OBJECT (app),
		"drag_data_received",
		GTK_SIGNAL_FUNC (filenames_dropped), NULL);

	/* statusbar */
	statusbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP(app),GTK_WIDGET (statusbar));
		
	gnome_app_install_menu_hints(app, gnome_mdi_get_menubar_info(app));
	
	gE_window_set_icon(GTK_WIDGET(app), "gE_icon");
	
} /* gE_window_new */

void gE_window_set_auto_indent (gint auto_indent)
{
	settings->auto_indent = auto_indent;
}

/* set the a window icon */
void gE_window_set_icon(GtkWidget *window, char *icon)
{
	GdkPixmap *pixmap;
        GdkBitmap *mask;

	gtk_widget_realize (window);
	
	pixmap = gdk_pixmap_create_from_xpm_d (window->window,
						&mask,
                                		&window->style->bg[GTK_STATE_NORMAL],
                                		(char **)gE_icon);
	
	gdk_window_set_icon (window->window, NULL, pixmap, mask);
	
	gtk_widget_unrealize (window);
}


void gE_window_set_status_bar (gint show_status)
{
	settings->show_status = show_status;
	if (show_status)
	  gtk_widget_show (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
	else
	  gtk_widget_hide (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
}


void
child_switch (GnomeMDI *mdi, gE_document *doc)
{
	gchar *title;

	if (gE_document_current())
	  {
	    gtk_widget_grab_focus(GE_VIEW(mdi->active_view)->text);
	    title = g_malloc0 (strlen (GEDIT_ID) +
					   strlen (GNOME_MDI_CHILD (gE_document_current())->name) + 4);
	    sprintf (title, "%s - %s",
		   GNOME_MDI_CHILD (gE_document_current())->name,
		   GEDIT_ID);
	    gtk_window_set_title(GTK_WINDOW(mdi->active_window), title);
	    g_free(title);
	  }
	  

}
/*	umm.. FIXME?
static gint
gE_destroy_window (GtkWidget *widget, GdkEvent *event, gE_data *data)
{
	window_close_cb(widget, data);
	return TRUE;
}
*/

/*
 * PRIVATE: doc_swaphc_cb
 *
 * if .c file is open open .h file 
 *
 * TODO: if a .h file is open, do we swap to a .c or a .cpp?  we should put a
 * check in there.  if both exist, then probably open both files.
 */
void
doc_swaphc_cb(GtkWidget *wgt, gpointer cbdata)
{
	size_t len;
	char *newfname;
	gE_document *doc;
	
	doc = gE_document_current();
	if (!doc || !doc->filename)
		return;

	newfname = NULL;
	len = strlen(doc->filename);
	while (len) {
		if (doc->filename[len] == '.')
			break;
		len--;
	};

	len++;
	if (doc->filename[len] == 'h') {
		newfname = g_strdup(doc->filename);
		newfname[len] = 'c';
	} else if (doc->filename[len] == 'H') {
		newfname = g_strdup(doc->filename);
		newfname[len] = 'C';
	} else if (doc->filename[len] == 'c') {
		newfname = g_strdup(doc->filename);
		newfname[len] = 'h';
		if (len < strlen(doc->filename) &&
			strcmp(doc->filename + len, "cpp") == 0)
			newfname[len+1] = '\0';
	} else if (doc->filename[len] == 'C') {
		newfname = g_strdup(doc->filename);
		if (len < strlen(doc->filename) &&
				strcmp(doc->filename + len, "CPP") == 0) {
			newfname[len] = 'H';
			newfname[len+1] = '\0';
		} else
			newfname[len] = 'H';
	}

	if (!newfname)
		return;

	/* hmm maybe whe should check if the file exist before we try
	 * to open.  this will be fixed later.... */
	doc = gE_document_new_with_file (newfname);
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
} /* doc_swaphc_cb */

/* the end */

#ifdef WITH_GMODULE_PLUGINS

gE_document
*gE_document_new_container(gE_window *w, gchar *title, gint with_split_screen)
{
	gE_document *doc;
	GtkWidget *table, *vscrollbar, *vpaned, *vbox;

	GtkStyle *style;
	gint *ptr; /* For plugin stuff. */

	doc = g_malloc0(sizeof(gE_document));

	ptr = g_new(int, 1);
	*ptr = ++last_assigned_integer;
	g_hash_table_insert (doc_int_to_pointer, ptr, doc);
	g_hash_table_insert (doc_pointer_to_int, doc, ptr);

	doc->window = w;

	if (w->notebook == NULL) {
		w->notebook = gtk_notebook_new ();
		gtk_notebook_set_scrollable (GTK_NOTEBOOK (w->notebook), TRUE);
	}
	
	vpaned = gtk_vbox_new (TRUE, TRUE);
	
	doc->tab_label = gtk_label_new (title);
	GTK_WIDGET_UNSET_FLAGS (doc->tab_label, GTK_CAN_FOCUS);
	doc->filename = NULL;
	doc->word_wrap = TRUE;
	doc->line_wrap = TRUE;
	doc->read_only = FALSE;
	gtk_widget_show (doc->tab_label);

	/* Create the upper split screen */
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
	gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
	gtk_box_pack_start (GTK_BOX (vpaned), table, TRUE, TRUE, 1);
	gtk_widget_show (table);

	/* Create it, but never gtk_widget_show () it. */
	doc->text = gtk_text_new (NULL, NULL);

	doc->viewport = gtk_viewport_new (NULL, NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), doc->viewport, 0, 1, 0, 1);

	vbox = gtk_vbox_new (FALSE, FALSE);

	vscrollbar = gtk_vscrollbar_new
		(GTK_VIEWPORT (doc->viewport)->vadjustment);
	gtk_box_pack_start (GTK_BOX (vbox), vscrollbar, TRUE, TRUE, 0);
	gtk_table_attach(GTK_TABLE(table), vbox, 1, 2, 0, 1,
			 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	GTK_WIDGET_UNSET_FLAGS (vscrollbar, GTK_CAN_FOCUS);
	gtk_widget_show (vscrollbar);
	gtk_widget_show (vbox);

	doc->split_screen = gtk_text_new (NULL, NULL);

	if (with_split_screen) {
		/* Create the bottom split screen */
		table = gtk_table_new (2, 2, FALSE);
		gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
		gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
		
		gtk_box_pack_start (GTK_BOX (vpaned), table, TRUE, TRUE, 1);
		gtk_widget_show (table);

		doc->split_viewport = gtk_viewport_new (NULL, NULL);
		gtk_table_attach_defaults (GTK_TABLE (table),
					   doc->split_viewport,
					   0, 1, 0, 1);
		doc->split_parent = GTK_WIDGET (doc->split_viewport)->parent;

		vscrollbar = gtk_vscrollbar_new
			(GTK_VIEWPORT (doc->split_viewport)->vadjustment);

		gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
				  GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

		GTK_WIDGET_UNSET_FLAGS (vscrollbar, GTK_CAN_FOCUS);
		gtk_widget_show (vscrollbar);
		gtk_widget_hide (GTK_WIDGET (doc->split_viewport)->parent);
	}
	
	gtk_widget_show (vpaned);
	gtk_notebook_append_page(GTK_NOTEBOOK(w->notebook), vpaned,
				 doc->tab_label);

	w->documents = g_list_append(w->documents, doc);

	gtk_notebook_set_page(GTK_NOTEBOOK(w->notebook),
		g_list_length(GTK_NOTEBOOK(w->notebook)->children) - 1);

	gtk_widget_grab_focus(doc->text);

	return doc;
}

#endif /* WITH_GMODULE_PLUGINS */

