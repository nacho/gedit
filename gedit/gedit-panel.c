/*
 * gedit-panel.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#include "gedit-panel.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "gedit-window.h"
#include "gedit-debug.h"

#define PANEL_ITEM_KEY "GeditPanelItemKey"

#define GEDIT_PANEL_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PANEL, GeditPanelPrivate))

struct _GeditPanelPrivate 
{
	GtkOrientation orientation;
	
	/* Title bar (vertical panel only) */
	GtkWidget *title_image;
	GtkWidget *title_label;

	/* Toolbar */
	GtkWidget *toolbar_container;
	GtkWidget *toolbar;

	/* Notebook */
	GtkWidget *notebook;

	GtkTooltips *tooltips;
};

typedef struct _GeditPanelItem GeditPanelItem;

struct _GeditPanelItem 
{
	gchar *name;
	GtkWidget *icon;
	GtkWidget *toolbar;
};

/* Properties */
enum {
	PROP_0,
	PROP_ORIENTATION
};

/* Signals */
enum {
	CLOSE,
	FOCUS_DOCUMENT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void	 gedit_panel_set_property	(GObject         *object,
						 guint            prop_id,
						 const GValue    *value,
						 GParamSpec      *pspec);

static void	 gedit_panel_class_init		(GeditPanelClass *klass);

static void	 gedit_panel_init 		(GeditPanel *panel);

static GObject	*gedit_panel_constructor	(GType type,
						 guint n_construct_properties,
						 GObjectConstructParam *construct_properties);


G_DEFINE_TYPE(GeditPanel, gedit_panel, GTK_TYPE_VBOX)

static void
gedit_panel_finalize (GObject *obj)
{
	g_object_unref (GEDIT_PANEL (obj)->priv->tooltips);

	if (G_OBJECT_CLASS (gedit_panel_parent_class)->finalize)
		(*G_OBJECT_CLASS (gedit_panel_parent_class)->finalize) (obj);
}

static void
gedit_panel_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	GeditPanel *panel = GEDIT_PANEL (object);
	
	switch (prop_id)
	{
		case PROP_ORIENTATION:
			g_value_set_enum(value, panel->priv->orientation);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;

	}
}

static void
gedit_panel_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GeditPanel *panel = GEDIT_PANEL (object);

	switch (prop_id)
	{
		case PROP_ORIENTATION:
			panel->priv->orientation = g_value_get_enum (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_panel_close (GeditPanel *panel)
{
	gtk_widget_hide (GTK_WIDGET (panel));
}

static void
gedit_panel_focus_document (GeditPanel *panel)
{
	GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (panel));
	if (GTK_WIDGET_TOPLEVEL (toplevel) && GEDIT_IS_WINDOW (toplevel))
   	{
		GeditView *view;

		view = gedit_window_get_active_view (GEDIT_WINDOW (toplevel));
		if (view != NULL)
			gtk_widget_grab_focus (GTK_WIDGET (view));
   	}
}

static void
gedit_panel_grab_focus (GtkWidget *w)
{
	gint n;
	GtkWidget *tab;
	GeditPanel *panel = GEDIT_PANEL (w);

	n = gtk_notebook_get_current_page (GTK_NOTEBOOK (panel->priv->notebook));
	if (n == -1)
		return;

	tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (panel->priv->notebook),
					 n);
	g_return_if_fail (tab != NULL);

	gtk_widget_grab_focus (tab);
}

