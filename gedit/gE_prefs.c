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
#include <time.h>
#include <string.h>
#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "gE_prefs.h"
#include "toolbar.h"

#define GEDIT_RC	".gedit"

typedef struct _gE_prefs {
	GtkWidget *window;
	GtkWidget *tfont;
	GtkWidget *tslant;
	GtkWidget *tweight;
	GtkWidget *tsize;
	GtkWidget *pcmd;
} gE_prefs;


static gE_prefs *prefs;
static char *rc;


static void gE_prefs_window(gE_window *w);
static char *gettime(char *buf);
static void ok_prefs(GtkWidget *widget, gE_window *window);
static void cancel_prefs(GtkWidget *widget, gpointer data);


void
prefs_cb (GtkWidget *widget, gpointer cbwindow)
{
	gE_prefs_window((gE_window *)cbwindow);
}


void 
gE_rc_parse(void)
{
	char *home;

	if ((home = getenv("HOME")) == NULL)
		return;

	rc = (char *)g_malloc(strlen(home) + strlen(GEDIT_RC) + 2);
	sprintf(rc, "%s/%s", home, GEDIT_RC);

	gtk_rc_parse(rc);
}


static void 
ok_prefs(GtkWidget *widget, gE_window *window)
{
	FILE *file;
	char stamp[50];

	if ((file = fopen(rc, "w")) == NULL) {
		g_print("Cannot open %s!\n", rc);
		return;
	}
	strcpy(stamp, "Modified: ");
	gettime(stamp + strlen(stamp));
	fprintf(file, "# gEdit rc file...\n");
	fprintf(file, "#\n");
	fprintf(file, "# %s\n", stamp);
	fprintf(file, "#\n\n");
	fprintf(file, "style \"text\"\n");
	fprintf(file, "{\n");
	fprintf(file, "  font = \"-*-%s-medium-%s-%s--%s-*-*-*-*-*-*-*\"\n",
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs->tfont)->entry)),
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs->tslant)->entry)),
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(prefs->tweight)->entry)),
		gtk_entry_get_text(GTK_ENTRY(prefs->tsize)));
	fprintf(file, "}\n");
	fprintf(file, "widget_class \"*GtkText\" style \"text\"\n");
	fclose(file);

#ifndef WITHOUT_GNOME
	gE_save_settings(window, gtk_entry_get_text(GTK_ENTRY(prefs->pcmd)));
#endif
	gtk_widget_destroy(prefs->window);
	prefs->window = NULL;
} /* ok_prefs */

static void 
cancel_prefs(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(prefs->window);
	prefs->window = NULL;
}

static void
gE_prefs_window(gE_window *window)
{
	GtkWidget *tmp, *vbox, *hbox;
	GList *fonts = NULL;
	GList *weight = NULL;
	GList *slant = NULL;

	if (prefs) {
		gtk_widget_show(prefs->window);
		return;
	}

	fonts = g_list_append(fonts, "courier");
	fonts = g_list_append(fonts, "helvetica");
	weight = g_list_append(weight, "*");
	weight = g_list_append(weight, "normal");
	weight = g_list_append(weight, "bold");
	weight = g_list_append(weight, "medium");
	slant = g_list_append(slant, "*");
	slant = g_list_append(slant, "i");
	slant = g_list_append(slant, "r");

	prefs = (gE_prefs *)g_malloc(sizeof(gE_prefs));
	prefs->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(prefs->window), "destroy",
		GTK_SIGNAL_FUNC(cancel_prefs), prefs->window);
	gtk_signal_connect(GTK_OBJECT(prefs->window), "delete_event",
		GTK_SIGNAL_FUNC(cancel_prefs), prefs->window);
	gtk_window_set_title(GTK_WINDOW(prefs->window), "Prefs");
	gtk_container_border_width(GTK_CONTAINER(prefs->window), 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(prefs->window), vbox);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	tmp = gtk_label_new(("Font:"));
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show(tmp);
	prefs->tfont = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(prefs->tfont), fonts);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(prefs->tfont)->entry),
		"courier");
	gtk_box_pack_start(GTK_BOX(hbox), prefs->tfont, TRUE, TRUE, 0);
	gtk_widget_show(prefs->tfont);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	tmp = gtk_label_new("Size:");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show(tmp);
	prefs->tsize = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(prefs->tsize), "12");
	gtk_box_pack_start(GTK_BOX(hbox), prefs->tsize, TRUE, TRUE, 0);
	gtk_widget_show(prefs->tsize);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	tmp = gtk_label_new("Weight:");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show(tmp);
	prefs->tweight = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(prefs->tweight), weight);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(prefs->tweight)->entry),
		"normal");
	gtk_box_pack_start(GTK_BOX(hbox), prefs->tweight, TRUE, TRUE, 0);
	gtk_widget_show(prefs->tweight);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	tmp = gtk_label_new("Slant:");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show(tmp);
	prefs->tslant = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(prefs->tslant), slant);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(prefs->tslant)->entry), "r");
	gtk_box_pack_start(GTK_BOX(hbox), prefs->tslant, TRUE, TRUE, 0);
	gtk_widget_show(prefs->tslant);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	tmp = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, TRUE, 0);
	gtk_widget_show(tmp);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	tmp = gtk_label_new("Print Command:");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show(tmp);
	prefs->pcmd = gtk_entry_new();
	if (window->print_cmd)
		gtk_entry_set_text(GTK_ENTRY(prefs->pcmd), window->print_cmd);
	gtk_box_pack_start(GTK_BOX(hbox), prefs->pcmd, TRUE, TRUE, 0);
	gtk_widget_show(prefs->pcmd);

	tmp = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, TRUE, 0);
	gtk_widget_show(tmp);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

