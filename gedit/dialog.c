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
#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gnome.h>

#include "main.h"
#include "gE_window.h"

typedef struct dlg_data {
	GtkWidget *     dlg;	/* top level dialog widget */
	GtkWidget **    wgts;	/* widgets for dialog buttons */
	short           num;	/* number of buttons */
	short           id;	/* id of button pushed, starts at 1. -1 error */
	GtkSignalFunc **cbs;	/* array of callbacks for button widgets */
	void **         cbd;	/* array of callback data */
} dlg_data_t;


/* local function prototypes */
static void def_cb(GtkWidget *wgt, gpointer cbdata);


/*
 * PUBLIC: ge_dialog()
 *
 * generic routine to create a dialog box.
 *
 * PARAMETERS:
 *     title   - title of dialog box.  if NULL, default is "Dialog Title"
 *     msg     - message to user.  if NULL, default is "Hello world!"
 *     num     - number of buttons.
 *     buttons - array of button text.  e.g., { "Yes", "No", "Cancel" }
 *               use GE_BUTTON_OK (et al) for both Gnome and non-Gnome.
 *               see dialog.h for definitions.
 *     dflt    - default button number, starting at 1.
 *     cbs     - array of callback functions.  if cbs is NULL, then the
 *               default callback function will be used.
 *     cbd     - array of callback data.  if cbd is NULL, then the default
 *               callback data (dlgdata) is used.
 *     block   - blocking or not.  if blocking, ge_dialog() does not return
 *               control back to the caller until the user has pushed one of
 *               the buttons (thus causing the dialog box to be destroyed).
 *               for now, this is always blocking.
 *
 * RETURN VALUES:
 *     blocking: number of button which was pushed by user, starting at 1.
 *     non-blocking: always returns success unless error.
 *     -1 if error.
 */
int
ge_dialog(char *title, char *msg, short num, char **buttons, short dflt,
	GtkSignalFunc **cbs, void **cbd, gboolean block)
{
	GtkWidget *tmp;
	GtkWidget **wgts;	/* array to hold all the button widgets */
	GtkWidget *dlg;		/* top level dialog widget */
	dlg_data_t *dlgdata;	/* dialog callback data */
	GtkSignalFunc *cbfunc;	/* callback func */
	gpointer *cbdata;
	short i, ret;

	/* for now, it's always blocking */
	g_assert(block == TRUE);

	if (num < 1 || dflt < 1 || buttons == NULL)
		return -1;

	dlg = gtk_dialog_new();
	
	gtk_window_set_title(GTK_WINDOW(dlg), (title) ? title : "Dialog Title");

	tmp = gtk_label_new((msg) ? msg : "Hello world!");
	gtk_box_pack_start(
		GTK_BOX(GTK_DIALOG(dlg)->vbox), tmp, TRUE, FALSE, 10);
	gtk_widget_show(tmp);

	wgts = (GtkWidget **)g_malloc(num * sizeof(GtkWidget *));
	dlgdata = (dlg_data_t *)g_malloc0(sizeof(dlg_data_t));
	dlgdata->dlg = dlg;
	dlgdata->wgts = (GtkWidget **)g_malloc0(num * sizeof(GtkWidget *));
	dlgdata->num = num;
	dlgdata->id = -1;
	dlgdata->cbs = cbs;
	dlgdata->cbd = cbd;

	for (i = 0; i < num; i++) {

		tmp = gnome_stock_button(buttons[i]);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
			tmp, TRUE, TRUE, 2);

		dlgdata->wgts[i] = tmp;

		cbfunc = (cbs && cbs[i]) ? cbs[i] : (GtkSignalFunc *)def_cb;
		cbdata = (cbd && cbd[i]) ? cbd[i] : dlgdata;
		gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
			GTK_SIGNAL_FUNC(cbfunc), cbdata);
		if (cbfunc != (GtkSignalFunc *)def_cb) {
			gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
				GTK_SIGNAL_FUNC(def_cb), dlgdata);
		}

		gtk_widget_show(tmp);

		if (dflt == i + 1) {
			GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);
			gtk_widget_grab_default(tmp);
		}
	}

	gtk_widget_show(dlg);
	gtk_grab_add(dlg);

	if (block) {
		/* loop around doing nothing until dialog widget is gone */
		while (dlgdata->dlg != NULL)
			gtk_main_iteration_do(TRUE);

		ret = dlgdata->id + 1;
		g_free(dlgdata->wgts);
		g_free(dlgdata);
		g_free(wgts);
		return ret;
	}

	return 0;	/* non-blocking always returns success */

} /* ge_dialog */


/*
 * PRIVATE: def_cb
 *
 * callback for all button widgets used in ge_dialog().  upon invocation,
 * the callback data contains an array of the widget pointers created in
 * ge_dialog().  this array is scanned and if a match is found, the id num
 * in the callback data is set, the top-level dialog widget is destroyed,
 * and the dialog widget is set to NULL.  setting the dialog widget to NULL
 * will cause the while loop in ge_dialog() to exit.
 */
static void
def_cb(GtkWidget *wgt, gpointer cbdata)
{
	short i;
	dlg_data_t *data = (dlg_data_t *)cbdata;

	g_assert(data != NULL);
	g_assert(data->num > 0);
	g_assert(data->wgts != NULL);
	g_assert(data->id == -1);

	for (i = 0; i < data->num; i++) {
		if (data->wgts[i] == wgt) {
			data->id = i;
			break;
		}
	}

	gtk_widget_destroy(data->dlg);
	data->dlg = NULL;	/* this exits the while loop in ge_dialog() */

} /* def_cb */

/* the end */
