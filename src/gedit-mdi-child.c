/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-mdi-child.c
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
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <libgnomeui/gnome-popup-menu.h>
#include <libgnomeui/gnome-icon-theme.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>

#include <bonobo/bonobo-i18n.h>

#include "gedit-mdi-child.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-marshal.h"
#include "gedit-file.h"
#include "gedit2.h"
#include "gedit-print.h"
#include "recent-files/egg-recent-util.h"

struct _GeditMDIChildPrivate
{
	gint dummy;
};

enum {
	STATE_CHANGED,
	UNDO_REDO_STATE_CHANGED,
	LAST_SIGNAL
};

static void gedit_mdi_child_class_init 	(GeditMDIChildClass	*klass);
static void gedit_mdi_child_init	(GeditMDIChild 		*mdi);
static void gedit_mdi_child_finalize 	(GObject 		*obj);

static void gedit_mdi_child_real_state_changed (GeditMDIChild* child);

static GtkWidget *gedit_mdi_child_create_view (BonoboMDIChild *child);

static void gedit_mdi_child_document_state_changed_handler (GeditDocument *document, 
							   GeditMDIChild* child);
static void gedit_mdi_child_document_readonly_changed_handler (GeditDocument *document, 
						gboolean readonly, GeditMDIChild* child);

static void gedit_mdi_child_document_can_undo_redo_handler (GeditDocument *document, 
						gboolean can, GeditMDIChild* child);

static gchar* gedit_mdi_child_get_config_string (BonoboMDIChild *child, gpointer data);


static GtkWidget * gedit_mdi_child_set_label (BonoboMDIChild *child,
					      GtkWidget *view,					
                                              GtkWidget *old_label,
					      gpointer data);

static BonoboMDIChildClass *parent_class = NULL;
static guint mdi_child_signals[LAST_SIGNAL] = { 0 };


GType
gedit_mdi_child_get_type (void)
{
	static GType mdi_child_type = 0;

  	if (mdi_child_type == 0)
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditMDIChildClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_mdi_child_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditMDIChild),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_mdi_child_init
      		};

      		mdi_child_type = g_type_register_static (BONOBO_TYPE_MDI_CHILD,
                				    	  "GeditMDIChild",
                                       	 	    	  &our_info,
                                       		    	  0);
    	}

	return mdi_child_type;
}

