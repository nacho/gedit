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

#include <time.h>

#include "gedit-plugin.h"
#include "gedit-debug.h"

#define MENU_ITEM_LABEL		N_("_Insert Date/Time")
#define MENU_ITEM_PATH		"/menu/Edit/EditOps_4/"
#define MENU_ITEM_NAME		"PluginInsertDateAndTime"	
#define MENU_ITEM_TIP		N_("Insert current date and time at the cursor position.")

enum
{
  COLUMN_FORMATS,
  NUM_COLUMNS
};

typedef struct _TimeConfigureDialog TimeConfigureDialog;

struct _TimeConfigureDialog {
	GtkWidget *dialog;

	GtkWidget *list;
};

static gchar *formats[] =
{
	"%c",
	"%x",
	"%X",
	"%x %X",
	"%a %b %d %H:%M:%S %Z %Y",
	"%a %b %d %H:%M:%S %Y",
	"%d/%m/%Y",
	"%m/%d/%Y",
	"%A %d %B %Y",
	"%Y-%m-%d",
	"%d %B %Y",
	"%A %b %d",
	"%H:%M:%S",
	"%H:%M",
	"%I:%M:%S %p",
	"%I:%M %p",
	"%H.%M.%S",
	"%H.%M",
	"%I.%M.%S %p",
	"%I.%M %p",
	"%d/%m/%Y %H:%M:%S",
	"%d/%m/%y %H:%M:%S",
	NULL
};

const gchar* sel_format = "%d/%m/%Y %H:%M:%S";
	
/* Gratiously ripped out of GIMP (app/general.c), with a fiew minor changes */
static char *
get_time (const gchar* format)
{
	static char static_buf[21];
  	gchar *tmp, *out = NULL;
  	time_t clock;
  	struct tm *now;
  	size_t out_length = 0;
  	
	gedit_debug (DEBUG_PLUGINS, "");

  	clock = time (NULL);
  	/*now = gmtime (&clock);*/
  	now = localtime (&clock);
	  	
  	tmp = static_buf;
	
  	/* date format derived from ISO 8601:1988 */
  	/*sprintf(tmp, "%04d-%02d-%02d%c%02d:%02d:%02d%c",
	  now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
	  ' ',
	  now->tm_hour, now->tm_min, now->tm_sec,
	  '\000'); 
	
	  return tmp;
	*/

	do
	{
		out_length += 200;
		out = (char *) realloc (out, out_length);
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
}

static GtkTreeModel*
create_model (void)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS, "");

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING);

	/* add data to the list store */
	while (formats[i] != NULL)
	{
		gchar* str;

		str = get_time (formats[i]);
		
		gedit_debug (DEBUG_PLUGINS, "%d : %s", i, str);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_FORMATS, str,
				    -1);
		g_free (str);

		++i;
	}
	
	return GTK_TREE_MODEL (store);
}

static void
create_formats_list (TimeConfigureDialog *dialog)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkTreeModel *model;
	GtkWidget *button;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (dialog != NULL);

	model = create_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->list), model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dialog->list), FALSE);

	g_object_unref (G_OBJECT (model));

	/* the First column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Available formats"), cell, 
			"text", COLUMN_FORMATS, NULL);
	/*
	gtk_tree_view_column_set_cell_data_func (column, cell, dialog_plugin_manager_view_cell_cb,
						 dialog, NULL);
	*/
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->list), column);

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

	gtk_signal_connect(GTK_OBJECT (dialog->dialog), "destroy",
			   GTK_SIGNAL_FUNC (dialog_destroyed), &dialog);

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

	the_time = get_time (sel_format);

	gedit_document_begin_user_action (doc);
	
	gedit_document_insert_text_at_cursor (doc, the_time, -1);

	gedit_document_end_user_action (doc);
}

G_MODULE_EXPORT GeditPluginState
configure (GeditPlugin *p, GtkWidget *parent)
{
	TimeConfigureDialog *dialog;

	gedit_debug (DEBUG_PLUGINS, "");

	
	dialog = get_configure_dialog (GTK_WINDOW (parent));

	if (dialog == NULL) 
	{
		g_warning
		    ("Could not create the configure dialog.\n");
		return PLUGIN_ERROR;
	}

	gtk_dialog_run (GTK_DIALOG (dialog->dialog));

	gtk_widget_destroy (dialog->dialog);
}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIEngine *ui_engine;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	ui_engine = bonobo_window_get_ui_engine (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || (gedit_document_is_readonly (doc)))		
		gedit_menus_set_verb_sensitive (ui_engine, "/commands/" MENU_ITEM_NAME, FALSE);
	else
		gedit_menus_set_verb_sensitive (ui_engine, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");
}
	
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList* top_windows;
	
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);
       
	while (top_windows)
	{
		BonoboUIComponent* ui_component;
		gchar *item_path;
		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
		
		item_path = g_strdup (MENU_ITEM_PATH MENU_ITEM_NAME);

		if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			gchar *xml;
					
			xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"\""
					" _label=\"%s\" _tip=\"%s\" hidden=\"0\" />", 
					MENU_ITEM_NAME, MENU_ITEM_LABEL, MENU_ITEM_TIP);

			bonobo_ui_component_set_translate (ui_component, 
					MENU_ITEM_PATH, xml, NULL);

			bonobo_ui_component_set_translate (ui_component, 
					"/commands/", "<cmd name = \"" MENU_ITEM_NAME "\" />", NULL);

			bonobo_ui_component_add_verb (ui_component, 
					MENU_ITEM_NAME, time_world_cb, 
					      NULL); 

			g_free (xml);
		}
		
		g_free (item_path);

		pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));
		
		top_windows = g_list_next (top_windows);
	}
	
	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	GList* top_windows;
	
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);
       
	while (top_windows)
	{
		BonoboUIComponent* ui_component;
		gchar *item_path;
		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
		
		item_path = g_strdup (MENU_ITEM_PATH MENU_ITEM_NAME);

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			bonobo_ui_component_rm (ui_component, item_path, NULL);
			bonobo_ui_component_rm (ui_component, "/commands/" MENU_ITEM_NAME, NULL);
		}
		
		g_free (item_path);
		
		top_windows = g_list_next (top_windows);
	}
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->name = _("Insert Date/Time");
	pd->desc = _("Inserts the current date and time at the cursor position.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->copyright = _("Copyright (C) 2002 - Alex Roberts");
	
	pd->private_data = NULL;
		
	return PLUGIN_OK;
}




