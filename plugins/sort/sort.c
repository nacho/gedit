/*
 * Sort plugin
 * Carlo Borreo borreo@softhome.net
 *
 * Sorts the current document
 */

#include <config.h>
#include <string.h>
#include <gnome.h>
#include <glib.h>
#include <glade/glade.h>

#include "window.h"
#include "document.h"
#include "utils.h"

#include "view.h"
#include "plugin.h"

#define GEDIT_PLUGIN_GLADE_FILE "/sort.glade"

struct GeditSortInfo
{
	gint col_skip;
	gboolean case_sensitive;
	GtkWidget *case_sensitive_checkbutton;
	GtkWidget *col_entry;
} sortinfo;

static void
gedit_plugin_finish (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG(widget));
}

static void
destroy_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_free (pd->name);
}

static int
my_compare( const void *s1, const void *s2 )
{
	gint l1, l2;

	l1 = strlen( s1 ) ;
	l2 = strlen( s2 ) ;
	if ( l1 < sortinfo.col_skip && l2 < sortinfo.col_skip )
		return 0 ;
	if ( l1 < sortinfo.col_skip )
		return -1 ;
	if ( l2 < sortinfo.col_skip )
		return 1 ;
	if ( sortinfo.case_sensitive )
		return strcmp( s1 + sortinfo.col_skip, s1 + sortinfo.col_skip ) ;
	else
		return strcasecmp( s1 + sortinfo.col_skip, s2 + sortinfo.col_skip ) ;
}

static void
callback_build_string( gpointer s, gchar **pointerptr )
{
	strcpy( *pointerptr, s ) ;
	*pointerptr += strlen( *pointerptr ) ;
	*(*pointerptr)++ = 0x0a ;
}

#define GLIST_MY_INSERT(S) rows = g_slist_insert_sorted( rows, (S), my_compare ) ;

/* the function that actually does the work
 * Here I am assuming that gedit_document_get_buffer allocates an extra byte and returns the buffer NULL-terminated
 * This was already true, but only recently it was documented
 */
static void
sort_document (void)
{
	GeditDocument *doc;
	GeditView *view;
	gchar *buffer, *target, *pointer;
	gint length, start, end, i;
	GSList *rows;

	view = gedit_view_active();
	if (view == NULL)
		return;

	doc = view->doc;

	buffer = gedit_document_get_buffer (doc);

	if (!gedit_view_get_selection (view, &start, &end))
	{
		start = 0;
		length = gedit_document_get_buffer_length (doc);
		end = length;
	}
	else
	{
		buffer[ end ] = 0 ;
		length = end - start ;
	}

	rows = NULL ;
	for ( i = end - 1 ; i >= start ; i -- )
		if ( buffer[ i ] == 0x0a || buffer[ i ] == '\0' )
		{
			buffer[ i ] = '\0' ;
			if ( i != end - 1 )
				GLIST_MY_INSERT( buffer + i + 1 ) ;
		}
	GLIST_MY_INSERT( buffer + start ) ;

	target = pointer = g_new( gchar, length + 1 ) ;

	g_slist_foreach (rows, callback_build_string, & pointer);
	*pointer = '\0' ;

	gedit_document_delete_text (doc, start, length, TRUE);
	gedit_document_insert_text (doc, target, start, TRUE);

	g_slist_free (rows);
	g_free (target);
	g_free (buffer);
}

static void
gedit_plugin_execute (GtkWidget *widget, gint button, gpointer data)
{
	if ( button == 0 )
	{
		sortinfo.col_skip = atoi(gtk_entry_get_text (GTK_ENTRY (sortinfo.col_entry))) - 1 ;
		sortinfo.case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sortinfo.case_sensitive_checkbutton));
		sort_document() ;
	}
	gnome_dialog_close (GNOME_DIALOG(widget));
}

static void
gedit_plugin_browse_create_dialog (void)
{
	GladeXML *gui;
	GtkWidget *dialog, *ok_button, *cancel_button;

	gedit_debug (DEBUG_PLUGINS, "");

	gui = glade_xml_new (GEDIT_GLADEDIR GEDIT_PLUGIN_GLADE_FILE, NULL);

	if (!gui)
	{
		g_warning ("Could not find %s", GEDIT_PLUGIN_GLADE_FILE);
		return;
	}

	dialog              = glade_xml_get_widget (gui, "dialog");
	sortinfo.col_entry	= glade_xml_get_widget (gui, "col_entry");
	ok_button			= glade_xml_get_widget (gui, "ok_button");
	cancel_button		= glade_xml_get_widget (gui, "cancel_button");
	sortinfo.case_sensitive_checkbutton = glade_xml_get_widget (gui, "case_sensitive_checkbutton");

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (sortinfo.col_entry != NULL);
	g_return_if_fail (ok_button != NULL);
	g_return_if_fail (cancel_button != NULL);
	g_return_if_fail (sortinfo.case_sensitive_checkbutton != NULL);

	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",GTK_SIGNAL_FUNC (gedit_plugin_execute),  NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",GTK_SIGNAL_FUNC (gedit_plugin_finish), NULL);

	/* Set the dialog parent and modal type */
	gnome_dialog_set_parent 	(GNOME_DIALOG (dialog),	 gedit_window_active());
	gtk_window_set_modal 		(GTK_WINDOW (dialog), TRUE);
	gnome_dialog_set_default     	(GNOME_DIALOG (dialog), 0);
	gnome_dialog_editable_enters 	(GNOME_DIALOG (dialog), GTK_EDITABLE (sortinfo.col_entry));

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}

gint
init_plugin (PluginData *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Sort");
	pd->desc = _("Sort a document.");
	pd->long_desc = _("Sort a document or selected text.");
	pd->author = "Carlo Borreo <borreo@softhome.net>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = TRUE;

	pd->private_data = (gpointer)gedit_plugin_browse_create_dialog;

	return PLUGIN_OK;
}
