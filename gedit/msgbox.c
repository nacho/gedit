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
#include <stdarg.h>
#include <time.h>
#include <string.h>
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "msgbox.h"

/*
 * TODO - put into preferences: max msg limit, and option for whether or not
 * to log msgs.  for now, limit is hard coded and we always log msgs.
 */

#define MAX_MSGS	100


typedef struct {
	GtkWidget *toplev;	/* toplevel window */
	GtkWidget *text;	/* a text widget stores the messages */
	gushort    num;		/* count of messages */
} gE_msgbox;

static gE_msgbox msgbox;


static void msgbox_append(char *msg);
static void msgbox_clear(void);
static void msgbox_truncate(void);
static void msgbox_set_sb(void);
static void msgbox_destroy(GtkWidget *w, gpointer data);
#ifdef DEBUG
static void msgbox_append_dbg(GtkWidget *w, gpointer data);
static char *msgdbg_str = "added a message!";
#endif


/*
 * PUBLIC: mbprintf
 *
 * print a formatted message to a the msgbox
 *
 * we need to use g_vsprintf(), which is in glib/gstring.c, but isn't found in
 * glib.h.  and of course, we can't have gstring.h be public because it might
 * be indecent exposure.  basically, we use g_vsprintf() to build a buffer of
 * the right/exact size needed to print the message.  if we don't, then we'd
 * have to parse the va_list and check all the printf formatters.
 */
extern char *g_vsprintf (const gchar *fmt, va_list *args, va_list *args2);

void
mbprintf(const char *fmt, ...)
{
	va_list ap1, ap2;
	char *buf;

	va_start(ap1, fmt);
	va_start(ap2, fmt);
	buf = g_vsprintf(fmt, &ap1, &ap2);
	va_end(ap1);
	va_end(ap2);
	msgbox_append(buf);
} /* mbprintf */


/*
 * PUBLIC: msgbox_create
 *
 * basically taken from testgtk.c.  should be called before any file is
 * opened.
 */
void
msgbox_create(void)
{
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *separator;
	GtkWidget *table;
	GtkWidget *vscrollbar;
	GtkWidget *text;

	g_return_if_fail(msgbox.toplev == NULL);
	g_return_if_fail(msgbox.text == NULL);

	msgbox.num = 0;
	msgbox.toplev = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name(msgbox.toplev, "messages");
	gtk_widget_set_usize(msgbox.toplev, 500, 250);
	gtk_window_set_policy(GTK_WINDOW(msgbox.toplev), TRUE, TRUE, FALSE);
	gtk_window_set_title(GTK_WINDOW(msgbox.toplev), "gEdit Messages");
	gtk_container_border_width(GTK_CONTAINER(msgbox.toplev), 0);

	gtk_signal_connect(GTK_OBJECT(msgbox.toplev), "destroy",
		GTK_SIGNAL_FUNC(msgbox_destroy), msgbox.toplev);

	box1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(msgbox.toplev), box1);
	gtk_widget_show(box1);

	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, TRUE, TRUE, 0);
	gtk_widget_show(box2);


	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacing(GTK_TABLE(table), 0, 2);
	gtk_table_set_col_spacing(GTK_TABLE(table), 0, 2);
	gtk_box_pack_start(GTK_BOX(box2), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	msgbox.text = gtk_text_new(NULL, NULL);
	text = msgbox.text;
	gtk_table_attach(GTK_TABLE(table), text, 0, 1, 0, 1,
		GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show(text);

	vscrollbar = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);
	gtk_table_attach(GTK_TABLE(table), vscrollbar, 1, 2, 0, 1,
		GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show(vscrollbar);

	gtk_widget_realize(text);
	gtk_text_set_editable(GTK_TEXT(text), FALSE);
	gtk_text_set_word_wrap(GTK_TEXT(text), FALSE);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
	gtk_widget_show(separator);


	box2 = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(box2), 10);
	gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, TRUE, 0);
	gtk_widget_show(box2);


	hbox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(box2), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
#ifdef DEBUG
	button = gtk_button_new_with_label("Add message!");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(msgbox_append_dbg), msgdbg_str);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);
#endif

	button = gtk_button_new_with_label("Clear");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(msgbox_clear), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Close");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(msgbox_close), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	mbprintf("Welcome to %s", GEDIT_ID);

} /* msgbox_create */


/*
 * PUBLIC: msgbox_show
 *
 * shows the message box.  invoked as a callback from the main menu.
 */
void
msgbox_show(GtkWidget *w, gpointer data)
{
	if (msgbox.toplev == NULL)
		msgbox_create();

	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	gtk_widget_show(msgbox.toplev);
	msgbox_set_sb();
} /* msgbox_show */


