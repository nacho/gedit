/* client.h - libraries for acting as a plugin.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#ifndef __CLIENT_H__
#define __CLIENT_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
  
#include <glib.h>

typedef struct
{
  gchar *menu_location;
} client_info;

gint client_init( gint *argc, gchar **argv[], client_info *info );
gint client_document_current( gint context );
gchar *client_document_filename( gint docid );
gint client_document_new( gint context, gchar *title );
gint client_document_open( gint context, gchar *title );
void client_document_close( gint docid );
void client_text_append( gint docid, gchar *buff, gint length );
void client_document_show( gint docid );
void client_finish( gint context );
gchar *client_text_get( gint docid );
gboolean client_program_quit();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CLIENT_H__ */
