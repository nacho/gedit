/* gEdit
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
  
  int gE_plugin_create( gint context, gchar *title );
  void gE_plugin_append( gint docid, gchar *buffer, gint length );
  void gE_plugin_show( gint docid );
  int gE_plugin_current( gint context );
  char *gE_plugin_filename( gint docid );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GE_PLUGIN_API_H__ */
