/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose Celorio
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

#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "prefs.h"
#include "view.h"
#include "dialogs.h"
#include "utils.h"
#include "commands.h"
#include "window.h"
#include "print.h"

static GtkWidget *propertybox;
static GtkWidget *statusbar;
#ifdef ENABLE_SPLIT_SCREEN
static GtkWidget *splitscreen;
#endif
static GtkWidget *autoindent;
static GtkWidget *toolbar_radio_button_1;
static GtkWidget *toolbar_radio_button_2;
static GtkWidget *toolbar_radio_button_3;

static GtkWidget *mdimode;
static GtkWidget *tabpos;
static GtkWidget *foreground;
static GtkWidget *background;
static GtkWidget *defaultfont;
static GtkWidget *print_wrap;
static GtkWidget *print_header;
static GtkWidget *print_lines;
static GtkWidget *print_orientation_portrait_radio_button;
static GtkWidget *print_lines_spin_button;
static GtkWidget *lines_label;

static GtkWidget *paperselector;
static GtkWidget *undo_levels;
static GtkWidget *undo_levels_spin_button;
static GtkWidget *undo_levels_label;

static gint
gtk_option_menu_get_active_index (GtkWidget *omenu)
{
	GtkMenu *menu;
	GtkWidget *active;
	GtkWidget *child;
	GList *children;
	gint i = 0;

	gedit_debug(DEBUG_PREFS, "");

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
destroy_cb (void)
{
	gedit_debug (DEBUG_PREFS, "");
	gtk_widget_destroy (GTK_WIDGET (propertybox));
	propertybox = NULL;
}

static void
apply_cb (GnomePropertyBox *pbox, gint page, gpointer data)
{
	GtkStyle *style;
	GdkColor *c;
	gint temp_mdi_mode;
	
	gedit_debug (DEBUG_PREFS, "");
	
	if (page == -1)
		return;

	settings->auto_indent = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (autoindent));
	settings->show_status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (statusbar));

	switch (gtk_radio_group_get_selected (GTK_RADIO_BUTTON(toolbar_radio_button_1)->group))
	{
	case 0:
		settings->toolbar_labels = GEDIT_TOOLBAR_SYSTEM;
		break;
	case 1:
		settings->toolbar_labels = GEDIT_TOOLBAR_ICONS;
		break;
	case 2:
		settings->toolbar_labels = GEDIT_TOOLBAR_ICONS_AND_TEXT;
		break;
	default:
		g_return_if_fail (FALSE);
		break;
		
	}

#ifdef ENABLE_SPLIT_SCREEN
	settings->splitscreen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (splitscreen));
#endif

#if 1 /* FIXME We are leaking memory here but it is crashing, I will continue tomorrow. */
	if (settings->font)
		g_free (settings->font);
#endif	
	settings->font = g_strdup (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (defaultfont)));

	if (gtk_radio_group_get_selected (GTK_RADIO_BUTTON(print_orientation_portrait_radio_button)->group)==0)
		settings->print_orientation = PRINT_ORIENT_PORTRAIT;
	else
		settings->print_orientation = PRINT_ORIENT_LANDSCAPE;
				
	settings->print_header = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (print_header));
	settings->print_wrap_lines = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (print_wrap));

	g_free (settings->papersize);
	if (strcmp( "custom", gnome_paper_selector_get_name( GNOME_PAPER_SELECTOR (paperselector)))==0)
	{
		g_warning ("Custom paper sizes not yet supported. Setting paper size to default\n");
		settings->papersize = strdup (gnome_paper_name_default());
	}
	else
		settings->papersize = strdup (gnome_paper_selector_get_name( GNOME_PAPER_SELECTOR (paperselector)));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (print_lines))) {
		settings->print_lines = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (print_lines_spin_button));
	}
	else
	{
		settings->print_lines = 0;
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (undo_levels)))
		settings->undo_levels = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (undo_levels_spin_button));
	else
		settings->undo_levels = 0;
	
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

	gtk_style_unref (style);

	settings->tab_pos = gtk_option_menu_get_active_index (tabpos);
	temp_mdi_mode = gtk_option_menu_get_active_index (mdimode);
	temp_mdi_mode = temp_mdi_mode != 3 ? temp_mdi_mode : GNOME_MDI_DEFAULT_MODE;

	if (settings->mdi_mode != temp_mdi_mode)
	{
		settings->mdi_mode = temp_mdi_mode;
		gedit_window_refresh_all (TRUE);
	}
	else
	{
		gedit_window_refresh_all (FALSE);
	}
	

	gedit_prefs_save_settings ();

}


