/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
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
 * Prefs glade-ized and rewritten by Jason Leach <leach@wam.umd.edu>
 */

/* TODO:
 * [ ] I think we're leaking mem with setting styles and whatnot.
 *
 */

#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "prefs.h"
#include "view.h"
#include "dialogs.h"

static GtkWidget *propertybox;

static GtkWidget *statusbar;
static GtkWidget *splitscreen;
static GtkWidget *autoindent;
static GtkWidget *wordwrap;

static GtkWidget *mdimode;
static GtkWidget *tabpos;

static GtkWidget *foreground;
static GtkWidget *background;
static GtkWidget *defaultfont;


static gint
gtk_option_menu_get_active_index (GtkWidget *omenu)
{
	GtkMenu *menu;
	GtkWidget *active;
	GtkWidget *child;
	GList *children;
	gint i = 0;


	menu = GTK_MENU (gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu)));
	active = gtk_menu_get_active (menu);
	
	child = NULL;
	children = GTK_MENU_SHELL (menu)->children;
	while (children)
	{
		child = children->data;
		children = children->next;

		if (child == active)
			return i;
		else
			i++;
	}

	return 0;
}

static void
gedit_window_refresh (void)
{
	gint i, j;
	View *nth_view;
	Document *doc;
	GtkStyle *style;
	GdkColor *bg, *fg;

	gedit_view_set_split_screen (VIEW (mdi->active_view),
				     (gint) VIEW (mdi->active_view)->splitscreen);

	gedit_window_set_status_bar (settings->show_status);

	/* the "Default" mode is index 3, but GNOME_MDI_DEFAULT_MODE
           is for some silly reason defined as 42 in gnome-mdi.h */
	if (settings->mdi_mode != 3)
		gnome_mdi_set_mode (mdi, settings->mdi_mode);
	else
		gnome_mdi_set_mode (mdi, GNOME_MDI_DEFAULT_MODE);

	tab_pos (settings->tab_pos);

	style = gtk_style_copy (gtk_widget_get_style (VIEW (mdi->active_view)->text));

	bg = &style->base[0];
	bg->red = settings->bg[0];
	bg->green = settings->bg[1];
	bg->blue = settings->bg[2];

	fg = &style->text[0];
	fg->red = settings->fg[0];
	fg->green = settings->fg[1];
	fg->blue = settings->fg[2];

	for (i = 0; i < g_list_length (mdi->children); i++)
	{
		doc = DOCUMENT (g_list_nth_data (mdi->children, i));
  	
		for (j = 0; j < g_list_length (doc->views); j++)
		{
			nth_view = g_list_nth_data (doc->views, j);
			gedit_view_set_word_wrap (nth_view, settings->word_wrap);

			gtk_widget_set_style (GTK_WIDGET (nth_view->split_screen),
					      style);
			gtk_widget_set_style (GTK_WIDGET (nth_view->text),
					      style);
			gedit_view_set_font (nth_view, settings->font);
		}
	}
}

static void
destroy_cb (void)
{
	gtk_widget_destroy (GTK_WIDGET (propertybox));
}

static void
help_cb (void)
{
	GnomeHelpMenuEntry help_entry = { NULL, "properties.html" };

	help_entry.name = gnome_app_id;
	gnome_help_display (NULL, &help_entry);
}

static void
apply_cb (GnomePropertyBox *pbox, gint page, gpointer data)
{
	GtkStyle *style;
	GdkColor *c;

	if (page == -1)
		return;

	settings->auto_indent = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (autoindent));
	settings->show_status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (statusbar));
	settings->splitscreen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (splitscreen));
	settings->word_wrap = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wordwrap));

	settings->font = g_strdup (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (defaultfont)));

	settings->mdi_mode = gtk_option_menu_get_active_index (mdimode);
	settings->mdi_mode = settings->mdi_mode != 3 ? settings->mdi_mode : GNOME_MDI_DEFAULT_MODE;

	settings->tab_pos = gtk_option_menu_get_active_index (tabpos);

        style = gtk_style_new ();
        
        c = &style->base[0];
        
        gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (background),
                                    &c->red, &c->green, &c->blue, 0);
        settings->bg[0] = c->red;
        settings->bg[1] = c->green;
        settings->bg[2] = c->blue;
        
        c = &style->text[0];
        
        gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (foreground),
                                    &c->red, &c->green, &c->blue, 0);
        settings->fg[0] = c->red;
        settings->fg[1] = c->green;
        settings->fg[2] = c->blue;

	gedit_window_refresh ();
	gedit_save_settings ();
}

