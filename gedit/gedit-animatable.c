/*
 * gedit-animatable.c
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
 *
 * gedit is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "gedit-animatable.h"
#include "gedit-overlay-child.h"
#include "theatrics/gedit-theatrics-choreographer.h"
#include "theatrics/gedit-theatrics-enum-types.h"

G_DEFINE_INTERFACE(GeditAnimatable, gedit_animatable, GEDIT_TYPE_OVERLAY_CHILD)

void
gedit_animatable_default_init (GeditAnimatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized)
	{
		g_object_interface_install_property (iface,
		                                     g_param_spec_enum ("easing",
		                                                        "Easing",
		                                                        "The Easing",
		                                                        GEDIT_TYPE_THEATRICS_CHOREOGRAPHER_EASING,
		                                                        GEDIT_THEATRICS_CHOREOGRAPHER_EASING_LINEAR,
		                                                        G_PARAM_READWRITE |
		                                                        G_PARAM_CONSTRUCT |
		                                                        G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
		                                     g_param_spec_enum ("blocking",
		                                                        "Blocking",
		                                                        "The Blocking",
		                                                        GEDIT_TYPE_THEATRICS_CHOREOGRAPHER_BLOCKING,
		                                                        GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE,
		                                                        G_PARAM_READWRITE |
		                                                        G_PARAM_CONSTRUCT |
		                                                        G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
		                                     g_param_spec_enum ("animation-state",
		                                                        "Animation State",
		                                                        "The Animation State",
		                                                        GEDIT_TYPE_THEATRICS_ANIMATION_STATE,
		                                                        GEDIT_THEATRICS_ANIMATION_STATE_COMING,
		                                                        G_PARAM_READWRITE |
		                                                        G_PARAM_CONSTRUCT |
		                                                        G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
		                                     g_param_spec_uint ("duration",
		                                                        "Duration",
		                                                        "The duration",
		                                                        0,
		                                                        G_MAXUINT,
		                                                        300,
		                                                        G_PARAM_READWRITE |
		                                                        G_PARAM_CONSTRUCT |
		                                                        G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
		                                     g_param_spec_double ("percent",
		                                                          "Percent",
		                                                          "The percent",
		                                                          0.0,
		                                                          G_MAXDOUBLE,
		                                                          0.0,
		                                                          G_PARAM_READWRITE |
		                                                          G_PARAM_CONSTRUCT |
		                                                          G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
		                                     g_param_spec_double ("bias",
		                                                          "Bias",
		                                                          "The bias",
		                                                          0.0,
		                                                          G_MAXDOUBLE,
		                                                          1.0,
		                                                          G_PARAM_READWRITE |
		                                                          G_PARAM_STATIC_STRINGS));

		initialized = TRUE;
	}
}

/* XXX: too lazy to add the methods, should we or can we survive only with props? */

/* ex:set ts=8 noet: */