static void
gtk_toggle_button_update_label_sensitivity (GtkWidget *widget, gboolean sens)
{
	GList *children;
	GList *l;
	GtkContainer *container;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_CONTAINER (widget));

	container = GTK_CONTAINER (widget);
	children = gtk_container_children (container);

	l = children;
	while (l)
	{
		GtkWidget *child = (GtkWidget*)l->data;

		if (GTK_IS_LABEL (child))
			gtk_widget_set_sensitive (child, sens);

		l = l->next;
	}

	g_list_free (children);
}


static void
prefs_changed (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_PREFS, "");
	gnome_property_box_changed (GNOME_PROPERTY_BOX (propertybox));
}

static void
prepare_general_page (GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	statusbar = glade_xml_get_widget (gui, "statusbar");
#ifdef ENABLE_SPLIT_SCREEN
	splitscreen = glade_xml_get_widget (gui, "splitscreen");
#endif	
	autoindent = glade_xml_get_widget (gui, "autoindent");
	toolbar_radio_button_1 = glade_xml_get_widget (gui, "toolbar_radio_button_1");
	toolbar_radio_button_2 = glade_xml_get_widget (gui, "toolbar_radio_button_2");
	toolbar_radio_button_3 = glade_xml_get_widget (gui, "toolbar_radio_button_3");

	if ( !statusbar ||
#ifdef ENABLE_SPLIT_SCREEN
	     !splitscreen ||
#endif
	     !autoindent ||
	     !toolbar_radio_button_1 ||
	     !toolbar_radio_button_2 ||
	     !toolbar_radio_button_3 )
	{
		g_warning ("Could not load widgets from prefs.glade correctly\n");
		return;
	}
	     

	/* set initial button status */
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (statusbar),
				     settings->show_status);
#ifdef ENABLE_SPLIT_SCREEN
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (splitscreen),
				     settings->splitscreen);
#endif
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (autoindent),
				     settings->auto_indent);

	switch (settings->toolbar_labels)
	{
	case GEDIT_TOOLBAR_SYSTEM:
		gtk_radio_button_select (GTK_RADIO_BUTTON(toolbar_radio_button_1)->group, 0);
		break;
	case GEDIT_TOOLBAR_ICONS:
		gtk_radio_button_select (GTK_RADIO_BUTTON(toolbar_radio_button_1)->group, 1);
		break;
	case GEDIT_TOOLBAR_ICONS_AND_TEXT:
		gtk_radio_button_select (GTK_RADIO_BUTTON(toolbar_radio_button_1)->group, 2);
		break;
	}

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (statusbar), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
#ifdef ENABLE_SPLIT_SCREEN
	gtk_signal_connect (GTK_OBJECT (splitscreen), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
#endif
	gtk_signal_connect (GTK_OBJECT (autoindent), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);

	gtk_signal_connect (GTK_OBJECT(toolbar_radio_button_1), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT(toolbar_radio_button_2), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT(toolbar_radio_button_3), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
}

static void
undo_levels_toggled (GtkWidget *widget, gpointer data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (undo_levels)))
	{
		gtk_toggle_button_update_label_sensitivity (undo_levels, TRUE);

		gtk_widget_set_sensitive (GTK_WIDGET (undo_levels_spin_button), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (undo_levels_label), TRUE);
	}
	else
	{
		gtk_toggle_button_update_label_sensitivity (undo_levels, FALSE);

		gtk_widget_set_sensitive (GTK_WIDGET (undo_levels_spin_button), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (undo_levels_label), FALSE);
	}

	gnome_property_box_changed (GNOME_PROPERTY_BOX (propertybox));
}

