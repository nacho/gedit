#ifndef __MSGBOX_H__
#define __MSGBOX_H__
/* NOT DEFINED AS WINDOWS 95 */
#define MB_YESNO 0x0001
#define MB_OK 0x0002
#define MB_ICONSTOP 0x0004
#define MB_CANCEL 0x0008
#define MB_RETRY 0x0010
#define IDOK 1
#define IDYES 2
#define IDNO 3
#define IDCANCEL 4
#define IDRETRY 5
#define sMessageBox(text) MessageBox(text,NULL,MB_OK);
typedef struct __MSGBOX_DATA
{
	GtkWidget *window;
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	gint ready;
	gint retvalue;
}msgbox_data;

void msgbox_callback_delete_event();
void msgbox_callback_ok(GtkWidget *w,gpointer d );
void msgbox_callback_cancel(GtkWidget *w,gpointer d);
void msgbox_callback_retry(GtkWidget *w,gpointer d);
void msgbox_callback_yes(GtkWidget *w,gpointer d);
void msgbox_callback_no(GtkWidget *w,gpointer d);
gint MessageBox(gchar *text,gchar *title,gint type);
void msgbox_modal_loop(msgbox_data *);
#endif
