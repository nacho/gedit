/*
 * gedit-theatrics-utils.h
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

#ifndef __GEDIT_THEATRICS_UTILS_H__
#define __GEDIT_THEATRICS_UTILS_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

void		gedit_theatrics_utils_draw_round_rectangle	(cairo_t *cr,
								 gboolean top_left_round,
								 gboolean top_right_round,
								 gboolean bottom_left_round,
								 gboolean bottom_right_round,
								 gdouble  x,
								 gdouble  y,
								 gdouble  r,
								 gdouble  w,
								 gdouble  h);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_UTILS_H__ */

/* ex:set ts=8 noet: */
