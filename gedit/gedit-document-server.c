/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-document-server.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-context.h>

#include "gedit-document-server.h"
#include "GNOME_Gedit.h"
#include "gedit-file.h"
#include "gedit-document.h"
#include "gedit-mdi.h"
#include "gedit2.h"

static void gedit_document_server_class_init (GeditDocumentServerClass *klass);
static void gedit_document_server_init (GeditDocumentServer *a);
static void gedit_document_server_object_finalize (GObject *object);

static GObjectClass *gedit_document_server_parent_class;

BonoboObject *
gedit_document_server_new (GeditDocument *doc)
{
	GeditDocumentServer *doc_server;

	g_return_val_if_fail (doc != NULL, NULL);

	doc_server = g_object_new (GEDIT_DOCUMENT_SERVER_TYPE, NULL);

	doc_server->doc = doc;

	return BONOBO_OBJECT (doc_server);
}

static void
impl_gedit_document_server_setLinePosition (PortableServer_Servant _servant,
					    CORBA_long position,
					    CORBA_Environment * ev)
{
	GeditDocumentServer *doc_server;


	doc_server = GEDIT_DOCUMENT_SERVER (bonobo_object_from_servant (_servant));

	gedit_document_goto_line (doc_server->doc, position);
}

static void
impl_gedit_document_server_insert (PortableServer_Servant _servant,
				   const CORBA_long offset,
				   const CORBA_char *str,
				   const CORBA_long len,
				   CORBA_Environment *ev)
{
	GeditDocumentServer *doc_server;

	doc_server = GEDIT_DOCUMENT_SERVER (bonobo_object_from_servant (_servant));

	if (gedit_document_is_readonly (doc_server->doc)) {
		g_warning ("Doc is readonly.  Sending exception.");
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_GNOME_Gedit_Document_DocumentReadOnly,
				     NULL);
		return;
	}
		
	gedit_document_insert_text (doc_server->doc, (gint)offset,
				    (const gchar *)str, (gint)len);
}

static void
impl_gedit_document_server_delete (PortableServer_Servant _servant,
				   const CORBA_long offset,
				   const CORBA_long len,
				   CORBA_Environment *ev)
{
	GeditDocumentServer *doc_server;

	doc_server = GEDIT_DOCUMENT_SERVER (bonobo_object_from_servant (_servant));

	if (gedit_document_is_readonly (doc_server->doc)) {
		g_warning ("Doc is readonly.  Sending exception.");
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_GNOME_Gedit_Document_DocumentReadOnly,
				     NULL);
		return;
	}

	gedit_document_delete_text (doc_server->doc, (gint)offset, (gint)len);
}

static CORBA_char *
impl_gedit_document_server_getChars (PortableServer_Servant _servant,
				     const CORBA_long offset,
				     const CORBA_long len,
				     CORBA_Environment *ev)
{
	CORBA_char *ret;
	gchar *chars;
	GeditDocumentServer *doc_server;
	
	doc_server = GEDIT_DOCUMENT_SERVER (bonobo_object_from_servant (_servant));

	chars = gedit_document_get_chars (doc_server->doc, (gint)offset,
					  (gint)offset+(gint)len);

	if (chars == NULL)
		return NULL;

	ret = CORBA_string_dup (chars);

	g_free (chars);

	return ret;
}

static CORBA_long
impl_gedit_document_server_getCharCount (PortableServer_Servant _servant,
					 CORBA_Environment *ev)
{
	GeditDocumentServer *doc_server;
	CORBA_long len;

	doc_server = GEDIT_DOCUMENT_SERVER (bonobo_object_from_servant (_servant));

	len = gedit_document_get_char_count (doc_server->doc);

	return len;
}

static void
gedit_document_server_class_init (GeditDocumentServerClass *klass)
{
        GObjectClass *object_class = (GObjectClass *) klass;
        POA_GNOME_Gedit_Document__epv *epv = &klass->epv;

        gedit_document_server_parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = gedit_document_server_object_finalize;

        /* connect implementation callbacks */
	epv->setLinePosition = impl_gedit_document_server_setLinePosition;
	epv->insert          = impl_gedit_document_server_insert;
	epv->delete          = impl_gedit_document_server_delete;
	epv->getChars        = impl_gedit_document_server_getChars;
	epv->getCharCount    = impl_gedit_document_server_getCharCount;
}

static void
gedit_document_server_init (GeditDocumentServer *c) 
{
}

static void
gedit_document_server_object_finalize (GObject *object)
{
        GeditDocumentServer *a = GEDIT_DOCUMENT_SERVER (object);

        gedit_document_server_parent_class->finalize (G_OBJECT (a));
}

BONOBO_TYPE_FUNC_FULL (
        GeditDocumentServer,                    
        GNOME_Gedit_Document, 
        BONOBO_TYPE_OBJECT,           
        gedit_document_server);