static void
gedit_panel_class_init (GeditPanelClass *klass)
{
	GtkBindingSet *binding_set;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GeditPanelPrivate));

	object_class->constructor = gedit_panel_constructor;
	object_class->finalize = gedit_panel_finalize;
	object_class->get_property = gedit_panel_get_property;
	object_class->set_property = gedit_panel_set_property;

	g_object_class_install_property (object_class,
					 PROP_ORIENTATION,
					 g_param_spec_enum ("orientation",
							    "Orientation",
							    "The panel's orientation",
							    GTK_TYPE_ORIENTATION,
							    GTK_ORIENTATION_VERTICAL,
							    G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	widget_class->grab_focus = gedit_panel_grab_focus;

	klass->close = gedit_panel_close;
	klass->focus_document = gedit_panel_focus_document;

	signals[CLOSE] =  g_signal_new ("close",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (GeditPanelClass, close),
		  			NULL, NULL,
		  			g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
	signals[FOCUS_DOCUMENT] =  g_signal_new ("focus_document",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (GeditPanelClass, focus_document),
		  			NULL, NULL,
		  			g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);					
	binding_set = gtk_binding_set_by_class (klass);
  
	gtk_binding_entry_add_signal (binding_set, 
				      GDK_Escape, 
				      0, 
				      "close", 
				      0);
	gtk_binding_entry_add_signal (binding_set, 
				      GDK_Return, 
				      GDK_CONTROL_MASK, 
				      "focus_document", 
				      0);
}

/* This is ugly, since it supports only known
 * storage types of GtkImage, otherwise fall back
 * to the empty icon.
 * See http://bugzilla.gnome.org/show_bug.cgi?id=317520.
 */
static void
set_gtk_image_from_gtk_image (GtkImage *image,
			      GtkImage *source)
{
	switch (gtk_image_get_storage_type (source))
	{
	case GTK_IMAGE_EMPTY:
		gtk_image_clear (image);
		break;
	case GTK_IMAGE_PIXMAP:
		{
			GdkPixmap *pm;
			GdkBitmap *bm;

			gtk_image_get_pixmap (source, &pm, &bm);
			gtk_image_set_from_pixmap (image, pm, bm);
		}
		break;
	case GTK_IMAGE_IMAGE:
		{
			GdkImage *i;
			GdkBitmap *bm;

			gtk_image_get_image (source, &i, &bm);
			gtk_image_set_from_image (image, i, bm);
		}
		break;
	case GTK_IMAGE_PIXBUF:
		{
			GdkPixbuf *pb;

			pb = gtk_image_get_pixbuf (source);
			gtk_image_set_from_pixbuf (image, pb);
		}
		break;
	case GTK_IMAGE_STOCK:
		{
			gchar *s_id;
			GtkIconSize s;

			gtk_image_get_stock (source, &s_id, &s);
			gtk_image_set_from_stock (image, s_id, s);
		}
		break;
	case GTK_IMAGE_ICON_SET:
		{
			GtkIconSet *is;
			GtkIconSize s;

			gtk_image_get_icon_set (source, &is, &s);
			gtk_image_set_from_icon_set (image, is, s);
		}
		break;
	case GTK_IMAGE_ANIMATION:
		{
			GdkPixbufAnimation *a;

			a = gtk_image_get_animation (source);
			gtk_image_set_from_animation (image, a);
		}
		break;
	case GTK_IMAGE_ICON_NAME:
		{
			const gchar *n;
			GtkIconSize s;

			gtk_image_get_icon_name (source, &n, &s);
			gtk_image_set_from_icon_name (image, n, s);
		}
		break;
	default:
		gtk_image_set_from_stock (image,
					  GTK_STOCK_FILE,
					  GTK_ICON_SIZE_MENU);
	}
}

static void
sync_title (GeditPanel     *panel,
	    GeditPanelItem *item)
{
	if (panel->priv->orientation != GTK_ORIENTATION_VERTICAL)
		return;

	if (item != NULL)
	{
		gtk_label_set_text (GTK_LABEL (panel->priv->title_label), 
				    item->name);

		set_gtk_image_from_gtk_image (GTK_IMAGE (panel->priv->title_image),
					      GTK_IMAGE (item->icon));
	}
	else
	{
		gtk_label_set_text (GTK_LABEL (panel->priv->title_label), 
				    _("Empty"));

		gtk_image_set_from_stock (GTK_IMAGE (panel->priv->title_image),
					  GTK_STOCK_FILE,
					  GTK_ICON_SIZE_MENU);
	}
}

static void
sync_toolbar (GeditPanel *panel,
	      GeditPanelItem *item)
{
	if (panel->priv->toolbar != NULL)
		gtk_container_remove (GTK_CONTAINER (panel->priv->toolbar_container),
				      panel->priv->toolbar);

	if (item != NULL && item->toolbar != NULL)
	{
		panel->priv->toolbar = item->toolbar;
		gtk_container_add (GTK_CONTAINER (panel->priv->toolbar_container),
				   item->toolbar);
	}
	else
	{
		panel->priv->toolbar = NULL;
	}
}

static void
notebook_page_changed (GtkNotebook     *notebook,
                       GtkNotebookPage *page,
                       guint            page_num,
                       GeditPanel      *panel)
{
	GtkWidget *item;
	GeditPanelItem *data;

	item = gtk_notebook_get_nth_page (notebook, page_num);
	g_return_if_fail (item != NULL);

	data = (GeditPanelItem *)g_object_get_data (G_OBJECT (item),
						    PANEL_ITEM_KEY);
	g_return_if_fail (data != NULL);

	sync_title (panel, data);
	sync_toolbar (panel, data);
}

static void
panel_show (GeditPanel *panel,
	    gpointer    user_data)
{
	gint page;
	GtkNotebook *nb;

	nb = GTK_NOTEBOOK (panel->priv->notebook);

	page = gtk_notebook_get_current_page (nb);

	if (page != -1)
		notebook_page_changed (nb, NULL, page, panel);
}

static void
close_button_clicked_cb (GtkWidget *widget,
			 GtkWidget *panel)
{
	gtk_widget_hide (panel);
}

static void
gedit_panel_init (GeditPanel *panel)
{
	panel->priv = GEDIT_PANEL_GET_PRIVATE (panel);
	g_return_if_fail (panel->priv != NULL);	
	
	panel->priv->tooltips = gtk_tooltips_new ();
	g_object_ref (G_OBJECT (panel->priv->tooltips));
	gtk_object_sink (GTK_OBJECT (panel->priv->tooltips));
}

static GtkWidget *
create_small_button (GtkWidget *image)
{
	/* This is quite ugly and sometimes don't give good results */
	gint w, h;
	GtkWidget *button = gtk_button_new ();	
	gtk_widget_show (image);		    
	gtk_button_set_relief (GTK_BUTTON (button),
			       GTK_RELIEF_NONE);

	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
	gtk_widget_set_size_request (image, w, h); 
	gtk_widget_set_size_request (button, w + 4, h + 4);
	gtk_container_add (GTK_CONTAINER (button), image);
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
	return button;
}

static void
build_notebook_for_panel (GeditPanel *panel)
{
	/* Create the panel notebook */
	panel->priv->notebook = gtk_notebook_new ();

	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (panel->priv->notebook),
				  GTK_POS_BOTTOM);
  	gtk_notebook_set_scrollable (GTK_NOTEBOOK (panel->priv->notebook),
  				     TRUE);
  	gtk_notebook_popup_enable (GTK_NOTEBOOK (panel->priv->notebook));
  
  	gtk_widget_show (GTK_WIDGET (panel->priv->notebook));
  
  	g_signal_connect (panel->priv->notebook,
  			  "switch-page",
  			  G_CALLBACK (notebook_page_changed),
  			  panel);  
}

static void
build_horizontal_panel (GeditPanel *panel)
{
	GtkWidget *box;
	GtkWidget *sidebar;
	GtkWidget *close_button;
	GtkWidget *image;
	
	box = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start (GTK_BOX (box), 
			    panel->priv->notebook, 
			    TRUE, 
			    TRUE, 
			    0);

	/* Toolbar, close button and first separator */
	sidebar = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (sidebar), 4);

	gtk_box_pack_start (GTK_BOX (box),
			    sidebar,
			    FALSE, 
			    FALSE, 
			    0);

	image = gtk_image_new_from_stock ("gtk-close", GTK_ICON_SIZE_MENU);
	close_button = create_small_button (image);

	gtk_box_pack_start (GTK_BOX (sidebar),
			    close_button,
			    FALSE, 
			    FALSE, 
			    0);

	gtk_tooltips_set_tip (panel->priv->tooltips,
			      close_button,
			      _("Hide panel"),
			      NULL);

	g_signal_connect (G_OBJECT (close_button),
			  "clicked",
                          G_CALLBACK (close_button_clicked_cb),
                          panel);

	panel->priv->toolbar_container = sidebar;
	panel->priv->toolbar = NULL;

	gtk_widget_show_all (box);

	gtk_box_pack_start (GTK_BOX (panel),
			    box,
			    TRUE,
			    TRUE,
			    0);
}

