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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "gE_document.h"
#include "gE_files.h"
#include "commands.h"
#include "toolbar.h"
#include "msgbox.h"

static void flw_update_entry(gE_window *w, gE_document *, int, char *text);
static void flw_select_file(GtkWidget *, gint, gint, GdkEventButton *,
	gpointer data);
static void clear_text (gE_document *doc);


static void
clear_text (gE_document *doc)
{
	int i = gtk_text_get_length (GTK_TEXT(doc->text));

	if (i > 0) {
		gtk_text_set_point (GTK_TEXT(doc->text), i);
		gtk_text_backward_delete (GTK_TEXT(doc->text), i);
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
gE_file_open(gE_window *w, gE_document *doc, gchar *fname)
{
	char *buf;
	int fd;
	gchar *title;
	size_t size, bytesread, num;

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

		gtk_text_freeze(GTK_TEXT(doc->text));
		clear_text(doc);

#ifdef USE_SIMPLE_READ_ALGORITHM
#define READ_CHUNK	32767
		/*
		 * the simple read algorithm basically just reads in READ_CHUNK
		 * bytes at a time.  we can still do better, but this a >>lot<<
		 * better than before, where we were reading 10 bytes at a
		 * time!  ideally, we'd like to alloc a buffer to read the
		 * entire file in at once, or a buffer as large as possible if
		 * the file is really really large.
		 */
		size = READ_CHUNK;
		buf = (char *)g_malloc(size+1);
#else
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
		while ((buf = (char *)malloc(size)) == NULL)
			size /= 2;
#ifdef DEBUG
		printf("size %lu is ok (using half)\n", (gulong)size);
#endif
		free(buf);
		size /= 2;
		if ((buf = (char *)malloc(size + 1)) == NULL) {
			perror("gE_file_open: unable to malloc read buffer");
			close(fd);
			return 1;
		}
#endif	/* ifdef USE_CONSERVATIVE_READ_ALGORITHM */

#ifdef DEBUG
		printf("malloc'd %lu bytes for read buf\n", (gulong)(size+1));
#endif
		/* buffer allocated, now actually read the file */
		bytesread = 0;
		while (bytesread < doc->sb->st_size) {
			num = read(fd, buf, size);
			if (num > 0) {
				gtk_text_insert(GTK_TEXT(doc->text), NULL,
					&doc->text->style->black,
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
		free(buf);
		close(fd);
		gtk_text_thaw(GTK_TEXT(doc->text));
	} /* filesize > 0 */

	/* misc settings */
	doc->filename = fname;
	gtk_label_set(GTK_LABEL(doc->tab_label), (const char *)g_basename(fname));
	gtk_text_set_point(GTK_TEXT(doc->text), 0);

#ifdef GTK_HAVE_FEATURES_1_1_0
	/* Copy the buffer if split-screening enabled */
	if (doc->split_screen) {
		int pos;

		buf = gtk_editable_get_chars(GTK_EDITABLE(doc->text), 0, -1);
		pos = 0;
		doc->flag = doc->split_screen;
		gtk_text_freeze(GTK_TEXT(doc->split_screen));
		gtk_editable_insert_text(GTK_EDITABLE(doc->split_screen),
			buf, strlen(buf), &pos);
		gtk_text_thaw(GTK_TEXT(doc->split_screen));
	}
#endif
	
	/* enable document change detection */
	doc->changed = FALSE;
	if (!doc->changed_id)
		doc->changed_id =
			gtk_signal_connect(
				GTK_OBJECT(doc->text),
				"changed", 
				GTK_SIGNAL_FUNC(doc_changed_cb),
				doc);

	/* reset the title of the gedit window */
	title = g_malloc(strlen(GEDIT_ID) +
			strlen(GTK_LABEL(doc->tab_label)->label) + 4);
	sprintf(title, "%s - %s",
		GEDIT_ID, GTK_LABEL (doc->tab_label)->label);
	gtk_window_set_title(GTK_WINDOW (w->window), title);
	g_free (title);

	/* if it's present, update files list window */
	if (w->files_list_window)
		flw_append_entry(
			w, doc,
			g_list_length(GTK_NOTEBOOK(w->notebook)->children) - 1,
			doc->filename);

	mbprintf("opened %s", doc->filename);
	gE_msgbar_set(w, MSGBAR_FILE_OPENED);
	recent_add (doc->filename);
	recent_update (w);
	
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
gE_file_save(gE_window *window, gE_document *doc, gchar *fname)
{
	FILE *fp;
	gchar *title;

	g_assert(window != NULL);
	g_assert(doc != NULL);
	g_assert(fname != NULL);

	if ((fp = fopen(fname, "w")) == NULL) {
		g_warning ("Can't open file %s for saving", fname);
		return 1;
	}
	gtk_text_thaw (GTK_TEXT(doc->text));

	if (fputs(gtk_editable_get_chars(GTK_EDITABLE(doc->text), 0,
		gtk_text_get_length(GTK_TEXT(doc->text))), fp) == EOF) {
		perror("Error saving file");
		fclose(fp);
		return 1;
	}
	fflush(fp);
	fclose(fp);

	if (doc->filename && doc->filename != fname)
		g_free(doc->filename);
	doc->filename = g_strdup(fname);
	doc->changed = FALSE;
	gtk_label_set(GTK_LABEL(doc->tab_label),
		(const char *)g_basename(doc->filename));
	if (!doc->changed_id)
		doc->changed_id =
			gtk_signal_connect (
				GTK_OBJECT(doc->text), "changed",
				GTK_SIGNAL_FUNC(doc_changed_cb), doc);

	title = g_malloc0(strlen(GEDIT_ID) +
			strlen(GTK_LABEL(doc->tab_label)->label) + 4);
	sprintf(title, "%s - %s",
			GEDIT_ID, GTK_LABEL (doc->tab_label)->label);
	gtk_window_set_title (GTK_WINDOW (window->window), title);
	g_free (title);

	flw_update_entry(window, doc,
		gtk_notebook_current_page(GTK_NOTEBOOK(window->notebook)),
		(char *)g_basename(fname));

	mbprintf("saved %s", doc->filename);
	gE_msgbar_set(window, MSGBAR_FILE_SAVED);
	return 0;
} /* gE_file_save */


/*
 * PUBLIC: files_list_popup
 *
 * creates a new popup window containing a list of open files/documents
 * for the gE_window from which it was invoked.  callback data should be a
 * pointer to the invoking gE_window.
 */
#define FLW_NUM_COLUMNS	3
void
files_list_popup(GtkWidget *widget, gpointer cbdata)
{
	int num;
	GList *dp;
	GtkWidget *tmp, *vbox, *flw;

	char *titles[] = { " # ", " Size (bytes) ", " File name " };
	gE_window *window = ((gE_data *)cbdata)->window;


	g_assert(window != NULL);

	if (window->files_list_window)
		return;

	flw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(flw), "Currently Open Files");
	gtk_signal_connect(GTK_OBJECT(flw), "destroy",
		GTK_SIGNAL_FUNC(flw_destroy), cbdata);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(flw), vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show(vbox);

	window->files_list_window_data =
		gtk_clist_new_with_titles(FLW_NUM_COLUMNS, titles);
	tmp = window->files_list_window_data;
	gtk_clist_set_column_width(GTK_CLIST(tmp), FlwFnumColumn, 25);
	gtk_clist_column_title_passive(GTK_CLIST(tmp), FlwFnumColumn);
	gtk_clist_set_column_width(GTK_CLIST(tmp), FlwFsizeColumn, 90);
	gtk_clist_column_title_passive(GTK_CLIST(tmp), FlwFsizeColumn);
	gtk_clist_column_title_passive(GTK_CLIST(tmp), FlwFnameColumn);
	gtk_clist_set_policy(
		GTK_CLIST(tmp), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);

	num = 0;
	dp = window->documents;
	while (dp) {
		flw_append_entry(window, (gE_document *)(dp->data), num,
			((gE_document *)(dp->data))->filename);
		dp = dp->next;
		num++;
	}

	/*
	 * MUST connect this signal **after** adding fnames to list, else
	 * we keep changing the cur doc to the newly added file.
	 */
	gtk_signal_connect(GTK_OBJECT(window->files_list_window_data),
		"select_row", GTK_SIGNAL_FUNC(flw_select_file), window);
	gtk_widget_show(window->files_list_window_data);

	tmp = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, TRUE, 0);
	gtk_widget_show(tmp);

	window->files_list_window_toolbar = gE_create_toolbar_flw(cbdata);
	tmp = window->files_list_window_toolbar;
	gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);
	gtk_widget_show(tmp);

	/*
	 * there should be a way to auto-detect how wide to make these widgets
	 * instead of having to use arbitrary values
	 */
	gtk_widget_set_usize(flw, 250, 350);

	gtk_widget_show(flw);

	window->files_list_window = flw;
} /* files_list_popup */


/*
 * PUBLIC: flw_destroy
 *
 * zap!
 */
void
flw_destroy(GtkWidget *widget, gpointer data)
{
	gE_window *w = ((gE_data *)data)->window;

	if (w->files_list_window) {
		gtk_widget_destroy(w->files_list_window);
		w->files_list_window = NULL;
		w->files_list_window_data = NULL;
	}
} /* flw_destroy */


/*
 * PUBLIC: flw_remove_entry
 *
 * removes an entry from the files list window.
 */
void
flw_remove_entry(gE_window *w, int num)
{
	char buf[4];
	int i;
	GtkCList *clist;
	
	g_assert(w != NULL);
	if (w->files_list_window == NULL)
		return;

	clist = GTK_CLIST(w->files_list_window_data);
	g_assert(clist != NULL);
	gtk_clist_freeze(clist);
	gtk_clist_remove(clist, num);

	/* renumber any entries starting with the row just removed */
	if (clist->rows > 1) {
		for (i = num; i < clist->rows; i++) {
			g_snprintf(buf, 4, "%d", i + 1);
			gtk_clist_set_text(clist, i, FlwFnumColumn, buf);
		}
	}

	/* highlight the current document in the files list window */
	num = gtk_notebook_current_page(GTK_NOTEBOOK(w->notebook));
	gtk_clist_select_row(clist, num, FlwFnumColumn);
	g_snprintf(buf, 4, "%d", num + 1);
	gtk_clist_set_text(clist, num, FlwFnumColumn, buf);
	gtk_clist_thaw(clist);
} /* flw_remove_entry */

/*
 * PUBLIC: flw_remove_entry
 *
 * appends an entry from the files list window.
 */
void
flw_append_entry(gE_window *w, gE_document *curdoc, int rownum, char *text)
{
	char numstr[4], sizestr[16];
	char *rownfo[FLW_NUM_COLUMNS];
	GtkCList *clist;

	
	g_assert(w != NULL);

	clist = GTK_CLIST(w->files_list_window_data);

	/*
	 * currently, when there are no documents opened, we always create a
	 * new document called "Untitled".  if all we have "open" right now is
	 * the "Untitled" doc, we need to remove it before appending the new
	 * entry to the list.  (alternatively, we could also just set the
	 * clist field to the new column info too.)
	 */
	if (rownum == 0 && text != NULL) {
		/*
		 * this variable shouldn't need to be static/global (AFAIK),
		 * but i get a segv on my Linux box at home if i do NOT have
		 * compiler optimization present.  yet, this works fine on
		 * my Digital Unix box at work.  there's either something
		 * screwy with GTK or my Linux box (which is a little dated:
		 * a Debian 1.2 system with gcc 2.7.2.1 and libc5).
		 */
		static char *t;
		gtk_clist_get_text(clist, rownum, FlwFnameColumn, &t);
		if (t != NULL && strcmp(t, UNTITLED) == 0) {
			gtk_clist_freeze(clist);
			gtk_clist_remove(clist, rownum);
			gtk_clist_thaw(clist);
		}
	}

	g_snprintf(numstr, 4, "%d", rownum + 1);
	rownfo[0] = numstr;
	if (curdoc->sb == NULL)
		rownfo[1] = UNKNOWN;
	else {
		g_snprintf(sizestr, 16, "%8lu ", (gulong)(curdoc->sb->st_size));
		rownfo[1] = sizestr;
	}

	if (text && strlen(text) > 0) {
		rownfo[2] =  (char *)g_basename(text);
	} else {
		rownfo[1] = UNKNOWN;
		rownfo[2] = UNTITLED;
	}
	gtk_clist_freeze(clist);
	gtk_clist_append(clist, rownfo);
	gtk_clist_select_row(clist, rownum, FlwFnumColumn);
	gtk_clist_thaw(clist);
} /* flw_append_entry */


/*
 * PRIVATE: flw_select_file
 *
 * callback routine when selecting a file in the files list window.
 * effectively, sets the notebook to show the selected file.
 * see select_row() in gtkclist.c to match function parameters.
 */
static void
flw_select_file(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
	gE_window *window = (gE_window *)data;

	gtk_notebook_set_page(GTK_NOTEBOOK(window->notebook), row);
} /* flw_select_file */


/*
 * PRIVATE: flw_update_entry
 *
 * currently only called from gE_file_save(), this routine updates the
 * clist entry after saving a file.  this is necessary because the filename
 * could have changed, and most likely, the file size would have changed
 * as well.
 */
static void
flw_update_entry(gE_window *w, gE_document *curdoc, int rownum, char *text)
{
	GtkCList *clist;
	char sizestr[16];
	
	g_assert(w != NULL);

	if (w->files_list_window == NULL)
		return;

	clist = GTK_CLIST(w->files_list_window_data);

	if (curdoc->sb == NULL) {
		curdoc->sb = g_malloc(sizeof(struct stat));
		if (stat(text, curdoc->sb) == -1) {
			/* print warning */
			gtk_clist_set_text(clist, rownum, FlwFsizeColumn,
				UNKNOWN);
			g_free(curdoc->sb);
			curdoc->sb = NULL;
		} else {
			g_snprintf(sizestr, 16, "%8lu ",
				(gulong)(curdoc->sb->st_size));
			gtk_clist_set_text(clist, rownum, FlwFsizeColumn,
				sizestr);
		}
	}
	else {
		g_snprintf(sizestr, 16, "%8lu ", (gulong)(curdoc->sb->st_size));
		gtk_clist_set_text(clist, rownum, FlwFsizeColumn, sizestr);
	}
	gtk_clist_set_text(clist, rownum, FlwFnameColumn, text);
} /* flw_update_entry */

/* the end */
