/*
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
#include <gtk/gtk.h>
#include "msgbox.h"
gint MessageBox(gchar *text,gchar *title,gint type)
{	
	gint retvalue;
	msgbox_data *ptr=g_malloc0(sizeof(msgbox_data));
	if(!ptr)
	{	g_print("Cannot allocate mem for MessageBox\n%s\n",text);	return 0; }
	
	ptr->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if(!title)
	{
		#ifdef PROGRAMTITLE
			gtk_window_set_title(GTK_WINDOW(ptr->window),PROGRAMTITLE);
		#else
			gtk_window_set_title(GTK_WINDOW(ptr->window),"Message");
		#endif
	}
	else
		gtk_window_set_title(GTK_WINDOW(ptr->window),title);
		
	gtk_signal_connect_object(GTK_OBJECT(ptr->window),"destroy",
				GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(ptr->window));
	
	gtk_widget_set_usize(GTK_WIDGET(ptr->window),250,150);
	ptr->vbox=gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(ptr->window),ptr->vbox);
	gtk_container_border_width(GTK_CONTAINER(ptr->vbox),8);

	ptr->label=gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(ptr->vbox),ptr->label,TRUE,FALSE,0);
	ptr->hbox=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(ptr->vbox),ptr->hbox,FALSE,FALSE,0);
	
	if(type & MB_OK) {
		ptr->button1=gtk_button_new_with_label("Ok");
		gtk_box_pack_start(GTK_BOX(ptr->hbox),ptr->button1,FALSE,FALSE,10);
		gtk_signal_connect(GTK_OBJECT(ptr->window),"delete_event",
					GTK_SIGNAL_FUNC(msgbox_callback_ok),ptr);
		gtk_signal_connect(GTK_OBJECT(ptr->button1),"clicked",
					GTK_SIGNAL_FUNC(msgbox_callback_ok),ptr);

		gtk_widget_show(ptr->button1);
	}
	else if(type & MB_YESNO)	{
		ptr->button1=gtk_button_new_with_label("Yes");
		ptr->button2=gtk_button_new_with_label("No");
		gtk_box_pack_start(GTK_BOX(ptr->hbox),ptr->button1,FALSE,FALSE,10);
		gtk_box_pack_start(GTK_BOX(ptr->hbox),ptr->button2,FALSE,FALSE,10);
		gtk_signal_connect(GTK_OBJECT(ptr->button1),"clicked",
					GTK_SIGNAL_FUNC(msgbox_callback_yes),ptr);
		gtk_signal_connect(GTK_OBJECT(ptr->button2),"clicked",
					GTK_SIGNAL_FUNC(msgbox_callback_no),ptr);
					
		gtk_widget_show(ptr->button1);
		gtk_widget_show(ptr->button2);
	}
	gtk_widget_show(ptr->label);
	gtk_widget_show(ptr->vbox);
	gtk_widget_show(ptr->hbox);
	gtk_widget_show(ptr->window);

	msgbox_modal_loop(ptr);
	retvalue =ptr->retvalue;
	
	g_free(ptr);
	
	return retvalue; 
}
void msgbox_modal_loop(msgbox_data *data)
{
	gtk_grab_add(data->window);
	while (!data->ready) {
		gtk_main_iteration_do(TRUE);
	};
	g_print("modalloop msgbox done\n");
	gtk_grab_remove(data->window);
	gtk_widget_destroy(data->window);
}
void msgbox_callback_ok(GtkWidget *w,gpointer d)
{
	msgbox_data *data;
	data=(msgbox_data *)d;
	data->ready=TRUE;
	data->retvalue=IDOK;
}
void msgbox_callback_cancel(GtkWidget *w,gpointer d)
{
	msgbox_data *data;
	data=(msgbox_data *)d;
	data->ready=TRUE;
	data->retvalue=IDCANCEL;
}
void msgbox_callback_retry(GtkWidget *w,gpointer d)
{
	msgbox_data *data;
	data=(msgbox_data *)d;
	data->ready=TRUE;
	data->retvalue=IDRETRY;
}
void msgbox_callback_yes(GtkWidget *w,gpointer d)
{
	msgbox_data *data;
	data=(msgbox_data *)d;
	data->ready=TRUE;
	data->retvalue=IDYES;
}
void msgbox_callback_no(GtkWidget *w,gpointer d)
{
	msgbox_data *data;
	data=(msgbox_data *)d;
	data->ready=TRUE;
	data->retvalue=IDNO;
}
