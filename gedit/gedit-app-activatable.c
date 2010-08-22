/*
 * gedit-app-activatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 Steve Frécinaux
 * Copyright (C) 2010 Jesse van den Kieboom
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-app-activatable.h"
#include "gedit-app.h"

/**
 * SECTION:gedit-app-activatable
 * @short_description: Interface for activatable extensions on apps
 * @see_also: #PeasExtensionSet
 *
 * #GeditAppActivatable is an interface which should be implemented by
 * extensions that should be activated on a gedit application.
 **/

G_DEFINE_INTERFACE(GeditAppActivatable, gedit_app_activatable, G_TYPE_OBJECT)

void
gedit_app_activatable_default_init (GeditAppActivatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized)
	{
		/**
		 * GeditAppActivatable:app:
		 *
		 * The app property contains the gedit app for this
		 * #GeditAppActivatable instance.
		 */
		g_object_interface_install_property (iface,
		                                     g_param_spec_object ("app",
		                                                          "App",
		                                                          "The gedit app",
		                                                          GEDIT_TYPE_APP,
		                                                          G_PARAM_READWRITE |
		                                                          G_PARAM_CONSTRUCT_ONLY |
		                                                          G_PARAM_STATIC_STRINGS));

		initialized = TRUE;
	}
}

/**
 * gedit_app_activatable_activate:
 * @activatable: A #GeditAppActivatable.
 *
 * Activates the extension on the application.
 */
void
gedit_app_activatable_activate (GeditAppActivatable *activatable)
{
	GeditAppActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_APP_ACTIVATABLE (activatable));

	iface = GEDIT_APP_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->activate != NULL)
	{
		iface->activate (activatable);
	}
}

/**
 * gedit_app_activatable_deactivate:
 * @activatable: A #GeditAppActivatable.
 *
 * Deactivates the extension from the application.
 *
 */
void
gedit_app_activatable_deactivate (GeditAppActivatable *activatable)
{
	GeditAppActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_APP_ACTIVATABLE (activatable));

	iface = GEDIT_APP_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->deactivate != NULL)
	{
		iface->deactivate (activatable);
	}
}
