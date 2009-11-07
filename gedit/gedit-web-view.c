/*
 * gedit-web-view.c
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


#include "gedit-web-view.h"
#include "gedit-document-interface.h"
#include <glib/gi18n.h>

#define GEDIT_WEB_VIEW_NAME _("Web View")

#define GEDIT_WEB_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_WEB_VIEW, GeditWebViewPrivate))

struct _GeditWebViewPrivate
{
};

static void	gedit_view_iface_init		(GeditViewIface  *iface);

G_DEFINE_TYPE_WITH_CODE (GeditWebView, 
			 gedit_web_view, 
			 WEBKIT_TYPE_WEB_VIEW,
			 G_IMPLEMENT_INTERFACE (GEDIT_TYPE_VIEW,
			 			gedit_view_iface_init))

static void
gedit_web_view_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_web_view_parent_class)->finalize (object);
}

static void
gedit_web_view_class_init (GeditWebViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = gedit_web_view_finalize;

	/*g_type_class_add_private (object_class, sizeof (GeditWebViewPrivate));*/
}

static void
gedit_web_view_init (GeditWebView *self)
{
	/*self->priv = GEDIT_WEB_VIEW_GET_PRIVATE (self);*/
	
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (self), "about:blank");
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (self), "http://www.gedit.org");
}

static const gchar *
gedit_view_get_name_impl (GeditView *view)
{
	return GEDIT_WEB_VIEW_NAME;
}

static GeditDocument *
gedit_view_get_document_impl (GeditView *view)
{
	return GEDIT_DOCUMENT (view);
}

static void
gedit_view_set_editable_impl (GeditView *view,
			      gboolean setting)
{
	webkit_web_view_set_editable (WEBKIT_WEB_VIEW (view), setting);
}

static gboolean
gedit_view_get_editable_impl (GeditView *view)
{
	return webkit_web_view_get_editable (WEBKIT_WEB_VIEW (view));
}

static gboolean
gedit_view_get_overwrite_impl (GeditView *view)
{
	return FALSE;
}

static void
gedit_view_cut_clipboard_impl (GeditView *view)
{
}

static void
gedit_view_copy_clipboard_impl (GeditView *view)
{
}

static void
gedit_view_paste_clipboard_impl (GeditView *view)
{
}

static void
gedit_view_delete_selection_impl (GeditView *view)
{
}

static void
gedit_view_select_all_impl (GeditView *view)
{
}

static void
gedit_view_scroll_to_cursor_impl (GeditView *view)
{
}

static void
gedit_view_set_font_impl (GeditView   *view, 
			  gboolean     def, 
			  const gchar *font_name)
{
}

static void
gedit_view_iface_init (GeditViewIface *iface)
{
	iface->get_name = gedit_view_get_name_impl;
	iface->get_document = gedit_view_get_document_impl;
	iface->set_editable = gedit_view_set_editable_impl;
	iface->get_editable = gedit_view_get_editable_impl;
	iface->get_overwrite = gedit_view_get_overwrite_impl;
	iface->cut_clipboard = gedit_view_cut_clipboard_impl;
	iface->copy_clipboard = gedit_view_copy_clipboard_impl;
	iface->paste_clipboard = gedit_view_paste_clipboard_impl;
	iface->delete_selection = gedit_view_delete_selection_impl;
	iface->select_all = gedit_view_select_all_impl;
	iface->scroll_to_cursor = gedit_view_scroll_to_cursor_impl;
	iface->set_font = gedit_view_set_font_impl;
}

GeditView *
gedit_web_view_new ()
{
	return GEDIT_VIEW (g_object_new (GEDIT_TYPE_WEB_VIEW, NULL));
}
