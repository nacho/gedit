/*
 * gedit-theatrics-utils.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Mike Krüger <mkrueger@novell.com> work.
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-theatrics-utils.h"

void
gedit_theatrics_utils_draw_round_rectangle (cairo_t *cr,
					    gboolean top_left_round,
					    gboolean top_right_round,
					    gboolean bottom_left_round,
					    gboolean bottom_right_round,
					    gdouble  x,
					    gdouble  y,
					    gdouble  r,
					    gdouble  w,
					    gdouble  h)
{
	/*
	   UA****BQ
	   H      C
	   *      *
	   G      D
	   TF****ES
	*/

	cairo_new_path (cr);

	if (top_left_round)
	{
		/* Move to A */
		cairo_move_to (cr, x + r, y);
	}
	else
	{
		/* Move to U */
		cairo_move_to (cr, x, y);
	}

	if (top_right_round)
	{
		/* Straight line to B */
		cairo_line_to (cr, x + w - r, y);

		/* Curve to C, Control points are both at Q */
		cairo_curve_to (cr, x + w, y, 
		                x + w, y,
		                x + w, y + r);
	}
	else
	{
		/* Straight line to Q */
		cairo_line_to (cr, x + w, y);
	}

	if (bottom_right_round)
	{
		/* Move to D */
		cairo_line_to (cr, x + w, y + h - r);

		/* Curve to E */
		cairo_curve_to (cr, x + w, y + h,
		                x + w, y + h,
		                x + w - r, y + h);
	}
	else
	{
		/* Move to S */
		cairo_line_to (cr, x + w, y + h);
	}

	if (bottom_left_round)
	{
		/* Line to F */
		cairo_line_to (cr, x + r, y + h);

		/* Curve to G */
		cairo_curve_to (cr, x, y + h, 
		                x, y + h , 
		                x, y + h - r);
	}
	else
	{
		/* Line to T */
		cairo_line_to (cr, x, y + h);
	}

	if (top_left_round)
	{
		/* Line to H */
		cairo_line_to (cr, x, y + r);

		/* Curve to A */
		cairo_curve_to (cr, x, y,
		                x , y, 
		                x + r, y);
	}
	else
	{
		/* Line to U */
		cairo_line_to (cr, x, y);
	}

	cairo_close_path (cr);
}

/* ex:set ts=8 noet: */
