/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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

#include <libgnomevfs/gnome-vfs.h>

#include "document.h"
#include "view.h"
#include "prefs.h"
#include "file.h"
#include "utils.h"
#include "window.h"
#include "recent.h"

#ifndef MAX_RECENT
#define MAX_RECENT 4
#endif

#ifndef DATA_ITEMS_TO_REMOVE
#define DATA_ITEMS_TO_REMOVE "items_to_remove"
#endif

       void gedit_recent_update (GnomeApp *app);
static void gedit_recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb (GtkWidget *w, gpointer data);
       void gedit_recent_remove (char *filename);

static GList *	gedit_recent_history_get_list (void);
static gchar *	gedit_recent_history_update_list (const gchar *filename);
void		gedit_recent_history_write_config (void);

/* FIXME: make the list local */
GList *gedit_recent_history_list = NULL;

static GList *
gedit_recent_history_get_list (void)
{
        gchar *filename, *key;
        gint max_entries, i;
        gboolean do_set = FALSE;

	gedit_debug (DEBUG_RECENT, "");
	
	if (gedit_recent_history_list)
		return gedit_recent_history_list;

	gnome_config_push_prefix ("/gedit/History/");

        /* Get maximum number of history entries.  Write default value to 
         * config file if no entry exists. */
        max_entries = gnome_config_get_int_with_default ("MaxFiles=4", &do_set);
	if (do_set)
                gnome_config_set_int ("MaxFiles", 4);

        /* Read the history filenames from the config file */
        for (i = 0; i < max_entries; i++)
	{
                key = g_strdup_printf ("File%d", i);
                filename = gnome_config_get_string (key);
                if (filename == NULL)
		{
			/* Ran out of filenames. */
			g_free (key);
			break;
                }
                gedit_recent_history_list = g_list_append (gedit_recent_history_list, filename);
                g_free (key);
        }

        gnome_config_pop_prefix ();

        return gedit_recent_history_list;
}

/**
 * gedit_recent_history_update_list:
 * @filename: 
 * 
 * This function updates the history list.  The return value is a 
 * pointer to the filename that was removed, if the list was already full
 * or NULL if no items were removed.
 * 
 * Return value: 
 **/
static gchar *
gedit_recent_history_update_list (const gchar *filename)
{
        gchar *name, *old_name = NULL;
        GList *l = NULL;
        GList *new_list = NULL;
        gint max_entries, count = 0;
        gboolean do_set = FALSE;
        gboolean found = FALSE;

	gedit_debug (DEBUG_RECENT, "");

        g_return_val_if_fail (filename != NULL, NULL);

        /* Get maximum list length from config */
        gnome_config_push_prefix ("/gedit/History/");
        max_entries = gnome_config_get_int_with_default ("MaxFiles=4", &do_set);
	if (do_set)
                gnome_config_set_int ("MaxFiles", max_entries);
        gnome_config_pop_prefix ();

        /* Check if this filename already exists in the list */
        for (l = gedit_recent_history_list; l && (count < max_entries); l = l->next)
	{
                if (!found && (!strcmp ((gchar *)l->data, filename)
			       || (count == max_entries - 1)))
		{
                        /* This is either the last item in the list, or a
                         * duplicate of the requested entry. */
                        old_name = (gchar *)l->data;
                        found = TRUE;
                }
		else  /* Add this item to the new list */
                        new_list = g_list_append (new_list, l->data);

                count++;
        }

        /* Insert the new filename to the new list and free up the old list */
        name = g_strdup (filename);
        new_list = g_list_prepend (new_list, name);
        g_list_free (gedit_recent_history_list);
        gedit_recent_history_list = new_list;

        return old_name;
}

/**
 * gedit_recent_history_write_config:
 * @void: 
 * 
 * Write contents of the history list to the configuration file.
 * 
 * Return Value: 
 **/
