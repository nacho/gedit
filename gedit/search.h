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
#ifndef __SEARCH_H__
#define __SEARCH_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Search and Replace */
extern void search_cb(GtkWidget *w, gpointer cbdata);
extern void search_replace_cb(GtkWidget *w, gpointer cbdata);
extern void search_again_cb(GtkWidget *w, gpointer cbdata);
extern void goto_line_cb(GtkWidget *w, gpointer cbwindow);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SEARCH_H__ */
