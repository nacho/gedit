# ASCII table plugin
plugindir = $(libdir)/gedit2/plugins

plugin_LTLIBRARIES = libasciitable.la

libasciitable_la_SOURCES = ascii.c


INCLUDES =						\
	-I$(top_srcdir)/src 				\
	$(GEDIT_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"	\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			


gladedir = $(datadir)/gedit2/glade
glade_DATA = asciitable.glade2

EXTRA_DIST = $(glade_DATA)