void
gedit_recent_history_write_config (void)
{
        gchar *key; 
        GList *l;
        gint max_entries, i = 0;

	gedit_debug (DEBUG_RECENT, "");

        max_entries = gnome_config_get_int ("/gedit/History/MaxFiles=4");
        gnome_config_clean_section ("/gedit/History");
	
        gnome_config_push_prefix ("/gedit/History/");
        gnome_config_set_int ("MaxFiles", max_entries);

        for (l = gedit_recent_history_list; l; l = l->next) {
                key = g_strdup_printf ("File%d", i++);
                gnome_config_set_string (key, (gchar *)l->data);
                g_free (l->data);
                g_free (key);
        }
	gnome_config_sync ();
        gnome_config_pop_prefix ();

        g_list_free (gedit_recent_history_list);
        gedit_recent_history_list = NULL;
}

#ifndef __DISABLE_UGLY_ULINE_HACK
/* We had a problem when adding files to the recent menus that contained underscores
 * because in gtk+ underscores are used to mark the accelerators in labels. The reason
 * so much code is needed to work arround this bug is because we are using gnome-libs
 * functions. This is an ugly hack which will go away when we port to GNOME 2.0
 */
static GtkWidget *
create_label (char *label_text, guint *keyval, GtkWidget **box, gint number)
{
	guint kv;
	gchar *number_text;
	GtkWidget *label;
	GtkWidget *number_widget;
	GtkWidget *hbox;

	number_text = g_strdup_printf ("_%i. ", number);

	hbox = gtk_hbox_new (FALSE, 0);

	label = gtk_label_new (label_text);
	number_widget = gtk_accel_label_new (number_text);
	
	gtk_box_pack_start (GTK_BOX (hbox), number_widget, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	kv = gtk_label_parse_uline (GTK_LABEL (number_widget), number_text);
	if (keyval)
		*keyval = kv;

	gtk_widget_show_all (hbox);

        *box = hbox;
	
	return number_widget;
}

static void
setup_uline_accel (GtkMenuShell  *menu_shell,
		   GtkAccelGroup *accel_group,
		   GtkWidget     *menu_item,
		   guint          keyval)
{
	if (keyval != GDK_VoidSymbol) {
		if (GTK_IS_MENU (menu_shell))
			gtk_widget_add_accelerator (menu_item,
						    "activate_item",
						    gtk_menu_ensure_uline_accel_group (GTK_MENU (menu_shell)),
						    keyval, 0,
						    0);
		if (GTK_IS_MENU_BAR (menu_shell) && accel_group)
			gtk_widget_add_accelerator (menu_item,
						    "activate_item", 
						    accel_group,
						    keyval, GDK_MOD1_MASK,
						    0);
	}
}

static void
create_menu_item (GtkMenuShell       *menu_shell,
		  GnomeUIInfo        *uiinfo,
		  int                 is_radio,
		  GSList            **radio_group,
		  GnomeUIBuilderData *uibdata,
		  GtkAccelGroup      *accel_group,
		  gboolean	      uline_accels,
		  gint		      pos,
		  gint number)
{
	GtkWidget *label;
	GtkWidget *box;
	guint keyval;
	int type;
	
	/* Translate configurable menu items to normal menu items. */

	if (uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
	        gnome_app_ui_configure_configurable( uiinfo );

	/* Create the menu item */

	switch (uiinfo->type) {
	case GNOME_APP_UI_ITEM:
		uiinfo->widget = gtk_menu_item_new ();
		break;
	case GNOME_APP_UI_SUBTREE:
	case GNOME_APP_UI_SUBTREE_STOCK:
	case GNOME_APP_UI_SEPARATOR:
	case GNOME_APP_UI_TOGGLEITEM:
	default:
		g_assert_not_reached ();
		break;
	}

	if (!accel_group)
		gtk_widget_lock_accelerators (uiinfo->widget);

	gtk_widget_show (uiinfo->widget);
	gtk_menu_shell_insert (menu_shell, uiinfo->widget, pos);

	/* If it is a separator, set it as insensitive so that it cannot be 
	 * selected, and return -- there is nothing left to do.
	 */

	if (uiinfo->type == GNOME_APP_UI_SEPARATOR) {
		gtk_widget_set_sensitive (uiinfo->widget, FALSE);
		return;
	}

	/* Create the contents of the menu item */

	/* Don't use gettext on the empty string since gettext will map
	 * the empty string to the header at the beginning of the .pot file. */

	label = create_label ( uiinfo->label [0] == '\0'?
			       "":(uiinfo->type == GNOME_APP_UI_SUBTREE_STOCK ?
				   D_(uiinfo->label):L_(uiinfo->label)),
			       &keyval,
			       &box,
			       number);

	gtk_container_add (GTK_CONTAINER (uiinfo->widget), box);

	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), 
					  uiinfo->widget);
	
	/* setup underline accelerators
	 */
	if (uline_accels)
		setup_uline_accel (menu_shell,
				   accel_group,
				   uiinfo->widget,
				   keyval);

	/* install global accelerator
	 */
	{
		GString *gstring;
		GtkWidget *widget;
		
		/* build up the menu item path */
		gstring = g_string_new ("");
		widget = uiinfo->widget;
		while (widget) {
			if (GTK_IS_MENU_ITEM (widget)) {
				GtkWidget *child = GTK_BIN (widget)->child;
				
				if (GTK_IS_LABEL (child)) {
					g_string_prepend (gstring, GTK_LABEL (child)->label);
					g_string_prepend_c (gstring, '/');
				}
				widget = widget->parent;
			} else if (GTK_IS_MENU (widget)) {
				widget = gtk_menu_get_attach_widget (GTK_MENU (widget));
				if (widget == NULL) {
					g_string_prepend (gstring, "/-Orphan");
					widget = NULL;
				}
			} else
				widget = widget->parent;
		}
		g_string_prepend_c (gstring, '>');
		g_string_prepend (gstring, gnome_app_id);
		g_string_prepend_c (gstring, '<');

		/* g_print ("######## menu item path: %s\n", gstring->str); */
		
		/* the item factory cares about installing the correct accelerator */
		
		gtk_item_factory_add_foreign (uiinfo->widget,
					      gstring->str,
					      accel_group,
					      uiinfo->accelerator_key,
					      uiinfo->ac_mods);
		g_string_free (gstring, TRUE);

	}
	
	/* Set toggle information, if appropriate */
	
	if ((uiinfo->type == GNOME_APP_UI_TOGGLEITEM) || is_radio) {
		gtk_check_menu_item_set_show_toggle
			(GTK_CHECK_MENU_ITEM(uiinfo->widget), TRUE);
		gtk_check_menu_item_set_active
			(GTK_CHECK_MENU_ITEM (uiinfo->widget), FALSE);
	}
	
	/* Connect to the signal and set user data */
	
	type = uiinfo->type;
	if (type == GNOME_APP_UI_SUBTREE_STOCK)
		type = GNOME_APP_UI_SUBTREE;
	
	if (type != GNOME_APP_UI_SUBTREE) {
	        gtk_object_set_data (GTK_OBJECT (uiinfo->widget),
				     GNOMEUIINFO_KEY_UIDATA,
				     uiinfo->user_data);

		gtk_object_set_data (GTK_OBJECT (uiinfo->widget),
				     GNOMEUIINFO_KEY_UIBDATA,
				     uibdata->data);

		(* uibdata->connect_func) (uiinfo, "activate", uibdata);
	}
}

