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
#ifndef __GE_FILES_H__
#define __GE_FILES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum { FlwFnumColumn = 0, FlwFsizeColumn, FlwFnameColumn } flw_col_t;


extern gint gE_file_open (gE_window *w, gE_document *doc, gchar *fname);
extern gint gE_file_save (gE_window *w, gE_document *doc, gchar *fname);
extern void files_list_popup(GtkWidget *widget, gpointer cbdata);
extern void flw_destroy(GtkWidget *widget, gpointer data);
extern void flw_remove_entry(gE_window *w, int num);
extern void flw_append_entry(gE_window *w, gE_document *, int , char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GE_FILES_H__ */
