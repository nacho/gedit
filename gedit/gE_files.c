/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "gE_window.h"
#include "gE_view.h"
#include "gE_files.h"
#include "commands.h"
/*#include "toolbar.h"*/
#include "gE_mdi.h"

static void clear_text (gE_view *view);


static void
clear_text (gE_view *view)
{
	gint i = gE_view_get_length (view);

	if (i > 0) {
		gE_view_set_position (view, i);
		gtk_text_backward_delete (GTK_TEXT(view->text), i);
	}
}


/*
 * PUBLIC: gE_file_open
 *
 * opens the file and reads it into the text wigdget.
 *
 * TODO - lock/unlock file before/after
 */
gint
gE_file_open (gE_document *doc, gchar *fname)
{
	char *nfile, *name;
	gchar *tmp_buf, *flash;
	struct stat stats;
	gint i;
	gE_view *nth_view;
	FILE *fp;

	name = fname;

	if (!stat(fname, &stats) && S_ISREG(stats.st_mode))
	{
		doc->buf_size = stats.st_size;
		if ((tmp_buf = g_new0 (gchar, doc->buf_size + 1)) != NULL) {
			if ((doc->filename = g_strdup (fname)) != NULL) {
                                /*gE_file_open (GE_DOCUMENT(doc));*/
				if ((fp = fopen (fname, "r")) != NULL) {
					doc->buf_size = fread (tmp_buf, 1, doc->buf_size,fp);
					doc->buf = g_string_new (tmp_buf);
					g_free (tmp_buf);
					gnome_mdi_child_set_name(GNOME_MDI_CHILD(doc), g_basename(fname));
					fclose (fp);
					for (i = 0; i < g_list_length (doc->views); i++) {
						nth_view = g_list_nth_data (doc->views, i);
						gE_view_refresh (nth_view);
                                                /* Make the document readonly if you can't write to the file. */
						gE_view_set_read_only (nth_view, access (fname, W_OK) != 0);
						if (!nth_view->changed_id)
							nth_view->changed_id =	gtk_signal_connect (GTK_OBJECT(nth_view->text), "changed",
												    GTK_SIGNAL_FUNC(view_changed_cb), nth_view);
					}
					flash = g_strdup_printf("%s %s",_(MSGBAR_FILE_OPENED), fname);
					gnome_app_flash(mdi->active_window, flash);
					g_free(flash);
					doc->changed = FALSE;
					recent_add (doc->filename);
					recent_update (GNOME_APP (mdi->active_window));
					return 0;
				}
				else
				{
					gnome_app_error(mdi->active_window, _("Can't open file!"));
					return NULL;
				}
			}
		}
	}
	return 1;
} /* gE_file_open */


/*
 * PUBLIC: gE_file_save
 *
 * saves the file.  uses a single fputs() call to save the file in one
 * sell fwoop.
 *
 * TODO - lock/unlock file before/after
 */
gint
gE_file_save(gE_document *doc, gchar *fname)
{

	FILE *fp;
	gchar *title;
	gchar *tmpstr;
	gE_view *view = GE_VIEW(mdi->active_view);

	/* FIXME: not sure what to do with all the gE_window refs.. 
	          i'll comment them out for now.. 				    */

	g_assert(doc != NULL);
	g_assert(fname != NULL);

	if ((fp = fopen (fname, "w")) == NULL) {
	
	  g_warning ("Can't open file %s for saving", fname);
	  
	  return 1;
	  
	}
	
	gtk_text_thaw (GTK_TEXT(view->text));

/*	 We dont need to get the chars, as the chars are there in the 
  	  public buffer..
  	  
	tmpstr = gtk_editable_get_chars (GTK_EDITABLE (view->text), 0,
		gtk_text_get_length (GTK_TEXT (view->text)));
*/	
	/* sync the buffer */
		gE_view_buffer_sync (view);
	
			if (fputs (view->document->buf->str, fp) == EOF) {
	
	  perror("Error saving file");
	  fclose(fp);
	  /*g_free (tmpstr);*/
	  
	  return 1;
	
	}
	
	/*g_free (tmpstr);*/
	
	if (fclose(fp) != 0) {
	
	  perror("Error saving file");
	  
	  return 1;
	  
	}

	if (doc->filename != fname) {
	
	  g_free(doc->filename);
	  doc->filename = g_strdup(fname);
	  
	}
	
	doc->changed = FALSE;
	
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc), g_basename(doc->filename));

	tmpstr = g_malloc0 (strlen (GEDIT_ID) +
					   strlen (GNOME_MDI_CHILD (gE_document_current())->name) + 4);
	sprintf (tmpstr, "%s - %s", GNOME_MDI_CHILD (gE_document_current())->name, GEDIT_ID);
	gtk_window_set_title(GTK_WINDOW(mdi->active_window), tmpstr);
	g_free(tmpstr);

	if (!view->changed_id)
	  view->changed_id =	gtk_signal_connect (GTK_OBJECT(view->text), "changed",
									    GTK_SIGNAL_FUNC(view_changed_cb), view);

	gnome_app_flash (mdi->active_window, _(MSGBAR_FILE_SAVED));
	
	return 0;
	
} /* gE_file_save */