static void
gedit_gnome_app_fill_menu_custom (GtkMenuShell       *menu_shell,
				  GnomeUIInfo        *uiinfo, 
				  GnomeUIBuilderData *uibdata,
				  GtkAccelGroup      *accel_group, 
				  gboolean            uline_accels,
				  gint                pos,
				  gint number)
{
	GnomeUIBuilderData *orig_uibdata;

	g_return_if_fail (menu_shell != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);
	g_return_if_fail (pos >= 0);

	/* Store a pointer to the original uibdata so that we can use it for 
	 * the subtrees */

	orig_uibdata = uibdata;

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
		case GNOME_APP_UI_HELP:
		case GNOME_APP_UI_RADIOITEMS:
		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_ITEM_CONFIGURABLE:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SUBTREE:
		case GNOME_APP_UI_SUBTREE_STOCK:
			g_assert_not_reached ();
			break;
		case GNOME_APP_UI_ITEM:
			create_menu_item (menu_shell, uiinfo, FALSE, NULL, uibdata, 
					  accel_group, uline_accels,
					  pos, number);
			pos++;
			break;
		default:
			g_warning ("Invalid GnomeUIInfo element type %d\n", 
					(int) uiinfo->type);
		}

	/* Make the end item contain a pointer to the parent menu shell */
	uiinfo->widget = GTK_WIDGET (menu_shell);
}

