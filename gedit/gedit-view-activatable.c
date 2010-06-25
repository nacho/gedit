/*
 * gedit-view-activatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 Steve Frécinaux
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

#include "gedit-view-activatable.h"

/**
 * SECTION:gedit-view-activatable
 * @short_description: Interface for extensions activatable on views
 * @see_also: #PeasExtensionSet
 *
 * #GeditViewActivatable is an interface which should be implemented by
 * extensions that should be activated on a gedit view (or the document it
 * contains).
 **/
G_DEFINE_INTERFACE(GeditViewActivatable, gedit_view_activatable, G_TYPE_OBJECT)

void
gedit_view_activatable_default_init (GeditViewActivatableInterface *iface)
{
}

/**
 * gedit_view_activatable_activate:
 * @activatable: A #GeditViewActivatable.
 * @view: The #GeditView on which the plugin should be activated.
 *
 * Activates the extension on the given view.
 */
void
gedit_view_activatable_activate (GeditViewActivatable *activatable,
				   GeditView            *view)
{
	GeditViewActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_VIEW_ACTIVATABLE (activatable));
	g_return_if_fail (GEDIT_IS_VIEW (view));

	iface = GEDIT_VIEW_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->activate != NULL)
	{
		iface->activate (activatable, view);
	}
}

/**
 * gedit_view_activatable_deactivate:
 * @activatable: A #GeditViewActivatable.
 * @view: A #GeditView.
 *
 * Deactivates the plugin on the given view.
 */
void
gedit_view_activatable_deactivate (GeditViewActivatable *activatable,
				     GeditView            *view)
{
	GeditViewActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_VIEW_ACTIVATABLE (activatable));
	g_return_if_fail (GEDIT_IS_VIEW (view));

	iface = GEDIT_VIEW_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->deactivate != NULL)
	{
		iface->deactivate (activatable, view);
	}
}
