/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-recent.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 James Willcox
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

/*
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-recent.h"
#include "gedit-prefs.h"


static GnomeRecentModel *model;


GnomeRecentModel *
gedit_recent_get_model (void)
{
	return model;
}

void
gedit_recent_init (void)
{
	model = gnome_recent_model_new ("gedit-2", gedit_settings->max_recents);
}
