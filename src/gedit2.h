/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit2.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence, Jason Leach
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
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

#ifndef __GEDIT2_H__
#define __GEDIT2_H__

#include "gedit-mdi.h"
#include "gedit-document.h"
#include "gedit-view.h" 

extern GeditMDI* gedit_mdi;
extern gboolean gedit_close_x_button_pressed;
extern gboolean gedit_exit_button_pressed;

BonoboWindow* gedit_get_active_window ();
GeditDocument* gedit_get_active_document ();
GeditView* gedit_get_active_view ();

#endif /* __GEDIT2_H__ */



