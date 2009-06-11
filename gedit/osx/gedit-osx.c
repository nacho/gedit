#include "gedit-osx.h"
#include <gdk/gdkquartz.h>
#include <Carbon/Carbon.h>

void
gedit_osx_set_window_title (GeditWindow   *window, 
			    gchar const   *title,
			    GeditDocument *document)
{
	NSWindow *native;

	g_return_if_fail (GEDIT_IS_WINDOW (window));

	if (GTK_WIDGET (window)->window == NULL)
	{
		return;
	}

	native = gdk_quartz_window_get_nswindow (GTK_WIDGET (window)->window);

	if (document)
	{
		bool ismodified;

		if (gedit_document_is_untitled (document))
		{
			[native setRepresentedURL:nil];
		}
		else
		{
			const gchar *uri = gedit_document_get_uri (document);
			NSURL *nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:uri]];
			
			[native setRepresentedURL:nsurl];
		}

		ismodified = !gedit_document_is_untouched (document); 
		[native setDocumentEdited:ismodified];
	}
	else
	{
		[native setRepresentedURL:nil];
		[native setDocumentEdited:false];
	}

	gtk_window_set_title (GTK_WINDOW (window), title);
}

gboolean
gedit_osx_show_url (const gchar *url)
{
 	return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
}

gboolean
gedit_osx_show_help (const gchar *link_id)
{
	gchar *link;
	gboolean ret;

	if (link_id)
	{
		link = g_strdup_printf ("http://library.gnome.org/users/gedit/stable/%s",
					link_id);
	}
	else
	{
		link = g_strdup ("http://library.gnome.org/users/gedit/stable/");
	}

	ret = gedit_osx_show_url (link);
	g_free (link);

	return ret;
}