static void
prefs_changed (GtkWidget *widget, gpointer data)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (propertybox));
}

static void
prepare_general_page (GladeXML *gui)
{
	statusbar = glade_xml_get_widget (gui, "statusbar");
	splitscreen = glade_xml_get_widget (gui, "splitscreen");
	autoindent = glade_xml_get_widget (gui, "autoindent");
	wordwrap = glade_xml_get_widget (gui, "wordwrap");

	/* set initial button status */
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (statusbar),
				     settings->show_status);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (splitscreen),
				     settings->splitscreen);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (autoindent),
				     settings->auto_indent);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (wordwrap),
				     settings->word_wrap);

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (statusbar), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (splitscreen), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (autoindent), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (wordwrap), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
}

static void
prepare_documents_page (GladeXML *gui)
{
	mdimode = glade_xml_get_widget (gui, "mdimode");
	tabpos = glade_xml_get_widget (gui, "tabpos");

	/* set initial states */
	gtk_option_menu_set_history (GTK_OPTION_MENU (mdimode),
				     settings->mdi_mode);
	gtk_option_menu_set_history (GTK_OPTION_MENU (tabpos),
				     settings->tab_pos);

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (GTK_MENU_SHELL (gtk_option_menu_get_menu(GTK_OPTION_MENU (mdimode)))),
			    "selection-done",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (GTK_MENU_SHELL (gtk_option_menu_get_menu(GTK_OPTION_MENU (tabpos)))),
			    "selection-done",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
}

static void
prepare_fontscolors_page (GladeXML *gui)
{
	GtkStyle *style;
	GdkColor *c;

	foreground = glade_xml_get_widget (gui, "foreground");
	background = glade_xml_get_widget (gui, "background");
	defaultfont = glade_xml_get_widget (gui, "defaultfont");

	/* setup the initial states */
	if (mdi->active_view)
		style = gtk_style_copy (gtk_widget_get_style (VIEW (mdi->active_view)->text));
	else
		style = gtk_style_new ();

	c = &style->base[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (background),
				    c->red, c->green, c->blue, 0);

	c = &style->text[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (foreground),
				    c->red, c->green, c->blue, 0);

	if (settings->font)
		gnome_font_picker_set_font_name (GNOME_FONT_PICKER (defaultfont),
						 settings->font);

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (foreground), "color_set",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (background), "color_set",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (defaultfont), "font_set",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
}

static void
dialog_prefs_impl (GladeXML *gui)
{
	propertybox = glade_xml_get_widget (gui, "propertybox");

	/* glade won't let me set the title of the propertybox */
	gtk_window_set_title (GTK_WINDOW (propertybox), _("gedit: Preferences"));

	prepare_general_page (gui);
	prepare_documents_page (gui);
	prepare_fontscolors_page (gui);

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (propertybox),
			    "apply",
			    GTK_SIGNAL_FUNC (apply_cb), NULL);

	gtk_signal_connect (GTK_OBJECT (propertybox),
			    "destroy",
			    GTK_SIGNAL_FUNC (destroy_cb), NULL);

	gtk_signal_connect (GTK_OBJECT (propertybox),
			    "delete_event",
			    GTK_SIGNAL_FUNC (gtk_false), NULL);

	gtk_signal_connect (GTK_OBJECT (propertybox),
			    "help",
			    GTK_SIGNAL_FUNC (help_cb), NULL);

	/* show everything */
	gtk_widget_show_all (propertybox);
}

void
dialog_prefs (void)
{
	GladeXML *gui;

	gui = glade_xml_new (GEDIT_GLADEDIR "/prefs.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find prefs.glade\n");
		return;
	}

	dialog_prefs_impl (gui);
	gtk_object_unref (GTK_OBJECT (gui));
}