static void
gedit_mdi_child_class_init (GeditMDIChildClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

  	gobject_class->finalize = gedit_mdi_child_finalize;

	klass->state_changed 		= gedit_mdi_child_real_state_changed;
	klass->undo_redo_state_changed  = NULL;
  		
	mdi_child_signals [STATE_CHANGED] =
		g_signal_new ("state_changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GeditMDIChildClass, state_changed),
			      NULL, NULL,
			      gedit_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	mdi_child_signals [UNDO_REDO_STATE_CHANGED] =
		g_signal_new ("undo_redo_state_changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GeditMDIChildClass, undo_redo_state_changed),
			      NULL, NULL,
			      gedit_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
                    
	BONOBO_MDI_CHILD_CLASS (klass)->create_view = 
		(BonoboMDIChildViewCreator)(gedit_mdi_child_create_view);

	BONOBO_MDI_CHILD_CLASS (klass)->get_config_string = 
		(BonoboMDIChildConfigFunc)(gedit_mdi_child_get_config_string);

	BONOBO_MDI_CHILD_CLASS (klass)->set_label = 
		(BonoboMDIChildLabelFunc)(gedit_mdi_child_set_label);

}

static void 
gedit_mdi_child_init (GeditMDIChild  *child)
{
	gedit_debug (DEBUG_MDI, "START");

	child->priv = g_new0 (GeditMDIChildPrivate, 1);

	gedit_debug (DEBUG_MDI, "END");
}

static void 
gedit_mdi_child_finalize (GObject *obj)
{
	GeditMDIChild *child;

	gedit_debug (DEBUG_MDI, "START");

	g_return_if_fail (obj != NULL);
	
   	child = GEDIT_MDI_CHILD (obj);

	g_return_if_fail (GEDIT_IS_MDI_CHILD (child));
	g_return_if_fail (child->priv != NULL);

	if (child->document != NULL)
		g_object_unref (G_OBJECT (child->document));

	g_free (child->priv);
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);

	gedit_debug (DEBUG_MDI, "END");
}

static void 
gedit_mdi_child_real_state_changed (GeditMDIChild* child)
{
	gchar* docname = NULL;
	gchar* tab_name = NULL;

	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (child != NULL);
	g_return_if_fail (child->document != NULL);

	docname = gedit_document_get_short_name (child->document);
	g_return_if_fail (docname != NULL);
	
	if (gedit_document_get_modified (child->document))
	{
		tab_name = g_strdup_printf ("%s*", docname);
	} 
	else 
	{
		if (gedit_document_is_readonly (child->document)) 
		{
			tab_name = g_strdup_printf ("%s - %s", docname, 
						/*Read only*/ _("RO"));
		} 
		else 
		{
			tab_name = g_strdup_printf ("%s", docname);
		}
	}
	
	g_free (docname);

	g_return_if_fail (tab_name != NULL);

	bonobo_mdi_child_set_name (BONOBO_MDI_CHILD (child), tab_name);

	g_free (tab_name);	
}

static void 
gedit_mdi_child_document_state_changed_handler (GeditDocument *document, GeditMDIChild* child)
{
	gedit_debug (DEBUG_MDI, "");
	g_return_if_fail (child->document == document);

	g_signal_emit (G_OBJECT (child), mdi_child_signals [STATE_CHANGED], 0);
}

static void 
gedit_mdi_child_document_readonly_changed_handler (GeditDocument *document, gboolean readonly, 
		               			   GeditMDIChild* child)
{
	gedit_debug (DEBUG_MDI, "");
	g_return_if_fail (child->document == document);

	g_signal_emit (G_OBJECT (child), mdi_child_signals [STATE_CHANGED], 0);
}

static void 
gedit_mdi_child_document_can_undo_redo_handler (GeditDocument *document, gboolean can, 
		               			GeditMDIChild* child)
{
	gedit_debug (DEBUG_MDI, "");
	g_return_if_fail (child->document == document);

	g_signal_emit (G_OBJECT (child), mdi_child_signals [UNDO_REDO_STATE_CHANGED], 0);
}


static void
gedit_mdi_child_connect_signals (GeditMDIChild *child)
{
	g_signal_connect (G_OBJECT (child->document), "name_changed",
			  G_CALLBACK (gedit_mdi_child_document_state_changed_handler), 
			  child);
	g_signal_connect (G_OBJECT (child->document), "readonly_changed",
			  G_CALLBACK (gedit_mdi_child_document_readonly_changed_handler), 
			  child);
	g_signal_connect (G_OBJECT (child->document), "modified_changed",
			  G_CALLBACK (gedit_mdi_child_document_state_changed_handler), 
			  child);
	g_signal_connect (G_OBJECT (child->document), "can_undo",
			  G_CALLBACK (gedit_mdi_child_document_can_undo_redo_handler), 
			  child);
	g_signal_connect (G_OBJECT (child->document), "can_redo",
			  G_CALLBACK (gedit_mdi_child_document_can_undo_redo_handler), 
			  child);
}

/**
 * gedit_mdi_child_new:
 * 
 * Creates a new #GeditMDIChild object.
 *
 * Return value: a new #GeditMDIChild
 **/
GeditMDIChild*
gedit_mdi_child_new (void)
{
	GeditMDIChild *child;
	gchar* doc_name;
	
	gedit_debug (DEBUG_MDI, "");

	child = GEDIT_MDI_CHILD (g_object_new (GEDIT_TYPE_MDI_CHILD, NULL));
  	g_return_val_if_fail (child != NULL, NULL);
	
	child->document = gedit_document_new ();
	g_return_val_if_fail (child->document != NULL, NULL);
	
	doc_name = gedit_document_get_short_name (child->document);
	bonobo_mdi_child_set_name (BONOBO_MDI_CHILD (child), doc_name);
	g_free(doc_name);

	gedit_mdi_child_connect_signals (child);
	
	gedit_debug (DEBUG_MDI, "END");
	
	return child;
}

GeditMDIChild*
gedit_mdi_child_new_with_uri (const gchar *uri, GError **error)
{
	GeditMDIChild *child;
	GeditDocument* doc;
	
	gedit_debug (DEBUG_MDI, "");

	doc = gedit_document_new_with_uri (uri, error);

	if (doc == NULL)
	{
		gedit_debug (DEBUG_MDI, "ERROR: %s", (*error)->message);
		return NULL;
	}

	child = GEDIT_MDI_CHILD (g_object_new (GEDIT_TYPE_MDI_CHILD, NULL));
  	g_return_val_if_fail (child != NULL, NULL);
	
	child->document = doc;
	g_return_val_if_fail (child->document != NULL, NULL);
	
	gedit_mdi_child_real_state_changed (child);
	
	gedit_mdi_child_connect_signals (child);

	gedit_debug (DEBUG_MDI, "END");
	
	return child;
}
		
static GtkWidget *
gedit_mdi_child_create_view (BonoboMDIChild *child)
{
	GeditView  *new_view;

	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (GEDIT_IS_MDI_CHILD (child), NULL);

	new_view = gedit_view_new (GEDIT_MDI_CHILD (child)->document);

	gtk_widget_show_all (GTK_WIDGET (new_view));

	return GTK_WIDGET (new_view);
}

static gchar* 
gedit_mdi_child_get_config_string (BonoboMDIChild *child, gpointer data)
{
	GeditMDIChild *c;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (GEDIT_IS_MDI_CHILD (child), NULL);

	c = GEDIT_MDI_CHILD (child);
	
	return gedit_document_get_raw_uri (c->document);
}

static void
gedit_mdi_child_tab_close_clicked (GtkWidget *button, GtkWidget *view)
{
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
	
	gedit_file_close (view);
}

static void
gedit_mdi_child_tab_save_clicked (GtkWidget *button,  GtkWidget *view)
{
	BonoboMDIChild *child;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
	
	child = bonobo_mdi_child_get_from_view (view);
	g_return_if_fail (GEDIT_IS_MDI_CHILD (child));

	gedit_file_save (GEDIT_MDI_CHILD (child), TRUE);
}

static void
gedit_mdi_child_tab_save_as_clicked (GtkWidget *button,  GtkWidget *view)
{
	BonoboMDIChild *child;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
	
	child = bonobo_mdi_child_get_from_view (view);
	g_return_if_fail (GEDIT_IS_MDI_CHILD (child));

	gedit_file_save_as (GEDIT_MDI_CHILD (child));
}

static void
gedit_mdi_child_tab_print_clicked (GtkWidget *button,  GtkWidget *view)
{
	BonoboMDIChild *child;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
	
	child = bonobo_mdi_child_get_from_view (view);
	g_return_if_fail (GEDIT_IS_MDI_CHILD (child));
	
	gedit_print (GEDIT_MDI_CHILD (child));
}

static void
gedit_mdi_child_tab_move_window_clicked (GtkWidget *button,  GtkWidget *view)
{
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);

	bonobo_mdi_move_view_to_new_window (BONOBO_MDI (gedit_mdi), view);
}

