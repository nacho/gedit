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
#ifndef __MSGBOX_H__
#define __MSGBOX_H__

#define mbprint(x)	mbprintf(x);

extern void mbprintf(const char *fmt, ...) G_GNUC_PRINTF (1, 2);
extern void msgbox_create(void);
extern void msgbox_show(GtkWidget *, gpointer data);
extern void msgbox_close(void);

#endif	/* __MSGBOX_H__ */
