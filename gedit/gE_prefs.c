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
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include "main.h"
#include "gE_prefs.h"
#include "toolbar.h"


typedef struct _gE_prefbox {
	GtkWidget *window;
	GtkWidget *tfont;
	GtkWidget *tslant;
	GtkWidget *tweight;
	GtkWidget *tsize;
	GtkWidget *pcmd;
} gE_prefbox;


static gE_prefbox *prefs;
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
	/*if ((rc = gE_prefs_open_file ("gtkrc", "r")) == NULL)
	{
		printf ("gE_rc_parse: Couldn't open gtk rc file for parsing.\n");
		return;
	}*/
	rc = gE_prefs_open_file ("gtkrc", "r");
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

	gE_save_settings(window, gtk_entry_get_text(GTK_ENTRY(prefs->pcmd)));
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

	prefs = (gE_prefbox *)g_malloc(sizeof(gE_prefbox));
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

	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_OK);

	gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
		GTK_SIGNAL_FUNC(ok_prefs), window);
	gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(tmp);
	gtk_widget_show(tmp);

	tmp = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);

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

void 
gE_save_settings(gE_window *window, gpointer cbwindow)
{
	window = (gE_window *) cbwindow;

	gE_prefs_set_int("tab pos", (gint) window->tab_pos);
	gE_prefs_set_int("auto indent", (gboolean) window->auto_indent);
	gE_prefs_set_int("show statusbar", (gboolean) window->show_status);
	gE_prefs_set_int("toolbar", (gint) window->have_toolbar);
	gE_prefs_set_int("tb text", (gint) window->have_tb_text);
	gE_prefs_set_int("tb pix", (gint) window->have_tb_pix);
	gE_prefs_set_int("tb relief", (gint) window->use_relief_toolbar);
	gE_prefs_set_int("splitscreen", (gint) window->splitscreen);

	gE_prefs_set_char("font", window->font);
	if (window->print_cmd == "")
		gE_prefs_set_char ("print command", "lpr -rs %s");
	else
		gE_prefs_set_char ("print command", window->print_cmd);

}

void gE_get_settings(gE_window *w)
{
	
	 w->tab_pos = gE_prefs_get_int("tab pos");
	 w->show_status = gE_prefs_get_int("show statusbar");
	 w->have_toolbar = gE_prefs_get_int("toolbar");
	 w->have_tb_text = gE_prefs_get_int("tb text");
	 w->have_tb_pix = gE_prefs_get_int("tb pix");
	 w->use_relief_toolbar = gE_prefs_get_int("tb relief");
	 w->splitscreen = gE_prefs_get_int("splitscreen");
	 w->font = gE_prefs_get_char("font");
	 if (w->font == NULL)
	   w->font = "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1";
	 w->print_cmd = gE_prefs_get_char("print command"); 
	 if (w->print_cmd == NULL)
	   w->print_cmd = "lpr %s";


	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), w->tab_pos);

	if (w->show_status == FALSE)
		gtk_widget_hide(GTK_WIDGET (w->statusbar)->parent);

}
