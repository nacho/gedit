/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-encodings-option-menu.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 - Paolo Maggi
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
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <gtk/gtk.h>

#include <libgnome/gnome-i18n.h>

#include "gedit-encodings-option-menu.h"

#include "gedit-prefs-manager.h"

#include "dialogs/gedit-encodings-dialog.h"

#define ENCODING_KEY "Enconding"

static void	 	  gedit_encodings_option_menu_class_init 	(GeditEncodingsOptionMenuClass 	*klass);
static void		  gedit_encodings_option_menu_init		(GeditEncodingsOptionMenu 	*menu);
static void		  gedit_encodings_option_menu_finalize 		(GObject 			*object);

static void		  update_menu 					(GeditEncodingsOptionMenu       *option_menu);

struct _GeditEncodingsOptionMenuPrivate
{
	gint activated_item;
};

static GObjectClass 	 *parent_class  = NULL;

GType
gedit_encodings_option_menu_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GeditEncodingsOptionMenuClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gedit_encodings_option_menu_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (GeditEncodingsOptionMenu),
			0,	/* n_preallocs */
			(GInstanceInitFunc) gedit_encodings_option_menu_init
		};

		our_type =
		    g_type_register_static (GTK_TYPE_OPTION_MENU,
					    "GeditEncodingsOptionMenu", &our_info, 0);
	}

	return our_type;
}

static void
gedit_encodings_option_menu_class_init (GeditEncodingsOptionMenuClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class		= g_type_class_peek_parent (klass);
	object_class->finalize	= gedit_encodings_option_menu_finalize;
}

static void
add_or_remove (GtkMenuItem *menu_item, GeditEncodingsOptionMenu *option_menu)
{

	if (GTK_IS_RADIO_MENU_ITEM (menu_item))
		option_menu->priv->activated_item = 
			gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu));
	else
	{
		
		GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (option_menu));
		
	       	if (!GTK_WIDGET_TOPLEVEL (toplevel))
			toplevel = NULL;	
		
		gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
					     option_menu->priv->activated_item);

		if (gedit_encodings_dialog_run ((toplevel != NULL) ? GTK_WINDOW (toplevel) : NULL))
			update_menu (option_menu);
	}
}

static void
update_menu (GeditEncodingsOptionMenu *option_menu)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	GSList    *group = NULL;
	GSList	  *encodings, *list;

	menu = gtk_menu_new ();

	menu_item = gtk_radio_menu_item_new_with_label (group, _("Auto Detected"));
	group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_separator_menu_item_new ();

	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (add_or_remove),
			  option_menu);

      	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	encodings = list = gedit_prefs_manager_get_encodings ();

	while (list != NULL)
	{
		const GeditEncoding *enc;
		gchar *str;

		enc = (const GeditEncoding *)list->data;
		str = gedit_encoding_to_string (enc);

		menu_item = gtk_radio_menu_item_new_with_label (group, str);

		g_signal_connect (menu_item,
				  "activate",
				  G_CALLBACK (add_or_remove),
				  option_menu);

		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));

		g_object_set_data (G_OBJECT (menu_item), ENCODING_KEY, (gpointer)enc);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);

		g_free (str);

		list = g_slist_next (list);
	}

	g_slist_free (encodings);

	menu_item = gtk_separator_menu_item_new ();
      	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic (_("Add or _Remove..."));

	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (add_or_remove),
			  option_menu);
	
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
}

static void
gedit_encodings_option_menu_init (GeditEncodingsOptionMenu *menu)
{
	menu->priv = g_new0 (GeditEncodingsOptionMenuPrivate, 1);

	update_menu (menu);
}

static void
gedit_encodings_option_menu_finalize (GObject *object)
{
	GeditEncodingsOptionMenu *menu;

	menu =  GEDIT_ENCODINGS_OPTION_MENU (object);
		
	if (menu->priv != NULL)
	{
		g_free (menu->priv); 
	}
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
gedit_encodings_option_menu_new (void)
{
	return g_object_new (GEDIT_TYPE_ENCODINGS_OPTION_MENU, NULL);
}

const GeditEncoding *
gedit_encodings_option_menu_get_selected_encoding (GeditEncodingsOptionMenu *menu)
{
	g_return_val_if_fail (GEDIT_IS_ENCODINGS_OPTION_MENU (menu), NULL);
	
	return NULL;
}

void
gedit_encodings_option_menu_set_selected_encoding (GeditEncodingsOptionMenu *menu,
						   const GeditEncoding      *encoding)
{
	g_return_if_fail (GEDIT_IS_ENCODINGS_OPTION_MENU (menu));
}