static void
build_vertical_panel (GeditPanel *panel)
{
	GtkWidget *close_button;
	GtkWidget *title_hbox;
	GtkWidget *icon_name_hbox;
	GtkWidget *image;
	GtkWidget *dummy_label;
	
	/* Create title hbox */
	title_hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (title_hbox), 5);
					
	gtk_box_pack_start (GTK_BOX (panel), title_hbox, FALSE, FALSE, 0);
	
	icon_name_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (title_hbox), 
			    icon_name_hbox, 
			    TRUE, 
			    TRUE, 
			    0);
		
	panel->priv->title_image = 
				gtk_image_new_from_stock (GTK_STOCK_FILE,
							  GTK_ICON_SIZE_MENU); 							     
	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    panel->priv->title_image, 
			    FALSE, 
			    TRUE, 
			    0);

	dummy_label = gtk_label_new (" ");
		
	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    dummy_label, 
			    FALSE, 
			    FALSE, 
			    0);	
				    
	panel->priv->title_label = gtk_label_new (_("Empty"));
	gtk_misc_set_alignment (GTK_MISC (panel->priv->title_label), 0, 0.5);
	gtk_label_set_ellipsize(GTK_LABEL (panel->priv->title_label), PANGO_ELLIPSIZE_END);

	gtk_box_pack_start (GTK_BOX (icon_name_hbox),
			    panel->priv->title_label,
			    TRUE,
			    TRUE,
			    0);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	close_button = create_small_button (image);
				       
	gtk_box_pack_start (GTK_BOX (title_hbox),
			    close_button, 
			    FALSE, 
			    FALSE, 
			    0);

	gtk_tooltips_set_tip (panel->priv->tooltips,
			      close_button,
			      _("Hide panel"),
			      NULL);

	g_signal_connect (G_OBJECT (close_button), 
			  "clicked",
                          G_CALLBACK (close_button_clicked_cb),
                          panel);
                 
	gtk_widget_show_all (title_hbox);

	panel->priv->toolbar_container = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (panel),
			    panel->priv->toolbar_container,
			    FALSE,
			    FALSE,
			    0);
	gtk_widget_show (panel->priv->toolbar_container);
	panel->priv->toolbar = NULL;

	gtk_box_pack_start (GTK_BOX (panel),
			    panel->priv->notebook,
			    TRUE,
			    TRUE,
			    0);
}

static GObject *
gedit_panel_constructor (GType type,
			 guint n_construct_properties,
			 GObjectConstructParam *construct_properties)
{
	
	/* Invoke parent constructor. */
	GeditPanelClass *klass = GEDIT_PANEL_CLASS (g_type_class_peek (GEDIT_TYPE_PANEL));
	GObjectClass *parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	GObject *obj = parent_class->constructor (type,
						  n_construct_properties,
						  construct_properties);

	/* Build the panel, now that we know the orientation 
			   (_init has been called previously) */
	GeditPanel *panel = GEDIT_PANEL (obj);

	build_notebook_for_panel (panel);
  	if (panel->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
  		build_horizontal_panel (panel);
	else
		build_vertical_panel (panel);

	g_signal_connect (panel,
			  "show",
			  G_CALLBACK (panel_show),
			  NULL);

	return obj;
}


GtkWidget *
gedit_panel_new (GtkOrientation orientation)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_PANEL, "orientation", orientation, NULL));
}

static GtkWidget *
build_tab_label (GeditPanel  *panel,
		 GtkWidget   *item,
		 const gchar *name,
		 GtkWidget   *icon)
{
	GtkWidget *hbox, *label_hbox, *label_ebox;
	GtkWidget *label;

	/* set hbox spacing and label padding (see below) so that there's an
	 * equal amount of space around the label */
	hbox = gtk_hbox_new (FALSE, 4);

	label_ebox = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (label_ebox), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), label_ebox, TRUE, TRUE, 0);

	label_hbox = gtk_hbox_new (FALSE, 4);
	gtk_container_add (GTK_CONTAINER (label_ebox), label_hbox);

	/* setup icon */
	gtk_box_pack_start (GTK_BOX (label_hbox), icon, FALSE, FALSE, 0);

	/* setup label */
        label = gtk_label_new (name);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (label), 0, 0);
	gtk_box_pack_start (GTK_BOX (label_hbox), label, TRUE, TRUE, 0);

	gtk_tooltips_set_tip (panel->priv->tooltips,
			      label_ebox,
			      name,
			      NULL);

	gtk_widget_show_all (hbox);

	if (panel->priv->orientation == GTK_ORIENTATION_VERTICAL)
		gtk_widget_hide(label);

	g_object_set_data (G_OBJECT (item), "label", label);
	g_object_set_data (G_OBJECT (item), "hbox", hbox);
	g_object_set_data (G_OBJECT (item), "label-ebox", label_ebox);

	return hbox;
}

