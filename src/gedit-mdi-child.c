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

#include "gedit-mdi-child.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-marshal.h"

struct _GeditMDIChildPrivate
{
	GtkWidget *tab_label;
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
}

static void 
gedit_mdi_child_init (GeditMDIChild  *child)
{
	gedit_debug (DEBUG_MDI, "START");

	child->priv = g_new0 (GeditMDIChildPrivate, 1);

	child->priv->tab_label = NULL;

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
			tab_name = g_strdup_printf ("RO - %s", docname);
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
