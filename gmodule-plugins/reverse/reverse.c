/* reverse.c - reverse plugin.
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

#include <string.h>
#include <gtk/gtk.h>

#include "client.h"
#include <glib.h>

int main( int argc, char *argv[] )
{
  int docid;
  int length;
  int context;
  int i;
  gchar *buff;
  client_info info;

  info.menu_location = "[Plugins]Reverse";

  context = client_init( &argc, &argv, &info );

  docid = client_document_current( context );
  length = strlen( buff = client_text_get( docid ) );

  for( i=0; i < ( length / 2 ); i++ )
    {
      char temp = buff[i];
      buff[i] = buff[length - i - 1];
      buff[length - i - 1] = temp;
    }
  docid = client_document_new( context, "reversed file" );
  client_text_append( docid, buff, length );
  
  client_document_show( docid );
  client_finish( context );
  
  exit(0);
}
