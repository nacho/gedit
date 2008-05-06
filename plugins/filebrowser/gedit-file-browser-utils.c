#include <libgnomeui/libgnomeui.h>

#include "gedit-file-browser-utils.h"

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

static GdkPixbuf *
pixbuf_from_theme_icon (GThemedIcon * icon, gint size)
{
	gchar **names;
	GtkIconInfo * info;
	GdkPixbuf * pixbuf;
	GError * error = NULL;
	gchar * name;
	
	// Get the icon theme names
	g_object_get (icon, "names", &names, NULL);
	info = gtk_icon_theme_choose_icon (gtk_icon_theme_get_default (), 
					   (gchar const **)names, 
					   size, 
					   0);
	
	pixbuf = gtk_icon_info_load_icon (info, &error);
	gtk_icon_info_free (info);
	
	name = g_strjoinv (", ", names);
	pixbuf = process_icon_pixbuf (pixbuf, name, size, error);

	g_free (name);
	g_strfreev (names);

	return pixbuf;	
}

static GdkPixbuf *
pixbuf_from_loadable_icon (GLoadableIcon * icon, gint size)
{
	// TODO: actual implement this
	return NULL;
}

GdkPixbuf *
gedit_file_browser_utils_pixbuf_from_icon (GIcon * icon,
                                           GtkIconSize size)
{
	GdkPixbuf * ret = NULL;
	gint width;
	
	if (!icon)
		return NULL;

	gtk_icon_size_lookup (size, &width, NULL);
	
	if (G_IS_THEMED_ICON (icon))
		ret = pixbuf_from_theme_icon (G_THEMED_ICON (icon), width);
	else if (G_IS_LOADABLE_ICON (icon))
		ret = pixbuf_from_loadable_icon (G_LOADABLE_ICON (icon), width);
	
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
	gchar * ret;
	
	GFileInfo * info = g_file_query_info (file,
					     G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, 
					     G_FILE_QUERY_INFO_NONE,
					     NULL, 
					     NULL);
	
	if (!info)
		return NULL;

	ret = g_strdup(g_file_info_get_display_name (info));
	g_object_unref (info);
	
	return ret;
}

gchar *
gedit_file_browser_utils_file_basename (GFile * file)
{
	gchar * display = gedit_file_browser_utils_file_display (file);
	gchar * ret;

	if (!display)
		return g_file_get_basename (file);

	ret = g_path_get_basename (display);
	
	g_free (display);
	return ret;
}

gchar *
gedit_file_browser_utils_uri_basename (gchar const * uri)
{
	GFile * file = g_file_new_for_uri (uri);
	gchar * ret = gedit_file_browser_utils_file_basename (file);
	
	g_object_unref (file);
	
	return ret;
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
_gedit_file_browser_utils_file_has_parent (GFile * file)
{
	GFile * parent = g_file_get_parent (file);
	
	if (!parent)
		return FALSE;
	
	g_object_unref (parent);
	return TRUE;
}

// ex:ts=8:noet:
