/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * time.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <gconf/gconf-client.h>

#include <time.h>

#include <gedit-plugin.h>
#include <gedit-debug.h>
#include <gedit-menus.h>

#define MENU_ITEM_LABEL		N_("_Insert Date/Time")
#define MENU_ITEM_PATH		"/menu/Edit/EditOps_4/"
#define MENU_ITEM_NAME		"PluginInsertDateAndTime"	
#define MENU_ITEM_TIP		N_("Insert current date and time at the cursor position.")

#define TIME_BASE_KEY 	"/apps/gedit2/plugins/time"
#define SEL_FORMAT_KEY	"/sel-format"


enum
{
  COLUMN_FORMATS,
  COLUMN_INDEX,
  NUM_COLUMNS
};

typedef struct _TimeConfigureDialog TimeConfigureDialog;

struct _TimeConfigureDialog {
	GtkWidget *dialog;

	GtkWidget *list;
};

static gchar *formats[] =
{
	"%c ",
	"%x ",
	"%X ",
	"%x %X ",
	"%a %b %d %H:%M:%S %Z %Y ",
	"%a %b %d %H:%M:%S %Y ",
	"%a %d %b %Y %H:%M:%S %Z ",
	"%a %d %b %Y %H:%M:%S ",
	"%d/%m/%Y ",
	"%d/%m/%y ",
	"%D ",
	"%A %d %B %Y ",
	"%A %B %d %Y ",
	"%Y-%m-%d ",
	"%d %B %Y ",
	"%B %d, %Y ",
	"%A %b %d ",
	"%H:%M:%S ",
	"%H:%M ",
	"%I:%M:%S %p ",
	"%I:%M %p ",
	"%H.%M.%S ",
	"%H.%M ",
	"%I.%M.%S %p ",
	"%I.%M %p ",
	"%d/%m/%Y %H:%M:%S ",
	"%d/%m/%y %H:%M:%S ",
	NULL
};

static gint 		 sel_format 		= 0;
static GConfClient 	*time_gconf_client 	= NULL;	


static char 			*get_time (const gchar* format);
static void			 dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static GtkTreeModel		*create_model (TimeConfigureDialog *dialog);
static void 			 scroll_to_selected (GtkTreeView *tree_view);
static void			 create_formats_list (TimeConfigureDialog *dialog);
static TimeConfigureDialog 	*get_configure_dialog (GtkWindow* parent);
static void			 time_world_cb (BonoboUIComponent *uic, gpointer user_data, 
						const gchar* verbname);
static void			 ok_button_pressed (TimeConfigureDialog *dialog);
static void			 help_button_pressed (TimeConfigureDialog *dialog);

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState configure (GeditPlugin *p, GtkWidget *parent);
G_MODULE_EXPORT GeditPluginState save_settings (GeditPlugin *pd);


static char *
get_time (const gchar* format)
{
  	gchar *out = NULL;
  	time_t clock;
  	struct tm *now;
  	size_t out_length = 0;
  	
	gedit_debug (DEBUG_PLUGINS, "");

  	clock = time (NULL);
  	now = localtime (&clock);
	  	
	do
	{
		out_length += 255;
		out = g_realloc (out, out_length);
	}
  	while (strftime (out, out_length, format, now) == 0);
  	
  	return out;
}

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}

	gedit_debug (DEBUG_PLUGINS, "END");
	
}

static GtkTreeModel*
create_model (TimeConfigureDialog *dialog)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS, "");

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	/* Set tree view model*/
	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->list), GTK_TREE_MODEL (store));
	
	/* add data to the list store */
	while (formats[i] != NULL)
	{
		gchar* str;

		str = get_time (formats[i]);
		
		gedit_debug (DEBUG_PLUGINS, "%d : %s", i, str);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_FORMATS, str,
				    COLUMN_INDEX, i,
				    -1);
		g_free (str);	
		
		if (i == sel_format)
		{
			GtkTreeSelection *selection;
						
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->list));
			g_return_val_if_fail (selection != NULL, GTK_TREE_MODEL (store));
			gtk_tree_selection_select_iter (selection, &iter);
		}	

		++i;
	}
	
	return GTK_TREE_MODEL (store);
}

static void 
scroll_to_selected (GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (tree_view);
	g_return_if_fail (model != NULL);

	/* Scroll to selected */
	selection = gtk_tree_view_get_selection (tree_view);
	g_return_if_fail (selection != NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GtkTreePath* path;

		path = gtk_tree_model_get_path (model, &iter);
		g_return_if_fail (path != NULL);

		gtk_tree_view_scroll_to_cell (tree_view,
					      path, NULL, TRUE, 1.0, 0.0);
		gtk_tree_path_free (path);
	}
}

static void
create_formats_list (TimeConfigureDialog *dialog)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (dialog != NULL);

	create_model (dialog);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dialog->list), TRUE);

	
	/* the Available formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Available formats"), cell, 
			"text", COLUMN_FORMATS, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->list), column);

	g_signal_connect (G_OBJECT (dialog->list), "realize", 
			G_CALLBACK (scroll_to_selected), NULL);

	gtk_widget_show (dialog->list);
}


static TimeConfigureDialog*
get_configure_dialog (GtkWindow* parent)
{
	static TimeConfigureDialog *dialog = NULL;

	GladeXML *gui;
	GtkWidget *content;
	GtkWidget *viewport;

	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				parent);
		
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "time.glade2",
			     "time_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find time.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (TimeConfigureDialog, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Configure insert date/time plugin"),
						      parent,
						      GTK_DIALOG_DESTROY_WITH_PARENT |
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_OK,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content		= glade_xml_get_widget (gui, "time_dialog_content");
	viewport 	= glade_xml_get_widget (gui, "formats_viewport");
	dialog->list 	= glade_xml_get_widget (gui, "formats_tree");

	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (viewport != NULL, NULL);
	g_return_val_if_fail (dialog->list != NULL, NULL);

	create_formats_list (dialog);
	
	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 10, 200);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	
	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);
	
	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;
}

static void
time_world_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
	GeditView *view;
	gchar *the_time;
	
	gedit_debug (DEBUG_PLUGINS, "");

	view = gedit_get_active_view ();
	g_return_if_fail (view != NULL);
	
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	the_time = get_time (formats[sel_format]);

	gedit_document_begin_user_action (doc);
	
	gedit_document_insert_text_at_cursor (doc, the_time, -1);

	gedit_document_end_user_action (doc);
}

static void
ok_button_pressed (TimeConfigureDialog *dialog)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->list));
	g_return_if_fail (model != NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->list));
	g_return_if_fail (selection != NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, COLUMN_INDEX, &sel_format, -1);
	}

	gedit_debug (DEBUG_PLUGINS, "Sel: %d", sel_format);
}

static void
help_button_pressed (TimeConfigureDialog *dialog)
{
	gedit_debug (DEBUG_PLUGINS, "");

	/* FIXME */
}

G_MODULE_EXPORT GeditPluginState
configure (GeditPlugin *p, GtkWidget *parent)
{
	TimeConfigureDialog *dialog;
	gint ret;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	dialog = get_configure_dialog (GTK_WINDOW (parent));

	if (dialog == NULL) 
	{
		g_warning
		    ("Could not create the configure dialog.\n");
		return PLUGIN_ERROR;
	}

	do
	{
		ret = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (ret)
		{
			case GTK_RESPONSE_OK:
				gedit_debug (DEBUG_PLUGINS, "Ok button pressed");
				ok_button_pressed (dialog);
				break;
			case GTK_RESPONSE_HELP:
				gedit_debug (DEBUG_PLUGINS, "Help button pressed");
				help_button_pressed (dialog);
				break;
			default:
				gedit_debug (DEBUG_PLUGINS, "Default");
		}

	} while (ret == GTK_RESPONSE_HELP);

	gedit_debug (DEBUG_PLUGINS, "Destroying dialog");

	gtk_widget_destroy (dialog->dialog);

	gedit_debug (DEBUG_PLUGINS, "Done");

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || (gedit_document_is_readonly (doc)))		
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);
	else
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	plugin->deactivate (plugin);

	g_object_unref (G_OBJECT (time_gconf_client));

	return PLUGIN_OK;
}
	
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList *top_windows;
        gedit_debug (DEBUG_PLUGINS, "");

        top_windows = gedit_get_top_windows ();
        g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME,
				     MENU_ITEM_LABEL, MENU_ITEM_TIP,
				     "gnome-stock-timer", time_world_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }
       
	
	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
save_settings (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (time_gconf_client != NULL, PLUGIN_ERROR);
	
	gconf_client_set_int (
			time_gconf_client,
			TIME_BASE_KEY SEL_FORMAT_KEY,
			sel_format,
		      	NULL);

	gconf_client_suggest_sync (time_gconf_client, NULL);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->name = _("Insert Date/Time");
	pd->desc = _("Inserts the current date and time at the cursor position.");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->copyright = _("Copyright (C) 2002 - Paolo Maggi");
	
	pd->private_data = NULL;
		
	time_gconf_client = gconf_client_get_default ();
	g_return_val_if_fail (time_gconf_client != NULL, PLUGIN_ERROR);

	gconf_client_add_dir (time_gconf_client,
			      TIME_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);
	
	sel_format = gconf_client_get_int (
				time_gconf_client,
				TIME_BASE_KEY SEL_FORMAT_KEY,
			      	NULL);

	return PLUGIN_OK;
}




