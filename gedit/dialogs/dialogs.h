#ifndef __GEDIT_DIALOGS_H__
#define __GEDIT_DIALOGS_H__

void gedit_dialog_about     (void);
void gedit_dialog_goto_line (void);
void gedit_dialog_replace   (gboolean replace);
void gedit_find_again       (void);
void gedit_dialog_prefs     (void);

void gedit_dialog_open_uri (void);

gchar * gedit_plugin_program_location_dialog (void);

void gedit_plugin_manager_create (GtkWidget *widget, gpointer data);

#endif /* __GEDIT_DIALOGS_H__ */
