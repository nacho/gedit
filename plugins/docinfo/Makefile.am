# Document Info plugin
plugindir = $(libdir)/gedit2/plugins

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"

plugin_LTLIBRARIES = libdocinfo.la

libdocinfo_la_SOURCES = docinfo.c

gladedir = $(datadir)/gedit2/glade
glade_DATA = docinfo.glade

EXTRA_DIST = $(glade_DATA)
