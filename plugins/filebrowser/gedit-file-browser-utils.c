#include "gedit-file-browser-utils.h"
#include <gedit/gedit-utils.h>

static GdkPixbuf *
process_icon_pixbuf (GdkPixbuf * pixbuf,
		     gchar const * name, 
		     gint size, 
		     GError * error)
{
	GdkPixbuf * scale;

	if (error != NULL) {
		g_warning ("Could not load theme icon %s: %s",
			   name,
			   error->message);
		g_error_free (error);
	}
	
	if (pixbuf && gdk_pixbuf_get_width (pixbuf) > size) {
		scale = gdk_pixbuf_scale_simple (pixbuf, 
		                                 size, 
		                                 size, 
		                                 GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scale;
	}

	return pixbuf;
}

GdkPixbuf *
gedit_file_browser_utils_pixbuf_from_theme (gchar const * name,
                                            GtkIconSize size)
{
	gint width;
	GError *error = NULL;
	GdkPixbuf *pixbuf;

	gtk_icon_size_lookup (size, &width, NULL);

	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), 
					   name, 
					   width, 
					   0, 
					   &error);

	pixbuf = process_icon_pixbuf (pixbuf, name, width, error);

	return pixbuf;
}

GdkPixbuf *
gedit_file_browser_utils_pixbuf_from_icon (GIcon * icon,
                                           GtkIconSize size)
{
	GdkPixbuf * ret = NULL;
	GtkIconTheme *theme;
	GtkIconInfo *info;
	
	if (!icon)
		return NULL;

	theme = gtk_icon_theme_get_default ();
	info = gtk_icon_theme_lookup_by_gicon (theme, icon, size, 0);
	
	if (!info)
		return NULL;
		
	ret = gtk_icon_info_load_icon (info, NULL);
	gtk_icon_info_free (info);
	
	return ret;
}

GdkPixbuf *
gedit_file_browser_utils_pixbuf_from_file (GFile * file,
                                           GtkIconSize size)
{
	GIcon * icon;
	GFileInfo * info;
	GdkPixbuf * ret = NULL;

	info = g_file_query_info (file, 
				  G_FILE_ATTRIBUTE_STANDARD_ICON, 
				  G_FILE_QUERY_INFO_NONE,
				  NULL, 
				  NULL);
	
	if (!info)
		return NULL;

	icon = g_file_info_get_icon (info);
	ret = gedit_file_browser_utils_pixbuf_from_icon (icon, size);
	g_object_unref (info);
	
	return ret;
}

gchar *
gedit_file_browser_utils_file_display (GFile * file)
{
	gchar *uri;
	gchar *ret;
	
	uri = g_file_get_uri (file);
	ret = gedit_utils_uri_for_display (uri);
	g_free (uri);
	
	return ret;
}

gchar *
gedit_file_browser_utils_file_basename (GFile * file)
{
	gchar *uri;
	gchar *ret;
	
	uri = g_file_get_uri (file);
	ret = gedit_file_browser_utils_uri_basename (uri);
	g_free (uri);
	
	return ret;
}

gchar *
gedit_file_browser_utils_uri_basename (gchar const * uri)
{
	return gedit_utils_basename_for_display (uri);
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

// ex:ts=8:noet:
