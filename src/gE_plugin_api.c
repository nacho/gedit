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

#include <gtk/gtk.h>
#include "main.h"
#include "gE_plugin_api.h"

int gE_plugin_create( gchar *title )
{
  return GPOINTER_TO_INT( gE_document_new(main_window) );
}

void gE_plugin_append( gint docid, gchar *buffer, gint length )
{
  gE_document *document = (gE_document *) GINT_TO_POINTER( docid );
  GtkText *text = GTK_TEXT( document->text );
  gtk_text_freeze( text );
  gtk_text_set_point( text, gtk_text_get_length( text ) );
  gtk_text_insert( text, NULL, NULL, NULL, buffer, length );
  gtk_text_thaw( text );
  document->changed = 1;
}

void gE_plugin_show( gint docid )
{
}

int gE_plugin_current()
{
  return GPOINTER_TO_INT( gE_document_current(main_window) );
}

gchar *gE_plugin_filename( gint docid )
{
  gE_document *document = (gE_document *) GINT_TO_POINTER( docid );
  if (document->filename == NULL)
  	return "";
  else
  	return document->filename;
}
