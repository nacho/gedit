/*
 * gedit-data-binding.c
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

#include "gedit-data-binding.h"

#include <gdk/gdk.h>

typedef struct
{
	GObject *object;
	
	gchar *property;
	GType type;

	guint notify_id;
	GeditDataBindingConversion conversion;
	gpointer userdata;
} Binding;

typedef enum
{
	GEDIT_DATA_BINDING_NONE = 0,
	GEDIT_DATA_BINDING_MUTUAL = 1 << 0
} GeditDataBindingFlags;

struct _GeditDataBinding
{
	Binding source;
	Binding dest;
	GeditDataBindingFlags flags;
};

static void on_data_binding_destroy (GeditDataBinding *binding, GObject *source);
static void gedit_data_binding_finalize (GeditDataBinding *binding);

static void on_data_binding_changed (GObject *source, GParamSpec *spec, GeditDataBinding *binding);

static void
binding_connect (GeditDataBinding *binding,
		 Binding *bd)
{
	gchar *nid = g_strconcat ("notify::", bd->property, NULL);
	bd->notify_id = g_signal_connect_after (bd->object, nid,
						G_CALLBACK(on_data_binding_changed),
						binding);
	g_free(nid);
}

static void
binding_fill (Binding *binding,
	      gpointer object,
	      const gchar *property,
	      GType type,
	      GeditDataBindingConversion conversion,
	      gpointer userdata)
{
	binding->object = G_OBJECT (object);
	binding->property = g_strdup (property);
	binding->type = type;
	binding->conversion = conversion ? conversion : (GeditDataBindingConversion)g_value_transform;
	binding->userdata = userdata;
}

static GeditDataBinding *
gedit_data_binding_create (gpointer source,
			   const gchar *source_property,
			   gpointer dest, const gchar *dest_property,
			   GeditDataBindingConversion source_to_dest,
			   GeditDataBindingConversion dest_to_source,
			   gpointer userdata,
			   GeditDataBindingFlags flags)
{
	GeditDataBinding *binding;
	GObjectClass *sclass;
	GObjectClass *dclass;
	GParamSpec *sspec;
	GParamSpec *dspec;

	g_return_val_if_fail (G_IS_OBJECT (source), NULL);
	g_return_val_if_fail (G_IS_OBJECT (dest), NULL);
	
	sclass = G_OBJECT_GET_CLASS (source);
	dclass = G_OBJECT_GET_CLASS (dest);
	
	sspec = g_object_class_find_property (sclass, source_property);
	
	if (!sspec)
	{
		g_warning("No such source property found: %s", source_property);
		return NULL;
	}
	
	dspec = g_object_class_find_property (dclass, dest_property);
	
	if (!dspec)
	{
		g_warning("No such dest property found: %s", dest_property);
		return NULL;
	}
	
	binding = g_slice_new0 (GeditDataBinding);
	
	binding->flags = flags;
	
	binding_fill (&binding->source, source, source_property,
		      G_PARAM_SPEC_VALUE_TYPE(sspec), source_to_dest, userdata);
	binding_fill (&binding->dest, dest, dest_property,
		      G_PARAM_SPEC_VALUE_TYPE(dspec), dest_to_source, userdata);
	
	binding_connect(binding, &binding->source);
	
	if (flags & GEDIT_DATA_BINDING_MUTUAL)
		binding_connect (binding, &binding->dest);
	
	g_object_weak_ref (binding->source.object,
			   (GWeakNotify)on_data_binding_destroy, binding);
	g_object_weak_ref (binding->dest.object,
			   (GWeakNotify)on_data_binding_destroy, binding);
	
	/* initial value */
	on_data_binding_changed (binding->source.object, NULL, binding);
	
	return binding;
}

GeditDataBinding *
gedit_data_binding_new_full (gpointer source,
			     const gchar *source_property,
			     gpointer dest,
			     const gchar *dest_property,
			     GeditDataBindingConversion conversion,
			     gpointer userdata)
{
	return gedit_data_binding_create (source, source_property,
					  dest, dest_property,
					  conversion, NULL,
					  userdata,
					  GEDIT_DATA_BINDING_NONE);
}

