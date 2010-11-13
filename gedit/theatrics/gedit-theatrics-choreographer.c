/*
 * gedit-theatrics-choreographer.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Scott Peterson <lunchtimemama@gmail.com> work.
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <math.h>
#include "gedit-theatrics-choreographer.h"

gint
gedit_theatrics_choreographer_pixel_compose (gdouble                           percent,
					     gint                              size,
					     GeditTheatricsChoreographerEasing easing)
{
	return (gint)round (gedit_theatrics_choreographer_compose_with_scale (percent, size, easing));
}

gdouble
gedit_theatrics_choreographer_compose_with_scale (gdouble                           percent,
						  gdouble                           scale,
						  GeditTheatricsChoreographerEasing easing)
{
	return scale * gedit_theatrics_choreographer_compose (percent, easing);
}

gdouble
gedit_theatrics_choreographer_compose (gdouble                           percent,
				       GeditTheatricsChoreographerEasing easing)
{
	g_return_val_if_fail (percent >= 0.0 && percent <= 1.0, 0.0);

	switch (easing)
	{
		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_IN:
			return percent * percent;

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_OUT:
			return -1.0 * percent * (percent - 2.0);

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_IN_OUT:
			percent *= 2.0;
			return percent < 1.0
				? percent * percent * 0.5
				: -0.5 * ((percent - 1.0) * (percent - 2.0) - 1.0);

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN:
			return pow (2.0, 10.0 * (percent - 1.0));

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_OUT:
			return -pow (2.0, -10.0 * percent) + 1.0;

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN_OUT:
			percent *= 2.0;
			return percent < 1.0
				? 0.5 * pow (2.0, 10.0 * (percent - 1.0))
				: 0.5 * (-pow (2.0, -10.0 * (percent - 1.0)) + 2.0);

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_SINE:
			return sin (percent * G_PI);

		case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_LINEAR:
		default:
			return percent;
	}
}

/* ex:set ts=8 noet: */
