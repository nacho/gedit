#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-file-browser-utils.h"

GdkPixbuf *
gedit_file_browser_utils_pixbuf_from_theme (gchar const *name,
                                            GtkIconSize size)
{
	GtkIconTheme *theme;
	gint width;
	GError *error = NULL;
	GdkPixbuf *scale;
	GdkPixbuf *pixbuf;

	theme = gtk_icon_theme_get_default ();
	gtk_icon_size_lookup (size, &width, NULL);

	pixbuf = gtk_icon_theme_load_icon (theme, name, width, 0, &error);

	if (error != NULL) {
		g_warning ("Could not load theme icon %s: %s", name,
			   error->message);
		g_error_free (error);
	}
	
	if (pixbuf && gdk_pixbuf_get_width (pixbuf) > width) {
		scale = gdk_pixbuf_scale_simple (pixbuf, 
		                                 width, 
		                                 width, 
		                                 GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scale;
	}

	return pixbuf;
}

GdkPixbuf *
gedit_file_browser_utils_pixbuf_from_mime_type (gchar const *uri,
                                                gchar const *mime,
                                                GtkIconSize size)
{
	GtkIconTheme *theme;
	GdkPixbuf *pixbuf;
	gchar *name;

	theme = gtk_icon_theme_get_default ();

	name =
	    gnome_icon_lookup (theme, NULL, uri, NULL, NULL, mime,
			       GNOME_ICON_LOOKUP_FLAGS_NONE,
			       GNOME_ICON_LOOKUP_RESULT_FLAGS_NONE);

	pixbuf = gedit_file_browser_utils_pixbuf_from_theme (name, size);
	g_free (name);
	
	return pixbuf;
}

gchar *
gedit_file_browser_utils_uri_basename (gchar const * uri)
{
	gchar * basename;
	gchar * normal;
	
	basename = g_filename_display_basename (uri);
	normal = gnome_vfs_unescape_string_for_display (basename);
	
	g_free (basename);
	
	return normal;
}

gboolean
gedit_file_browser_utils_confirmation_dialog (GeditWindow * window,
                                              GtkMessageType type,
                                              gchar const *message,
		                              gchar const *secondary, 
		                              gchar const * button_stock, 
		                              gchar const * button_label)
{
	GtkWidget *dlg;
	gint ret;
	GtkWidget *button;

	dlg = gtk_message_dialog_new (GTK_WINDOW (window),
				      GTK_DIALOG_MODAL |
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      type,
				      GTK_BUTTONS_NONE, message, NULL);

	if (secondary)
		gtk_message_dialog_format_secondary_text
		    (GTK_MESSAGE_DIALOG (dlg), secondary, NULL);

	/* Add a cancel button */
	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (button);
	
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
                                      button,
                                      GTK_RESPONSE_CANCEL);

	/* Add custom button */
	button = gtk_button_new_from_stock (button_stock);
	
	if (button_label) {
		gtk_button_set_use_stock (GTK_BUTTON (button), FALSE);
		gtk_button_set_label (GTK_BUTTON (button), button_label);
	}
	
	gtk_widget_show (button);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
                                      button,
                                      GTK_RESPONSE_OK);
	
	ret = gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);

	return (ret == GTK_RESPONSE_OK);
}

gboolean 
gedit_file_browser_utils_is_local (gchar const * uri)
{
	return strncmp(uri, "file://", 7) == 0;
}
