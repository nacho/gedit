/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*-
 * vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts, Evan Lawrence, and Chris Lahey
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GE_PLUGIN_API_H__
#define __GE_PLUGIN_API_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  
#include "plugin.h"

extern GHashTable *doc_int_to_pointer;
extern GHashTable *doc_pointer_to_int;
extern GHashTable *win_int_to_pointer;
extern GHashTable *win_pointer_to_int;
extern int last_assigned_integer;

#include "gE_plugin.h"

extern void gE_plugin_program_register (plugin_info *info);
extern void add_plugins_to_window (plugin_info *info, gE_window *window);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GE_PLUGIN_API_H__ */