static void
do_ui_signal_connect (GnomeUIInfo *uiinfo, gchar *signal_name, 
		GnomeUIBuilderData *uibdata)
{
	if (uibdata->is_interp)
		gtk_signal_connect_full (GTK_OBJECT (uiinfo->widget), 
				signal_name, NULL, uibdata->relay_func, 
				uibdata->data ? 
				uibdata->data : uiinfo->user_data,
				uibdata->destroy_func, FALSE, FALSE);
	
	else if (uiinfo->moreinfo)
		gtk_signal_connect (GTK_OBJECT (uiinfo->widget), 
				signal_name, uiinfo->moreinfo, uibdata->data ? 
				uibdata->data : uiinfo->user_data);
}

static void
gedit_insert_menus (GnomeApp *app, const gchar *path, GnomeUIInfo *menuinfo, gint number)
{
	GtkWidget *parent;
	gint pos;
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};

	parent = gnome_app_find_menu_pos(app->menubar, path, &pos);
	if(parent == NULL) {
		g_warning("gnome_app_insert_menus_custom: couldn't find "
			  "insertion point for menus!");
		return;
	}

	/* create menus and insert them */
	gedit_gnome_app_fill_menu_custom (GTK_MENU_SHELL (parent), menuinfo, &uidata, 
					  app->accel_group, TRUE, pos, number);
}
#endif

static void
gedit_recent_add_menu_item (GnomeApp *app, const gchar *file_name, const gchar *path, gint num)
{
	GnomeUIInfo *menu;
	gchar* unescaped_str;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (file_name != NULL);
	g_return_if_fail (path != NULL);

	unescaped_str = gnome_vfs_unescape_string_for_display (file_name);
	g_return_if_fail (unescaped_str != NULL);
	
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	menu->label = g_strdup_printf ("%s", unescaped_str);
	menu->type = GNOME_APP_UI_ITEM;
	menu->hint = NULL;
	menu->moreinfo = (gpointer) recent_cb;
	menu->user_data = (gchar *) file_name;
	menu->unused_data = NULL;
	menu->pixmap_type = 0;
	menu->pixmap_info = NULL;
	menu->accelerator_key = 0;
	
	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
#if 1
	gedit_insert_menus (GNOME_APP (app), path, menu, num);
#else	
	gnome_app_insert_menus (GNOME_APP(app), path, menu);
#endif

	g_free (unescaped_str);
	g_free (menu->label);
	g_free (menu);
}

/**
 * gedit_recent_update_menus:
 * @app: 
 * @recent_files: 
 * 
 * Update the gui part of the recently-used menu
 * 
 **/
