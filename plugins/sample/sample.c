/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 * Copyright (C) 1999-2001 Alex Roberts, Evan Lawrence, Jason Leach & Jose M Celorio
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * AUTHORS: 
 *   Alex Roberts <bse@error.fsnet.co.uk>
 *
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "document.h"
#include "utils.h"
#include "view.h"
#include "plugin.h"

static void
destroy_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");
}


/**
 * insert_hello:
 * @void: 
 * 
 * The function that actualy does the work.
 **/
static void
insert_hello (void)
{
	GeditView *view = gedit_view_active();

	gedit_debug (DEBUG_PLUGINS, "");

	if (!view)
	     return;

	gedit_document_insert_text (view->doc, "Hello World", gedit_view_get_position (view), TRUE);
	
}
	

gint
init_plugin (PluginData *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Hello World");
	pd->desc = _("Sample 'hello world' plugin.");
	pd->long_desc = _("Sample 'hello world' plugin.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = TRUE;

	pd->private_data = (gpointer)insert_hello;
	
	return PLUGIN_OK;
}




