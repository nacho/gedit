#ifndef __DIALOGS_H__
#define __DIALOGS_H__

void dialog_about     (void);
void dialog_goto_line (void);
void dialog_replace   (gint full);
void dialog_prefs     (void);

gchar * gedit_plugin_program_location_dialog (void);

void gedit_plugin_manager_create (GtkWidget *widget, gpointer data);

#endif /* __DIALOGS_H__ */