static void 
menu_show (GtkWidget *widget, GtkWidget *view)
{
	GtkWidget *menu_item;

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)) != view)
		bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);

	menu_item = g_object_get_data (G_OBJECT (widget), "move-to-menu-item");
	
	gtk_widget_set_sensitive (menu_item, 
			bonobo_mdi_n_children_for_window (
				bonobo_mdi_get_window_from_view (view)) > 1);
}

static void 
read_only_changed_cb (GeditDocument *doc, gboolean readonly, gpointer *data)
{
	g_return_if_fail (GTK_IS_WIDGET (data));

	gtk_widget_set_sensitive (GTK_WIDGET (data), !readonly);
}

static GtkWidget *
create_popup_menu (BonoboMDIChild *child, GtkWidget *view)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	GeditDocument *doc;
	
	menu = gtk_menu_new ();

	/* Add the close button */
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE, NULL);
	gtk_widget_show (menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_mdi_child_tab_close_clicked), view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the separator */
	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the print button */
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PRINT, NULL);
	gtk_widget_show (menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_mdi_child_tab_print_clicked), view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the separator */
	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the save as button */
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE_AS, NULL);
	gtk_widget_show (menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_mdi_child_tab_save_as_clicked), view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the save button */
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, NULL);
	gtk_widget_show (menu_item);

	doc = GEDIT_MDI_CHILD (child)->document;
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	gtk_widget_set_sensitive (menu_item, !gedit_document_is_readonly (doc));
	g_signal_connect (G_OBJECT (doc), "readonly_changed",
		      	  G_CALLBACK (read_only_changed_cb), menu_item);

	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_mdi_child_tab_save_clicked), view);
	
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
		
	/* Add the separator */
	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the detach tab button */
	menu_item = gtk_menu_item_new_with_mnemonic (_("_Move to New Window"));
	gtk_widget_show (menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_mdi_child_tab_move_window_clicked), view);
	
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	g_object_set_data (G_OBJECT (menu), "move-to-menu-item", menu_item);

	g_signal_connect (G_OBJECT (menu), "show",
		      	  G_CALLBACK (menu_show), view);

	return menu;
}


