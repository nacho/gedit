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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MSGBAR_CLEAR			" "
#define MSGBAR_FILE_NEW			"New File..."
#define MSGBAR_FILE_OPENED		"File Opened..."
#define MSGBAR_FILE_CLOSED		"File Closed..."
#define MSGBAR_FILE_CLOSED_ALL	"All Files Closed..."
#define MSGBAR_FILE_PRINTED		"Print Command Executed..."
#define MSGBAR_FILE_SAVED		"File Saved..."
#define MSGBAR_CUT				"Selection Cut..."
#define MSGBAR_COPY				"Selection Copied..."
#define MSGBAR_PASTE			"Selection Pasted..."
#define MSGBAR_SELECT_ALL		"All Text Selected..."

extern void gE_window_toggle_statusbar (GtkWidget *w, gpointer cbwindow);
extern gE_window *gE_window_new();
extern gE_document *gE_document_new(gE_window *window);
extern gE_document *gE_document_current(gE_window *window);
extern void gE_document_toggle_wordwrap (GtkWidget *w, gpointer cbwindow);
#ifdef GTK_HAVE_FEATURES_1_1_0
extern void gE_document_toggle_scrollball (GtkWidget *w, gE_window *window);
#endif
extern void gE_msgbar_set(gE_window *window, char *msg);
extern void gE_msgbar_clear(gpointer data);
extern void gE_msgbar_timeout_add(gE_window *window);
extern void notebook_switch_page(GtkWidget *w, GtkNotebookPage *page,
    gint num, gE_window *window);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GE_DOCUMENT_H__ */