void
gedit_panel_add_item (GeditPanel  *panel, 
		      GtkWidget   *item, 
		      const gchar *name,
		      GtkWidget   *image)
{
	GeditPanelItem *data;
	GtkWidget *tab_label;
	GtkWidget *menu_label;

	g_return_if_fail (GEDIT_IS_PANEL (panel));
	g_return_if_fail (GTK_IS_WIDGET (item));
	g_return_if_fail (name != NULL);
	g_return_if_fail (image == NULL || GTK_IS_IMAGE (image));

	data = g_new (GeditPanelItem, 1);

	data->name = g_strdup (name);

	if (image == NULL)
	{
		/* default to empty */
		data->icon = gtk_image_new_from_stock (GTK_STOCK_FILE,
						       GTK_ICON_SIZE_MENU);
	}
	else
	{
		data->icon = image;
	}

	data->toolbar = NULL;

	g_object_set_data (G_OBJECT (item),
		           PANEL_ITEM_KEY,
		           data);

	tab_label = build_tab_label (panel, item, data->name, data->icon);

	menu_label = gtk_label_new (name);
	gtk_misc_set_alignment (GTK_MISC (menu_label), 0.0, 0.5);

	if (!GTK_WIDGET_VISIBLE (item))
		gtk_widget_show (item);

	gtk_notebook_append_page_menu (GTK_NOTEBOOK (panel->priv->notebook),
				       item,
				       tab_label,
				       menu_label);
}

void
gedit_panel_add_item_with_stock_icon (GeditPanel  *panel, 
				      GtkWidget   *item, 
				      const gchar *name,
				      const gchar *stock_id)
{
	GtkWidget *icon = NULL;

	if (stock_id != NULL)
	{
		icon = gtk_image_new_from_stock (stock_id,
						 GTK_ICON_SIZE_MENU);
	}

	gedit_panel_add_item (panel, item, name, icon);
}

gboolean
gedit_panel_remove_item (GeditPanel *panel,
			 GtkWidget  *item)
{
	GeditPanelItem *data;
	gint page_num;
	GtkWidget *ebox;
	
	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);

	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (panel->priv->notebook),
					  item);
					  
	if (page_num == -1)
		return FALSE;
		
	data = (GeditPanelItem *)g_object_get_data (G_OBJECT (item),
					            PANEL_ITEM_KEY);
	g_return_val_if_fail (data != NULL, FALSE);
	
	g_free (data->name);
	g_free (data);

	g_object_set_data (G_OBJECT (item),
		           PANEL_ITEM_KEY,
		           NULL);

	ebox = g_object_get_data (G_OBJECT (item), "label-ebox");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (panel->priv->tooltips), 
			      ebox, 
			      NULL, 
			      NULL);  
	
	if (data->toolbar != NULL)
	{
		if (panel->priv->toolbar == data->toolbar)
		{
			gtk_container_remove (GTK_CONTAINER (panel->priv->toolbar_container),
					      panel->priv->toolbar);
			panel->priv->toolbar = NULL;
		}
		gtk_widget_destroy (data->toolbar);
	}
	
	gtk_notebook_remove_page (GTK_NOTEBOOK (panel->priv->notebook), 
				  page_num);

	/* if we removed all the pages, reset the title */
	if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (panel->priv->notebook)) == 0)
	{
		sync_title (panel, NULL);
		sync_toolbar (panel, NULL);
	}

	return TRUE;
}

gboolean
gedit_panel_activate_item (GeditPanel *panel,
			   GtkWidget  *item)
{
	gint page_num;

	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);

	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (panel->priv->notebook),
					  item);

	if (page_num == -1)
		return FALSE;

	gtk_notebook_set_current_page (GTK_NOTEBOOK (panel->priv->notebook),
				       page_num);

	return TRUE;
}

gboolean
gedit_panel_item_is_active (GeditPanel *panel,
			    GtkWidget  *item)
{
	gint cur_page;
	gint page_num;

	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);

	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (panel->priv->notebook),
					  item);

	if (page_num == -1)
		return FALSE;

	cur_page = gtk_notebook_get_current_page (
				GTK_NOTEBOOK (panel->priv->notebook));

	return (page_num == cur_page);
}

GtkOrientation
gedit_panel_get_orientation (GeditPanel *panel)
{
	return panel->priv->orientation;
}