/*
 * PRIVATE: msgbox_append
 *
 * add another message the msgbox.  all msgs contain the current time/data
 * before the actual string provided by the user.
 */
static void
msgbox_append(char *msg)
{
	time_t curtime;
	char *timestr, *buf;

	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	msgbox.num++;
	if (msgbox.num == MAX_MSGS)
		msgbox_truncate();

	gtk_text_freeze(GTK_TEXT(msgbox.text));

	/* get string for current time and strip off \n added by ctime() */
	time(&curtime);
	timestr = ctime(&curtime);
	if (timestr[strlen(timestr) - 1] == '\n')
		timestr[strlen(timestr) - 1] = '\0';

	/* build the time/data as a string, append \n to msg if user didn't */
	buf = (char *)g_malloc(strlen(timestr) + strlen(msg) + 5);
	sprintf(buf, "[%s] %s%c", timestr, msg,
		(msg[strlen(msg) - 1] == '\n') ? '\0' : '\n');
	gtk_text_insert(GTK_TEXT(msgbox.text), NULL,
		&msgbox.text->style->black, NULL, buf, strlen(buf));
	g_free(buf);

	msgbox_set_sb();
	gtk_text_thaw(GTK_TEXT(msgbox.text));
} /* void msgbox_entry_append */


/*
 * PUBLIC: msgbox_close
 *
 * we don't really close/destroy the box; we hide it (since we still log
 * messages to the text box).
 */
void
msgbox_close(void)
{
	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	gtk_widget_hide(msgbox.toplev);
} /* msgbox_hide */


/*
 * PRIVATE: msgbox_destroy
 *
 * zap!
 */
static void
msgbox_destroy(GtkWidget *w, gpointer data)
{
	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	gtk_widget_destroy(msgbox.text);
	gtk_widget_destroy(msgbox.toplev);
	msgbox.text = NULL;
	msgbox.toplev = NULL;
} /* msgbox_destroy */


static void
msgbox_set_sb(void)
{
	GtkAdjustment *vadj;
	GtkText *text;
	float value;

	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	/* always set the scrollbar to view the latest message */
	if (msgbox.num > 0) {
		text = GTK_TEXT(msgbox.text);
		vadj = GTK_ADJUSTMENT(text->vadj);
		if (vadj == NULL)
			return;

		value = (msgbox.num * vadj->upper) * 1.0 /
			msgbox.num - vadj->page_increment;

		gtk_adjustment_set_value(GTK_ADJUSTMENT(text->vadj), value);
	}
} /* msgbox_set_sb */


/*
 * PRIVATE: msgbox_truncate
 *
 * when there are MAX_MSGS number of msgs, delete the first half of them to
 * free up space.
 */
static void
msgbox_truncate(void)
{
	guint i, num, err;
	gboolean found = FALSE;
	GtkText *text = GTK_TEXT(msgbox.text);

	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	for (num = 0, i = 0; i < text->gap_position; i++) {
		if (text->text[i] == '\n') {
			num++;
			if (num == (MAX_MSGS / 2)) {
				i++;	/* skip over \n */
				gtk_text_set_point(text, i);
				found = TRUE;
				break;
			}
		}
	}

	if (!found) {
		for (i = i + text->gap_size; i < text->text_end; i++) {
			if (text->text[i] == '\n') {
				num++;
				if (num == (MAX_MSGS / 2)) {
					i++;	/* skip over \n */
					gtk_text_set_point(text, i);
					found = TRUE;
					break;
				}
			}
		}
	}

	g_assert(found == TRUE);

	gtk_text_freeze(text);
	err = gtk_text_backward_delete(text, i);
	gtk_text_set_point(text, gtk_text_get_length(text));
	gtk_text_thaw(text);

	msgbox.num -= num;
} /* msgbox_truncate */


/*
 * PRIVATE: msgbox_clear
 *
 * clears and resets everything
 */
static void
msgbox_clear(void)
{
	int err;

	g_return_if_fail(msgbox.toplev != NULL);
	g_return_if_fail(msgbox.text != NULL);

	gtk_text_freeze(GTK_TEXT(msgbox.text));
	gtk_text_set_point(GTK_TEXT(msgbox.text), 0);
	err = gtk_text_forward_delete(
			GTK_TEXT(msgbox.text),
			gtk_text_get_length(GTK_TEXT(msgbox.text)));
	gtk_text_thaw(GTK_TEXT(msgbox.text));
	msgbox.num = 0;
} /* msgbox_clear */


#ifdef DEBUG
/*
 * PRIVATE: msgbox_append_dbg
 *
 * for debugging purposes only
 */
static void
msgbox_append_dbg(GtkWidget *w, gpointer data)
{
	mbprintf("%s", (char *)data);
} /* msgbox_append_dbg */
#endif

/* the end */
