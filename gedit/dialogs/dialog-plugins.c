#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "file.h"
#include "plugin.h"
#include "document.h"
#include "utils.h"
#include "window.h"

#include "dialogs/dialogs.h"

/**
 * gedit_plugin_email_sendmail_location_dialog:
 * @void: 
 * 
 * it displays and pop-up a dialog so that the user can specify a
 * location for sendmail.
 * 
 * Return Value: a string with the path specified by the user
 **/
gchar *
gedit_plugin_program_location_dialog (void)
{
	GladeXML *gui;
	GtkWidget *program_location_dialog;
	GtkWidget *program_location_entry;
	gchar * location = NULL;

	gedit_debug ("", DEBUG_PLUGINS);

	gui = glade_xml_new (GEDIT_GLADEDIR "/program.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find program.glade");
		return NULL;
	}

	program_location_dialog = glade_xml_get_widget (gui, "program_location_dialog");
	program_location_entry  = glade_xml_get_widget (gui, "program_location_file_entry");

	if (!program_location_dialog || !program_location_entry)
	{
		g_warning ("Could not get the program location dialog from program.glade");
		return NULL;
	}

	/* If we do this the main plugin dialog goes below the program window. Chema.
	  gnome_dialog_set_parent (GNOME_DIALOG(program_location_dialog), gedit_window_active());
	*/
	switch (gnome_dialog_run (GNOME_DIALOG (program_location_dialog)))
	{
	case 0:
		location = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(program_location_entry), FALSE);
		gnome_dialog_close (GNOME_DIALOG(program_location_dialog));
		break;
	case 1:
		gnome_dialog_close (GNOME_DIALOG(program_location_dialog));
		break;
	default:
		location = NULL;
		break;
	}

	gtk_object_unref (GTK_OBJECT (gui));

	return location;
}