static void
prepare_documents_page (GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	mdimode                  = glade_xml_get_widget (gui, "mdimode");
	tabpos                   = glade_xml_get_widget (gui, "tabpos");
	undo_levels              = glade_xml_get_widget (gui, "undo_levels");
	undo_levels_label        = glade_xml_get_widget (gui, "undo_levels_label");
	undo_levels_spin_button  = glade_xml_get_widget (gui, "undo_levels_spin_button");

	if ( !mdimode ||
	     !tabpos  ||
	     !undo_levels ||
	     !undo_levels_label ||
	     !undo_levels_spin_button)
	{
		g_warning ("Could not load widgets from prefs.glade correctly\n");
		return;
	}
	     

	/* set initial button states */
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (undo_levels),
				     settings->undo_levels>0);

	if (settings->undo_levels < 1)
	{
		gtk_toggle_button_update_label_sensitivity (undo_levels, FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (undo_levels_spin_button), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (undo_levels_label), FALSE);
	}
	else
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (undo_levels_spin_button),
					   (guint) settings->undo_levels);

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
	gtk_signal_connect (GTK_OBJECT (undo_levels), "toggled",
			    GTK_SIGNAL_FUNC (undo_levels_toggled), NULL);
	gtk_signal_connect (GTK_OBJECT (GTK_SPIN_BUTTON (undo_levels_spin_button)->adjustment),
			    "value_changed",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
}

static void
prepare_fontscolors_page (GladeXML *gui)
{
	GtkStyle *style;
	GdkColor *c;

	gedit_debug (DEBUG_PREFS, "");
	
	foreground  = glade_xml_get_widget (gui, "foreground");
	background  = glade_xml_get_widget (gui, "background");
	defaultfont = glade_xml_get_widget (gui, "defaultfont");

	/* setup the initial states */
	if (mdi->active_view)
		style = gtk_style_copy (gtk_widget_get_style (GEDIT_VIEW (mdi->active_view)->text));
	else
		style = gtk_style_new ();

	c = &style->base[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (background),
				    c->red, c->green, c->blue, 0);

	c = &style->text[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (foreground),
				    c->red, c->green, c->blue, 0);

	gtk_style_unref (style);
	
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
print_lines_toggled (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_PREFS, "");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (print_lines)))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (print_lines_spin_button), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (lines_label), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (GTK_WIDGET (print_lines_spin_button), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (lines_label), FALSE);
	}

	gnome_property_box_changed (GNOME_PROPERTY_BOX (propertybox));
}

static void
prepare_printing_page (GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	print_header = glade_xml_get_widget (gui, "printheader");
	print_lines  = glade_xml_get_widget (gui, "printlines");
	print_lines_spin_button = glade_xml_get_widget (gui, "printlinesspin");
	lines_label  = glade_xml_get_widget (gui, "lineslabel");
	print_wrap   = glade_xml_get_widget (gui, "printwrap");
	print_orientation_portrait_radio_button = glade_xml_get_widget (gui,
									"print_orientation_portrait_radio_button");

	if ( !print_header ||
	     !print_lines ||
	     !print_lines_spin_button ||
	     !lines_label ||
	     !print_wrap ||
	     !print_orientation_portrait_radio_button)
	{
		g_warning ("Could not load the widgets for the print page.");
		return;
	}

	/* set initial button states */
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (print_header),
				     settings->print_header);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (print_lines),
				     settings->print_lines);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (print_wrap),
				     settings->print_wrap_lines);

	if (settings->print_orientation == PRINT_ORIENT_PORTRAIT)
		gtk_radio_button_select (GTK_RADIO_BUTTON(print_orientation_portrait_radio_button)->group,0);
	else
		gtk_radio_button_select (GTK_RADIO_BUTTON(print_orientation_portrait_radio_button)->group,1);

	if (!settings->print_lines)
	{
		gtk_widget_set_sensitive (GTK_WIDGET (print_lines_spin_button), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (lines_label), FALSE);
	}
	else
		gtk_spin_button_set_value(GTK_SPIN_BUTTON( print_lines_spin_button), (gint)settings->print_lines);
			

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (print_header), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (print_wrap), "toggled",
			    GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (print_lines), "toggled",
			    GTK_SIGNAL_FUNC (print_lines_toggled), NULL);
	gtk_signal_connect (GTK_OBJECT (GTK_SPIN_BUTTON (print_lines_spin_button)->adjustment),
			    "value_changed", GTK_SIGNAL_FUNC (prefs_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (print_orientation_portrait_radio_button),
			    "toggled", GTK_SIGNAL_FUNC (prefs_changed), NULL);
}

