/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-window-server.c
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

#include "gedit-window-server.h"
#include "gedit-document-server.h"
#include "GNOME_Gedit.h"
#include "gedit-file.h"
#include "gedit-mdi.h"
#include "gedit2.h"

static void gedit_window_server_class_init (GeditWindowServerClass *klass);
static void gedit_window_server_init (GeditWindowServer *a);
static void gedit_window_server_object_finalize (GObject *object);
static GObjectClass *gedit_window_server_parent_class;

BonoboObject *
gedit_window_server_new (BonoboWindow *win)
{
	GeditWindowServer *server;

	server = g_object_new (GEDIT_WINDOW_SERVER_TYPE, NULL);
	server->win = win;

	return BONOBO_OBJECT (server);
}

static void
impl_gedit_window_server_openURIList (PortableServer_Servant _servant,
				   const GNOME_Gedit_URIList * uris,
				   CORBA_Environment * ev)
{
	GList *list = NULL;
	guint i;

	/* convert from CORBA_sequence into GList */
	for (i=0; i < uris->_length; i++) {
		list = g_list_append (list, g_strdup (uris->_buffer[i]));
	}

	if (list) {
		gedit_file_open_uri_list (list, 0, TRUE);
	}

	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

static GNOME_Gedit_Document
impl_gedit_window_server_newDocument (PortableServer_Servant _servant,
                                CORBA_Environment * ev)
{
	GeditDocument *doc;
	BonoboObject *doc_server;

	gedit_file_new ();

	doc = gedit_get_active_document ();

	doc_server = gedit_document_server_new (doc);

	return BONOBO_OBJREF (doc_server);
}

static void
impl_gedit_window_server_grabFocus (PortableServer_Servant _servant,
					 CORBA_Environment * ev)
{
	BonoboWindow *window;

	window = gedit_get_active_window ();
	gtk_window_present (GTK_WINDOW (window));
}

static void
gedit_window_server_class_init (GeditWindowServerClass *klass)
{
        GObjectClass *object_class = (GObjectClass *) klass;
        POA_GNOME_Gedit_Window__epv *epv = &klass->epv;

        gedit_window_server_parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = gedit_window_server_object_finalize;

        /* connect implementation callbacks */
	epv->newDocument = impl_gedit_window_server_newDocument;
	epv->openURIList = impl_gedit_window_server_openURIList;
	epv->grabFocus   = impl_gedit_window_server_grabFocus;
}

static void
gedit_window_server_init (GeditWindowServer *c) 
{
}

static void
gedit_window_server_object_finalize (GObject *object)
{
        GeditWindowServer *a = GEDIT_WINDOW_SERVER (object);

        gedit_window_server_parent_class->finalize (G_OBJECT (a));
}

BONOBO_TYPE_FUNC_FULL (
        GeditWindowServer,                    
        GNOME_Gedit_Window, 
        BONOBO_TYPE_OBJECT,           
        gedit_window_server);
