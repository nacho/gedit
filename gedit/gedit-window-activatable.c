/*
 * gedit-window-activatable.h
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

#include "gedit-window-activatable.h"

/**
 * SECTION:gedit-window-activatable
 * @short_description: Interface for extensions activatable on windows
 * @see_also: #PeasExtensionSet
 *
 * #GeditWindowActivatable is an interface which should be implemented by
 * extensions that should be activated on a gedit main window.
 **/
G_DEFINE_INTERFACE(GeditWindowActivatable, gedit_window_activatable, G_TYPE_OBJECT)

void
gedit_window_activatable_default_init (GeditWindowActivatableInterface *iface)
{
}

/**
 * gedit_window_activatable_activate:
 * @activatable: A #GeditWindowActivatable.
 * @window: The #GeditWindow on which the plugin should be activated.
 *
 * Activates the extension on the given window.
 */
void
gedit_window_activatable_activate (GeditWindowActivatable *activatable,
				   GeditWindow            *window)
{
	GeditWindowActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_WINDOW_ACTIVATABLE (activatable));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	iface = GEDIT_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->activate != NULL)
	{
		iface->activate (activatable, window);
	}
}

/**
 * gedit_window_activatable_deactivate:
 * @activatable: A #GeditWindowActivatable.
 * @window: A #GeditWindow.
 *
 * Deactivates the plugin on the given window.
 */
void
gedit_window_activatable_deactivate (GeditWindowActivatable *activatable,
				     GeditWindow            *window)
{
	GeditWindowActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_WINDOW_ACTIVATABLE (activatable));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	iface = GEDIT_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->deactivate != NULL)
	{
		iface->deactivate (activatable, window);
	}
}

/**
 * gedit_window_activatable_update_state:
 * @activatable: A #GeditWindowActivatable.
 * @window: A #GeditWindow.
 *
 * Triggers an update of the plugin insternal state to take into account
 * state changes in the window state, due to a plugin or an user action.
 */
void
gedit_window_activatable_update_state (GeditWindowActivatable *activatable,
				       GeditWindow            *window)
{
	GeditWindowActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_WINDOW_ACTIVATABLE (activatable));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	iface = GEDIT_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->update_state != NULL)
	{
		iface->update_state (activatable, window);
	}
}