#ifdef WITHOUT_GNOME
	tmp = gtk_button_new_with_label("Ok");
#else
	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
#endif
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
		GTK_SIGNAL_FUNC(ok_prefs), window);
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(tmp);
	gtk_widget_show(tmp);

#ifdef WITHOUT_GNOME
	tmp = gtk_button_new_with_label("Cancel");
#else
	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
		GTK_SIGNAL_FUNC(cancel_prefs), (gpointer) "button");
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show(tmp);
	gtk_widget_show(prefs->window);
} /* gE_prefs_window */

static char *
gettime(char *buf)
{
	time_t clock;
	struct tm *now;
	char *buffer;

	buffer = buf;
	clock = time(NULL);
	now = gmtime(&clock);

	sprintf(buffer, "%04d-%02d-%02d%c%02d:%02d:%02d%c",
		now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
		(FALSE ? 'T' : ' '),
		now->tm_hour, now->tm_min, now->tm_sec,
		(FALSE ? 'Z' : '\000'));
	return buffer;
}

#ifndef WITHOUT_GNOME

void 
gE_save_settings(gE_window *window, gpointer cbwindow)
{
	window = (gE_window *) cbwindow;

	gnome_config_push_prefix("/gEdit/Global/");

	mbprintf("window->tab_pos = %d", window->tab_pos);
	mbprintf("window->auto_indent = %d", window->auto_indent);
	mbprintf("window->show_status = %d", window->show_status);
	mbprintf("window->have_toolbar = %d", window->have_toolbar);
	mbprintf("window->have_tb_text = %d", window->have_tb_text);
	mbprintf("window->have_tb_pix = %d", window->have_tb_pix);

	gnome_config_set_int("tab pos", (gint) window->tab_pos);
	gnome_config_set_int("auto indent", (gboolean) window->auto_indent);
	gnome_config_set_int("show statusbar", (gboolean) window->show_status);
	gnome_config_set_int("toolbar", (gint) window->have_toolbar);
	gnome_config_set_int("tb text", (gint) window->have_tb_text);
	gnome_config_set_int("tb pix", (gint) window->have_tb_pix);

	if (window->print_cmd == "")
		gnome_config_set_string("print command", "lpr -rs %s");
	else
		gnome_config_set_string("print command", window->print_cmd);
	/*gnome_config_set_string ("print command", cmd); */

	gnome_config_pop_prefix();
	gnome_config_sync();
}

void gE_get_settings(gE_window *w)
{
	gnome_config_push_prefix("/gEdit/Global/");

	w->tab_pos = gnome_config_get_int("tab pos");
	w->show_status = gnome_config_get_int("show statusbar");
	w->have_toolbar = gnome_config_get_int("toolbar");
	w->have_tb_text = gnome_config_get_int("tb text");
	w->have_tb_pix = gnome_config_get_int("tb pix");

	w->print_cmd = gnome_config_get_string("print command");

	gnome_config_pop_prefix();
	gnome_config_sync();

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), w->tab_pos);

	if (w->show_status == FALSE)
		gtk_widget_hide(w->statusbox);

	if (w->have_toolbar == TRUE) {
		tb_on_cb(NULL, w);

		if (w->have_tb_text == FALSE && w->have_tb_pix == TRUE)
			tb_pic_only_cb(NULL, w);

		if (w->have_tb_text == TRUE && w->have_tb_pix == FALSE)
			tb_text_only_cb(NULL, w);

		if (w->have_tb_text == FALSE && w->have_tb_pix == FALSE)
			tb_pic_text_cb(NULL, w);
	} else {
		tb_off_cb(NULL, w);
	}
}
#endif