GeditDataBinding *
gedit_data_binding_new (gpointer source,
			const gchar *source_property,
			gpointer dest,
			const gchar *dest_property)
{
	return gedit_data_binding_new_full (source, source_property,
					    dest, dest_property,
					    NULL, NULL);
}

GeditDataBinding *
gedit_data_binding_new_mutual_full (gpointer source,
				    const gchar *source_property,
				    gpointer dest,
				    const gchar *dest_property,
				    GeditDataBindingConversion source_to_dest,
				    GeditDataBindingConversion dest_to_source,
				    gpointer userdata)
{
	return gedit_data_binding_create (source, source_property,
					  dest, dest_property,
					  source_to_dest, dest_to_source,
					  userdata,
					  GEDIT_DATA_BINDING_MUTUAL);
}

GeditDataBinding *
gedit_data_binding_new_mutual (gpointer source,
			       const gchar *source_property,
			       gpointer dest,
			       const gchar *dest_property)
{
	return gedit_data_binding_new_mutual_full (source, source_property,
						   dest, dest_property,
						   NULL, NULL,
						   NULL);
}
static void
gedit_data_binding_finalize (GeditDataBinding *binding)
{
	g_free (binding->source.property);
	g_free (binding->dest.property);

	g_slice_free (GeditDataBinding, binding);
}

void
gedit_data_binding_free (GeditDataBinding *binding)
{
	if (binding->source.notify_id)
		g_signal_handler_disconnect (binding->source.object,
					     binding->source.notify_id);

	if (binding->dest.notify_id)
		g_signal_handler_disconnect (binding->dest.object,
					     binding->dest.notify_id);

	g_object_weak_unref (binding->source.object,
			     (GWeakNotify)on_data_binding_destroy, binding);
	g_object_weak_unref (binding->dest.object,
			     (GWeakNotify)on_data_binding_destroy, binding);
	
	gedit_data_binding_finalize (binding);
}

static void
on_data_binding_destroy (GeditDataBinding *binding,
			 GObject *object)
{
	Binding *bd = binding->source.object == object ? &binding->dest : &binding->source;
	
	/* disconnect notify handler */
	if (bd->notify_id)
		g_signal_handler_disconnect (bd->object, bd->notify_id);
	
	/* remove weak ref */
	g_object_weak_unref (bd->object, (GWeakNotify)on_data_binding_destroy,
			     binding);

	/* finalize binding */
	gedit_data_binding_finalize (binding);
}

static void 
on_data_binding_changed (GObject *object,
			 GParamSpec *spec,
			 GeditDataBinding *binding)
{
	Binding *source = binding->source.object == object ? &binding->source : &binding->dest;
	Binding *dest = binding->source.object == object ? &binding->dest : &binding->source;

	/* Transmit to dest */
	GValue value = { 0, };
	g_value_init (&value, dest->type);
	
	GValue svalue = { 0, };
	g_value_init (&svalue, source->type);
	
	g_object_get_property (source->object, source->property, &svalue);
	g_object_get_property (dest->object, dest->property, &value);
	
	if (source->conversion (&svalue, &value, source->userdata))
	{
		if (dest->notify_id)
			g_signal_handler_block (dest->object, dest->notify_id);

		g_object_set_property (dest->object, dest->property, &value);

		if (dest->notify_id)
			g_signal_handler_unblock (dest->object, dest->notify_id);
	}
	
	g_value_unset (&value);
	g_value_unset (&svalue);
}

/* conversion utilities */
gboolean
gedit_data_binding_color_to_string (GValue const *color,
				    GValue *string,
				    gpointer userdata)
{
	GdkColor *clr = g_value_get_boxed (color);
	gchar *s = gdk_color_to_string (clr);
	
	g_value_take_string (string, s);
	
	return TRUE;
}

gboolean
gedit_data_binding_string_to_color (GValue const *string,
				    GValue *color,
				    gpointer userdata)
{
	const gchar *s = g_value_get_string (string);
	GdkColor clr;
	
	gdk_color_parse (s, &clr);
	g_value_set_boxed (color, &clr);
	
	return TRUE;
}

