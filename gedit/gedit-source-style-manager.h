/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-source-style-manager.h
 *
 * Copyright (C) 2007 - Paolo Borelli
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
 *
 * $Id: gedit-source-style-manager.h 5598 2007-04-15 13:16:24Z pborelli $
 */

#ifndef __GEDIT_SOURCE_STYLE_MANAGER_H__
#define __GEDIT_SOURCE_STYLE_MANAGER_H__

#include <gtksourceview/gtksourcestylemanager.h>

G_BEGIN_DECLS

GtkSourceStyleManager	*gedit_get_source_style_manager			(void);

GtkSourceStyleScheme	*gedit_source_style_manager_get_default_scheme	(GtkSourceStyleManager	*manager);

/*
 * Non exported functions
 */
gboolean		_gedit_source_style_manager_set_default_scheme	(GtkSourceStyleManager	*manager,
									 const gchar		*scheme_id);

gboolean		_gedit_source_style_manager_scheme_is_gedit_user_scheme
									(GtkSourceStyleManager	*manager,
									const gchar		*scheme_id);

G_END_DECLS

#endif /* __GEDIT_SOURCE_STYLE_MANAGER_H__ */
