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

extern void gE_window_set_auto_indent (gE_window *window, gint auto_indent);
extern void gE_window_set_status_bar (gE_window *window, gint show_status);
extern gE_window *gE_window_new();
extern gE_document *gE_document_new(gE_window *window);
extern gE_document *gE_document_current(gE_window *window);
extern void gE_document_set_word_wrap (gE_document *doc, gint word_wrap);
extern void gE_document_set_line_wrap (gE_document *doc, gint line_wrap);
extern void gE_document_set_read_only (gE_document *doc, gint read_only);
#ifndef WITHOUT_GNOME
extern void gE_document_set_scroll_ball (gE_document *doc, gint scroll_ball);
#endif
#ifdef GTK_HAVE_FEATURES_1_1_0
extern void gE_document_set_split_screen (gE_document *doc, gint split_screen);
#endif
extern void gE_msgbar_set(gE_window *window, char *msg);
extern void gE_msgbar_clear(gpointer data);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GE_DOCUMENT_H__ */
