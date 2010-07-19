/*
 * gedit-web-view.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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


#ifndef __GEDIT_WEB_VIEW_H__
#define __GEDIT_WEB_VIEW_H__

#include <webkit/webkit.h>
#include "gedit-view-interface.h"
#include "gedit-document.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_WEB_VIEW		(gedit_web_view_get_type ())
#define GEDIT_WEB_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_WEB_VIEW, GeditWebView))
#define GEDIT_WEB_VIEW_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_WEB_VIEW, GeditWebView const))
#define GEDIT_WEB_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_WEB_VIEW, GeditWebViewClass))
#define GEDIT_IS_WEB_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_WEB_VIEW))
#define GEDIT_IS_WEB_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_WEB_VIEW))
#define GEDIT_WEB_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_WEB_VIEW, GeditWebViewClass))

typedef struct _GeditWebView		GeditWebView;
typedef struct _GeditWebViewClass	GeditWebViewClass;
typedef struct _GeditWebViewPrivate	GeditWebViewPrivate;

struct _GeditWebView
{
	WebKitWebView parent;

	GeditWebViewPrivate *priv;
};

struct _GeditWebViewClass
{
	WebKitWebViewClass parent_class;
};

GType		 gedit_web_view_get_type	(void) G_GNUC_CONST;

GeditView	*gedit_web_view_new		(GeditDocument *doc);


G_END_DECLS

#endif /* __GEDIT_WEB_VIEW_H__ */