static void
prepare_pagesize_page (GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	paperselector  = glade_xml_get_widget (gui, "paperselector");

	gnome_paper_selector_set_name( GNOME_PAPER_SELECTOR (paperselector),
				       settings->papersize);

	/* connect signals */
	gtk_signal_connect(GTK_OBJECT( GTK_COMBO( GNOME_PAPER_SELECTOR(paperselector)->paper)->entry),
			   "changed",
			   GTK_SIGNAL_FUNC (prefs_changed), NULL);
			   
}

#if 0
static void
gedit_prefs_notebook_switch_page (GtkWidget *widget, gpointer data)
{
	g_print ("Page switched\n");
}
#endif

static void
dialog_prefs_impl (GladeXML *gui)
{
	static GnomeHelpMenuEntry help_entry = { NULL, "prefs" };

	gedit_debug (DEBUG_PREFS, "");

	propertybox = glade_xml_get_widget (gui, "propertybox");
#ifdef USE_NOTEBOOK	
	notebook    = glade_xml_get_widget (gui, "notebook");
#endif

	g_return_if_fail (propertybox != NULL);
#ifdef USE_NOTBEOOK	
	g_return_if_fail (notebook    != NULL);
#endif	
	
	/* glade won't let me set the title of the propertybox */
	gtk_window_set_title (GTK_WINDOW (propertybox), _("gedit: Preferences"));

	prepare_general_page (gui);
	prepare_documents_page (gui);
	prepare_fontscolors_page (gui);
	prepare_printing_page (gui);
	prepare_pagesize_page (gui);

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (propertybox),"apply",
			    GTK_SIGNAL_FUNC (apply_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (propertybox),"destroy",
			    GTK_SIGNAL_FUNC (destroy_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (propertybox),"delete_event",
			    GTK_SIGNAL_FUNC (gtk_false), NULL);

	help_entry.name = gnome_app_id;
	gtk_signal_connect (GTK_OBJECT (propertybox),"help",
			    GTK_SIGNAL_FUNC (gnome_help_pbox_display), &help_entry);
#ifdef USE_NOTEBBOK	
	gtk_signal_connect (GTK_OBJECT (notebook),"switch_page",
			    GTK_SIGNAL_FUNC (gedit_prefs_notebook_switch_page), NULL);
#endif

	gnome_dialog_set_parent (GNOME_DIALOG (propertybox),
				 gedit_window_active());

	/* show everything */
	gtk_widget_show_all (propertybox);
}

void
dialog_prefs (void)
{
	static GladeXML *gui = NULL;

	gedit_debug (DEBUG_PREFS, "");

	
	if (!propertybox)
	{
		gui = glade_xml_new (GEDIT_GLADEDIR "prefs.glade", NULL);

		if (!gui)
		{
			g_warning ("Could not find prefs.glade\n");
			return;
		}
		dialog_prefs_impl (gui);
		gtk_object_unref (GTK_OBJECT (gui));
	}
	else
	{
		gdk_window_show (propertybox->window);
		gdk_window_raise (propertybox->window);
	}
}
