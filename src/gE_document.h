/* vi:set ts=4 sts=0 sw=4:
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
#ifndef __GE_DOCUMENT_H__
#define __GE_DOCUMENT_H__

#define MSGBAR_CLEAR		" "
#define MSGBAR_FILE_NEW		"New File..."
#define MSGBAR_FILE_OPENED	"File Opened..."
#define MSGBAR_FILE_CLOSED	"File Closed..."
#define MSGBAR_FILE_CLOSED_ALL	"All Files Closed..."
#define MSGBAR_FILE_PRINTED	"Print Command Executed..."
#define MSGBAR_FILE_SAVED	"File Saved..."
#define MSGBAR_CUT		"Selection Cut..."
#define MSGBAR_COPY		"Selection Copied..."
#define MSGBAR_PASTE		"Selection Pasted..."
#define MSGBAR_SELECT_ALL	"All Text Selected..."

extern GtkWidget *col_label;

extern void gedit_window_set_auto_indent (gint auto_indent);
extern void gedit_window_set_status_bar (gint show_status);
extern void gedit_window_new (GnomeMDI *mdi, GnomeApp *app);

extern void doc_swaphc_cb (GtkWidget *w, gpointer cbdata);
extern void child_switch (GnomeMDI *mdi, gedit_document *doc);

#ifdef WITH_GMODULE_PLUGINS
extern gedit_document *gedit_document_new_container (gedit_window *w, gchar *title,
					       gint with_split_screen);
#endif

#endif /* __GE_DOCUMENT_H__ */