/* FIXME: implementing theme changed handler */

static GnomeIconTheme *theme = NULL;

static GtkWidget *
set_tab_icon (GtkWidget *image, BonoboMDIChild *child)
{
	GdkPixbuf *pixbuf;
	gchar *raw_uri;
	gchar *mime_type = NULL;
	int icon_size;

	g_return_val_if_fail (GTK_IS_IMAGE (image), NULL);
	g_return_val_if_fail (GEDIT_IS_MDI_CHILD (child), NULL);

	if (theme == NULL)
		theme = gnome_icon_theme_new ();

	raw_uri = gedit_document_get_uri (GEDIT_MDI_CHILD (child)->document);	
	
	if (raw_uri != NULL)
		mime_type = gnome_vfs_get_mime_type (raw_uri);

	if (mime_type == NULL)
		mime_type = g_strdup ("text/plain");

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (image),
					   GTK_ICON_SIZE_MENU, NULL,
					   &icon_size);
	pixbuf = egg_recent_util_get_icon (theme, raw_uri,
					   mime_type, icon_size);

	g_free (raw_uri);
	g_free (mime_type);
		
	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
		
	if (pixbuf)
		g_object_unref (pixbuf);

	return image;	
}
	
static GtkWidget *
gedit_mdi_child_set_label (BonoboMDIChild *child, GtkWidget *view,  GtkWidget *old_hbox,
                           gpointer data)
{
	GtkWidget *ret;
	gchar *name;
	gchar *uri;

	static GtkTooltips *tooltips = NULL;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (GEDIT_IS_MDI_CHILD (child), NULL);
	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);
		
	if (tooltips == NULL)
		tooltips = gtk_tooltips_new ();
	
	name = bonobo_mdi_child_get_name (child);

 	uri = gedit_document_get_uri (GEDIT_MDI_CHILD (child)->document);
	g_return_val_if_fail (uri != NULL, NULL);
		
	if (old_hbox != NULL) 
	{
		GtkWidget *label;
		GtkWidget *image;

		label = g_object_get_data (G_OBJECT (old_hbox),	"label");
		image = g_object_get_data (G_OBJECT (old_hbox),	"image");
	
		gtk_label_set_text (GTK_LABEL (label), name);
		set_tab_icon (image, child);

		ret = old_hbox;
	} 
	else 
	{
		GtkWidget *event_box;	
		GtkSettings *settings;
		GtkWidget *hbox;
		GtkWidget *label;
		GtkWidget *button;
		GtkWidget *image;
		GtkWidget *popup_menu;
		gint w, h;

		settings = gtk_widget_get_settings (view);

		gtk_icon_size_lookup_for_settings (settings,
						   GTK_ICON_SIZE_MENU,
						   &w, &h);

		label = gtk_label_new (name);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

		button = gtk_button_new ();
				
		g_signal_connect (button, "clicked",
				  G_CALLBACK (gedit_mdi_child_tab_close_clicked),
				  view);
		
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
		image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
		gtk_widget_set_size_request (button, w + 2, h + 2);

		gtk_container_add (GTK_CONTAINER (button), image);
		
		image = gtk_image_new ();
		set_tab_icon (image, child);
		
		gtk_widget_show (image);

		hbox = gtk_hbox_new (FALSE, 2);

		event_box = gtk_event_box_new ();
		
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		
		g_object_set_data (G_OBJECT (event_box), "label", label);
		g_object_set_data (G_OBJECT (event_box), "image", image);
		
		popup_menu = create_popup_menu (child, view);
		
		gnome_popup_menu_attach (popup_menu, event_box, NULL);

		gtk_container_add (GTK_CONTAINER (event_box), hbox);

		gtk_widget_show_all (event_box);
		ret = event_box;
	}

	gtk_tooltips_set_tip (tooltips, ret, uri, NULL);

	g_free (name);
	g_free (uri);

	return ret;
}
