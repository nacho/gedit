/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-application-server.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 James Willcox
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */


#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-context.h>

#include "gedit-application-server.h"
#include "gedit-document-server.h"
#include "gedit-window-server.h"
#include "GNOME_Gedit.h"
#include "gedit-file.h"
#include "gedit-mdi.h"
#include "gedit2.h"


static void gedit_application_server_class_init (GeditApplicationServerClass *klass);
static void gedit_application_server_init (GeditApplicationServer *a);
static void gedit_application_server_object_finalize (GObject *object);
static GObjectClass *gedit_application_server_parent_class;

static BonoboObject *
gedit_application_server_factory (BonoboGenericFactory *this_factory,
			   const char *iid,
			   gpointer user_data)
{
        GeditApplicationServer *a;
        
        a  = g_object_new (GEDIT_APPLICATION_SERVER_TYPE, NULL);

        return BONOBO_OBJECT (a);
}

BonoboObject *
gedit_application_server_new (void)
{
	BonoboGenericFactory *factory;

	factory = bonobo_generic_factory_new ("OAFIID:GNOME_Gedit_Factory",
					      gedit_application_server_factory,
					      NULL);

	return BONOBO_OBJECT (factory);
}

static GNOME_Gedit_Window
impl_gedit_application_server_newWindow (PortableServer_Servant _servant,
					 CORBA_Environment * ev)
{
	BonoboWindow *win;
	BonoboObject *win_server;

	bonobo_mdi_open_toplevel (BONOBO_MDI (gedit_mdi), NULL);
	gedit_file_new ();

	/* let the UI update */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	win = bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi));

	win_server = gedit_window_server_new (win);

	return BONOBO_OBJREF (win_server);
}


static GNOME_Gedit_Window
impl_gedit_application_server_getActiveWindow (PortableServer_Servant _servant,
					       CORBA_Environment * ev)
{
	BonoboWindow *win;
	BonoboObject *win_server;

	win = bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi));

	win_server = gedit_window_server_new (win);

	return BONOBO_OBJREF (win_server);
}

static GNOME_Gedit_Window
impl_gedit_application_server_getActiveDocument (PortableServer_Servant _servant,
						 CORBA_Environment * ev)
{
	GeditDocument *doc;
	BonoboObject *doc_server;

	doc = gedit_get_active_document ();

	doc_server = gedit_document_server_new (doc);

	return BONOBO_OBJREF (doc_server);
}

static void
impl_gedit_application_server_quit (PortableServer_Servant _servant,
                                CORBA_Environment * ev)
{
	gedit_file_exit ();
}

static void
gedit_application_server_class_init (GeditApplicationServerClass *klass)
{
        GObjectClass *object_class = (GObjectClass *) klass;
        POA_GNOME_Gedit_Application__epv *epv = &klass->epv;

        gedit_application_server_parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = gedit_application_server_object_finalize;

        /* connect implementation callbacks */
	epv->newWindow         = impl_gedit_application_server_newWindow;
	epv->getActiveDocument = impl_gedit_application_server_getActiveDocument;
	epv->getActiveWindow   = impl_gedit_application_server_getActiveWindow;

	epv->quit              = impl_gedit_application_server_quit;
}

static void
gedit_application_server_init (GeditApplicationServer *c) 
{
}

static void
gedit_application_server_object_finalize (GObject *object)
{
        GeditApplicationServer *a = GEDIT_APPLICATION_SERVER (object);

        gedit_application_server_parent_class->finalize (G_OBJECT (a));
}

BONOBO_TYPE_FUNC_FULL (
        GeditApplicationServer,                    
        GNOME_Gedit_Application, 
        BONOBO_TYPE_OBJECT,           
        gedit_application_server);
