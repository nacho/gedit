# Tag list plugin
plugindir = $(libdir)/gedit2/plugins

taglistdir = $(datadir)/gedit2/taglist
taglist_DATA = taglist.xml

INCLUDES = \
	-I$(top_srcdir)/src 				\
	$(GEDIT_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"	\
	-DGEDIT_TAGLIST=\""$(taglistdir)/taglist.xml"\"	\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			

plugin_LTLIBRARIES = libtaglist.la

libtaglist_la_SOURCES = \
	gedit-taglist-plugin-parser.c	\
	gedit-taglist-plugin-parser.h	\
	gedit-taglist-plugin-window.c	\
	gedit-taglist-plugin-window.h	\
	gedit-taglist-plugin.c		\
	gedit-taglist-plugin.h


EXTRA_DIST = $(taglist_DATA)
