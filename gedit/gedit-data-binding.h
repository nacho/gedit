/*
 * gedit-data-binding.h
 *
 * Copyright (C) 2009 - Jesse van den Kieboom
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

#ifndef __GEDIT_DATA_BINDING_H__
#define __GEDIT_DATA_BINDING_H__

#include <glib-object.h>

typedef struct _GeditDataBinding GeditDataBinding;

typedef gboolean (*GeditDataBindingConversion) (GValue const *source,
						GValue *dest,
						gpointer userdata);

GeditDataBinding	*gedit_data_binding_new		(gpointer source,
							 const gchar *source_property,
							 gpointer dest,
							 const gchar *dest_property);

GeditDataBinding	*gedit_data_binding_new_full	(gpointer source,
							 const gchar *source_property,
							 gpointer dest,
							 const gchar *dest_property,
							 GeditDataBindingConversion conversion,
							 gpointer userdata);

GeditDataBinding	*gedit_data_binding_new_mutual	(gpointer source,
							 const gchar *source_property,
							 gpointer dest,
							 const gchar *dest_property);

GeditDataBinding	*gedit_data_binding_new_mutual_full
							(gpointer source,
							 const gchar *source_property,
							 gpointer dest,
							 const gchar *dest_property,
							 GeditDataBindingConversion source_to_dest,
							 GeditDataBindingConversion dest_to_source,
							 gpointer userdata);

void			 gedit_data_binding_free	(GeditDataBinding *binding);

/* conversion utilities */
gboolean		 gedit_data_binding_color_to_string
							(GValue const *color,
							 GValue *string,
							 gpointer userdata);
gboolean		 gedit_data_binding_string_to_color
							(GValue const *string,
							 GValue *color,
							 gpointer userdata);

#endif /* __GEDIT_DATA_BINDING_H__ */