static void
gedit_recent_update_menus (GnomeApp *app, GList *recent_files)
{	
	static gint items_in_menu = 0;
	const gchar *file_name;
	gchar *path;
	int i;
	gint* items_to_remove = NULL;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (app != NULL);

	items_to_remove = gtk_object_get_data (GTK_OBJECT(app), DATA_ITEMS_TO_REMOVE);

	if(items_to_remove != NULL) {

	        /* xgettext translators: Use the name of the stock 
		 * Menu item for "File"	if it is not the same, we are going to 
		 * fail to insert the recent menu items.*/
		gnome_app_remove_menu_range (app, _("_File/"), 7, *items_to_remove);
	
		gtk_object_remove_data(GTK_OBJECT(app), DATA_ITEMS_TO_REMOVE);
	}

	items_in_menu = g_list_length (recent_files);
	
        /* xgettext translators: Use the name of the stock Menu item for "File"
	   if it is not the same, we are going to fail to insert the recent menu items. */
	path = g_strdup_printf ("%s/%s", _("_File"), "<Separator>");
	for (i = items_in_menu; i > 0;  i--) {
		file_name = g_list_nth_data (recent_files, i-1);
		gedit_recent_add_menu_item (app, file_name, path, i);
	}
	g_free (path);

	items_to_remove = g_new0 (gint, 1);
	*items_to_remove = items_in_menu;
	gtk_object_set_data_full (GTK_OBJECT(app), DATA_ITEMS_TO_REMOVE,items_to_remove, g_free);
}
	
/* Callback for a click on a file in the recently used menu */
static void
recent_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (data != NULL);

	if (!gedit_document_new_with_file (data))
	{
		gedit_flash_va (_("Unable to open recent file: %s"), (gchar *) data);
		gedit_recent_remove ((gchar *) data);
		gedit_recent_history_write_config ();
		gedit_recent_update_all_windows (mdi);		
	}
	else
	{
		gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), gedit_document_current()->readonly);
	}

}

/**
 * recent_update:
 * @app: 
 *
 * Grabs the list of recently used documents, then updates the menus by
 * calling recent_update_menus().  Should be called when a new
 * window is created. It updates the menu of a single window.
 **/
void
gedit_recent_update (GnomeApp *app)
{
	GList *filelist = NULL;

	gedit_debug (DEBUG_RECENT, "");

	filelist = gedit_recent_history_get_list ();

	gedit_recent_update_menus (app, filelist);
}

/**
 * recent_update_all_windows:
 * @app: 
 *
 * Updates the recent files menus in all open windows.
 * Should be called after each addition to the recent documents list.
 **/
void
gedit_recent_update_all_windows (GnomeMDI *mdi)
{
	gint i;
	
	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (mdi != NULL);
       	g_return_if_fail (mdi->windows != NULL);
       
	for (i = 0; i < g_list_length (mdi->windows); i++)
        {
                gedit_recent_update (GNOME_APP (g_list_nth_data (mdi->windows, i)));
        }
}


/**
 * recent_add:
 * @filename: Filename of document to add to the recently accessed list
 *
 * Record a file in GNOME's recent documents database 
 **/
void
gedit_recent_add (const char *file_name)
{
	gchar *del_name;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (file_name != NULL);

	del_name = gedit_recent_history_update_list (file_name);

	g_free (del_name);
}


/**
 * recent_remove:
 * @filename: Filename of document to remove from the recently accessed list
 *
 * Record a file in GNOME's recent documents database 
 **/
void
gedit_recent_remove (char *file_name)
{
	gint n;
	guchar * nth_list_item;

	gedit_debug (DEBUG_RECENT, "");

	for (n=0; n < g_list_length (gedit_recent_history_list); n++)
	{
		nth_list_item = g_list_nth_data (gedit_recent_history_list, n);
		if (strcmp (nth_list_item, file_name) == 0)
		{
			gedit_recent_history_list = g_list_remove (gedit_recent_history_list, nth_list_item);
			g_free (nth_list_item);			
			return;
		}
	}
}
