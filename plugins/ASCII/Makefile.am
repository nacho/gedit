# ASCII table plugin
plugindir = $(libdir)/gedit-2/plugins

plugin_LTLIBRARIES = libasciitable.la

libasciitable_la_SOURCES = ascii.c
libasciitable_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)


INCLUDES =						\
	-I$(top_srcdir)/src 				\
	$(GEDIT_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			


gladedir = $(datadir)/gedit-2/glade
glade_DATA = asciitable.glade2

EXTRA_DIST = $(glade_DATA)


