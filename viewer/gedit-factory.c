/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-factory.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 Jeroen Zwartepoorte
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <libbonobo.h>
#include <bonobo/bonobo-shlib-factory.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include "gedit-viewer.h"

static BonoboObject *
gedit_factory (BonoboGenericFactory *factory,
	       const char           *component_id,
	       gpointer              closure)
{
	BonoboControl *control = NULL;

	if (!gnome_vfs_initialized ())
		if (!gnome_vfs_init ())
			return NULL;

	if (!strcmp (component_id, "OAFIID:GNOME_Gedit_Control"))
		control = gedit_viewer_new ();

	return BONOBO_OBJECT (control);
}

BONOBO_ACTIVATION_SHLIB_FACTORY ("OAFIID:GNOME_Gedit_ViewerFactory",
				 N_("Gedit viewer factory"),
				 gedit_factory, NULL);
