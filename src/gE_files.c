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
gE_file_open(gE_document *doc, gchar *fname)
{
	int fd;
	gchar *buf, *title, *flash;
	size_t size, bytesread, num;
	gE_view *view = GE_VIEW(mdi->active_view);
	int pos;

	/* get file stats (e.g., file size is used by files list window) */
	doc->sb = g_malloc(sizeof(struct stat));
	if (stat(fname, doc->sb) == -1) {
		/* need to print warning */
		g_free(doc->sb);
		doc->sb = NULL;
	}

	if (doc->sb && doc->sb->st_size > 0) {

		/* open file */
		if ((fd = open(fname, O_RDONLY)) == -1) {
			char msg[STRING_LENGTH_MAX];

			sprintf(msg, "gE_file_open could not open %s", fname);
			perror(msg);
			return 1;
		}

		gtk_text_freeze(GTK_TEXT(view->text));
		clear_text(view);

		/*
		 * the more aggressive/intelligent method is to try to allocate
		 * a buffer to fit all the contents of the file; that way, we
		 * only read the file once.  basically, we try to allocate a
		 * buffer that is twice the number of bytes that is needed.
		 * this is because gtk_text_insert() does a memcpy() of the
		 * read buffer into the text widget's buffer, so we must ensure
		 * that there is enough memory for both the read and the
		 * memcpy().  since we'd like to read the entire file at once,
		 * so we start with a buffer that is twice the filesize.  if
		 * this fails (e.g., we're reading a really large file), we
		 * keep reducing the size by half until we are able to get a
		 * buffer.
		 */
		size = 2 * (doc->sb->st_size + 1);
		while ((buf = (char *)g_malloc(size)) == NULL)
			size /= 2;
#ifdef DEBUG
		printf("size %lu is ok (using half)\n", (gulong)size);
#endif
		g_free(buf);
		size /= 2;
		if ((buf = (char *)g_malloc(size + 1)) == NULL) {
			perror("gE_file_open: unable to malloc read buffer");
			close(fd);
			return 1;
		}

#ifdef DEBUG
		printf("malloc'd %lu bytes for read buf\n", (gulong)(size+1));
#endif
		/* buffer allocated, now actually read the file */
		bytesread = 0;
		while (bytesread < doc->sb->st_size) {
			num = read(fd, buf, size);
			if (num > 0) {
				gtk_text_insert(GTK_TEXT(view->text), NULL,
					&view->text->style->black,
					NULL, buf, num);
				bytesread += num;
			} else if (num == -1) {
				/* need to print warning/error message */
				perror("read error");
				break;
			} else /* if (num == 0) */ {
#ifdef DEBUG
				printf("hmm that's odd... read zero bytes");
#endif
			}
		}
		g_free(buf);
		close(fd);
		gtk_text_thaw(GTK_TEXT(view->text));
	} /* filesize > 0 */

	/* misc settings */
	doc->filename = g_malloc0 (strlen (fname) + 1);
	doc->filename = g_strdup (fname);
	
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc), g_basename(fname));
	
	gE_view_set_position (view, 0);

	/* Copy the buffer if split-screening enabled */
/*	if (doc->split_screen) {*/

		buf = gtk_editable_get_chars(GTK_EDITABLE(view->text), 0, -1);
		pos = 0;
		view->flag = view->split_screen;
		gtk_text_freeze(GTK_TEXT(view->split_screen));
		gtk_editable_insert_text(GTK_EDITABLE(view->split_screen),
			buf, strlen(buf), &pos);
		gtk_text_thaw(GTK_TEXT(view->split_screen));
		g_free (buf);
/*	}
*/
	
	/* enable document change detection */
	view->changed = FALSE;
	if (!view->changed_id)
	  view->changed_id =	gtk_signal_connect(GTK_OBJECT(view->text),"changed", 
				                       GTK_SIGNAL_FUNC(view_changed_cb),view);


	flash = g_strdup_printf("%s %s",_(MSGBAR_FILE_OPENED), fname);
	gnome_app_flash(mdi->active_window, flash);
	g_free(flash);
		
	recent_add (doc->filename);
	recent_update (GNOME_APP (mdi->active_window));

	/* Make the document readonly if you can't write to the file. */
	gE_view_set_read_only (view, access (fname, W_OK) != 0);
	
	return 0;
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

	tmpstr = gtk_editable_get_chars (GTK_EDITABLE (view->text), 0,
		gtk_text_get_length (GTK_TEXT (view->text)));
	if (fputs (tmpstr, fp) == EOF)
	  {
	   perror("Error saving file");
	   fclose(fp);
	   g_free (tmpstr);
	   return 1;
	  }
	g_free (tmpstr);
	if (fclose(fp) != 0)
	  {
	   perror("Error saving file");
	   return 1;
	  }

	if (doc->filename != fname)
	  {
	    g_free(doc->filename);
	    doc->filename = g_strdup(fname);
	  }
	doc->changed = FALSE;
	
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc), g_basename(doc->filename));

	tmpstr = g_malloc0 (strlen (GEDIT_ID) +
					   strlen (GNOME_MDI_CHILD (gE_document_current())->name) + 4);
	    sprintf (tmpstr, "%s - %s",
		   GNOME_MDI_CHILD (gE_document_current())->name,
		   GEDIT_ID);
	    gtk_window_set_title(GTK_WINDOW(mdi->active_window), tmpstr);
	    g_free(tmpstr);

	if (!view->changed_id)
	  view->changed_id =	gtk_signal_connect (GTK_OBJECT(view->text), "changed",
									    GTK_SIGNAL_FUNC(view_changed_cb), view);

	gnome_app_flash (mdi->active_window, _(MSGBAR_FILE_SAVED));
	
	return 0;
} /* gE_file_save */
