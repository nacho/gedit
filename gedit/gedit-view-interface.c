/*
 * gedit-view.c
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
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


#include "gedit-view-interface.h"
#include <gtk/gtk.h>

/* Signals */
enum
{
	DROP_URIS,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL] = { 0 };

/* Default implementation */
static const gchar *
gedit_view_get_name_default (GeditView *view)
{
	g_return_val_if_reached (NULL);
}

static GeditDocument *
gedit_view_get_document_default (GeditView *view)
{
	g_return_val_if_reached (NULL);
}

static void
gedit_view_set_editable_default (GeditView *view,
				 gboolean   setting)
{
	g_return_if_reached ();
}

static gboolean
gedit_view_get_editable_default (GeditView *view)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_view_get_overwrite_default (GeditView   *view)
{
	g_return_val_if_reached (FALSE);
}

static void
gedit_view_cut_clipboard_default (GeditView *view)
{
	g_return_if_reached ();
}

static void
gedit_view_copy_clipboard_default (GeditView *view)
{
	g_return_if_reached ();
}

static void
gedit_view_paste_clipboard_default (GeditView *view)
{
	g_return_if_reached ();
}

static void
gedit_view_delete_selection_default (GeditView *view)
{
	g_return_if_reached ();
}

static void
gedit_view_select_all_default (GeditView *view)
{
	g_return_if_reached ();
}

static void
gedit_view_scroll_to_cursor_default (GeditView *view)
{
	g_return_if_reached ();
}

static void
gedit_view_set_font_default (GeditView *view,
			     gboolean     def,
			     const gchar *font_name)
{
	g_return_if_reached ();
}

static void 
gedit_view_init (GeditViewIface *iface)
{
	static gboolean initialized = FALSE;
	
	iface->get_name = gedit_view_get_name_default;
	iface->get_document = gedit_view_get_document_default;
	iface->set_editable = gedit_view_set_editable_default;
	iface->get_editable = gedit_view_get_editable_default;
	iface->get_overwrite = gedit_view_get_overwrite_default;
	iface->cut_clipboard = gedit_view_cut_clipboard_default;
	iface->copy_clipboard = gedit_view_copy_clipboard_default;
	iface->paste_clipboard = gedit_view_paste_clipboard_default;
	iface->delete_selection = gedit_view_delete_selection_default;
	iface->select_all = gedit_view_select_all_default;
	iface->scroll_to_cursor = gedit_view_scroll_to_cursor_default;
	iface->set_font = gedit_view_set_font_default;
	
	if (!initialized)
	{
		initialized = TRUE;
		
		g_object_interface_install_property (iface,
						     g_param_spec_boolean ("editable",
									   "Editable",
									   "Whether the text can be modified by the user",
									   TRUE,
									   G_PARAM_READWRITE));
		
		/* A new signal DROP_URIS has been added to allow plugins to intercept
		 * the default dnd behaviour of 'text/uri-list'. GeditTextView now handles
		 * dnd in the default handlers of drag_drop, drag_motion and 
		 * drag_data_received. The view emits drop_uris from drag_data_received
		 * if valid uris have been dropped. Plugins should connect to 
		 * drag_motion, drag_drop and drag_data_received to change this 
		 * default behaviour. They should _NOT_ use this signal because this
		 * will not prevent gedit from loading the uri
		 */
		view_signals[DROP_URIS] =
			g_signal_new ("drop_uris",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
				      G_STRUCT_OFFSET (GeditViewIface, drop_uris),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__BOXED,
				      G_TYPE_NONE, 1, G_TYPE_STRV);
	}
}

GType 
gedit_view_get_type ()
{
	static GType gedit_view_type_id = 0;
	
	if (!gedit_view_type_id)
	{
		static const GTypeInfo g_define_type_info =
		{
			sizeof (GeditViewIface),
			(GBaseInitFunc) gedit_view_init, 
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		gedit_view_type_id = 
			g_type_register_static (G_TYPE_INTERFACE,
						"GeditView",
						&g_define_type_info,
						0);
	
		g_type_interface_add_prerequisite (gedit_view_type_id,
						   GTK_TYPE_WIDGET);
	}
	
	return gedit_view_type_id;
}

/**
 * gedit_view_get_name:
 * @view: a #GeditView
 *
 * Gets a name for @view. For example: "Text Editor"
 *
 * Returns: a name for @view.
 */
const gchar *
gedit_view_get_name (GeditView *view)
{
	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);
	return GEDIT_VIEW_GET_INTERFACE (view)->get_name (view);
}

/**
 * gedit_view_get_document:
 * @view: a #GeditView
 */
GeditDocument *
gedit_view_get_document (GeditView *view)
{
	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);
	
	return GEDIT_VIEW_GET_INTERFACE (view)->get_document (view);
}

void
gedit_view_set_editable (GeditView *view,
			 gboolean   setting)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->set_editable (view, setting);
}

gboolean
gedit_view_get_editable (GeditView *view)
{
	g_return_val_if_fail (GEDIT_IS_VIEW (view), FALSE);
	return GEDIT_VIEW_GET_INTERFACE (view)->get_editable (view);
}

gboolean
gedit_view_get_overwrite (GeditView   *view)
{
	g_return_val_if_fail (GEDIT_IS_VIEW (view), FALSE);
	return GEDIT_VIEW_GET_INTERFACE (view)->get_overwrite (view);
}

/**
 * gedit_view_cut_clipboard:
 * @view: a #GeditView
 */
void
gedit_view_cut_clipboard (GeditView *view)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->cut_clipboard (view);
}

/**
 * gedit_view_copy_clipboard:
 * @view: a #GeditView
 */
void
gedit_view_copy_clipboard (GeditView *view)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->copy_clipboard (view);
}

/**
 * gedit_view_paste_clipboard:
 * @view: a #GeditView
 */
void
gedit_view_paste_clipboard (GeditView *view)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->paste_clipboard (view);
}

/**
 * gedit_view_delete_selection:
 * @view: a #GeditView
 * 
 * Deletes the text currently selected in the #GtkTextBuffer associated
 * to the view and scroll to the cursor position.
 **/
void
gedit_view_delete_selection (GeditView *view)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->delete_selection (view);
}

/**
 * gedit_view_select_all:
 * @view: a #GeditView
 * 
 * Selects all the text displayed in the @view.
 **/
void
gedit_view_select_all (GeditView *view)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->select_all (view);
}

/**
 * gedit_view_scroll_to_cursor:
 * @view: a #GeditView
 * 
 * Scrolls the @view to the cursor position.
 **/
void
gedit_view_scroll_to_cursor (GeditView *view)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->scroll_to_cursor (view);
}

/**
 * gedit_view_set_font:
 * @view: a #GeditView
 * @def: whether to reset the default font
 * @font_name: the name of the font to use
 * 
 * If @def is #TRUE, resets the font of the @view to the default font
 * otherwise sets it to @font_name.
 **/
void
gedit_view_set_font (GeditView   *view,
		     gboolean     def,
		     const gchar *font_name)
{
	g_return_if_fail (GEDIT_IS_VIEW (view));
	GEDIT_VIEW_GET_INTERFACE (view)->set_font (view, def, font_name);
}
