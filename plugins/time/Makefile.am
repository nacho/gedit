# Sample plugin
plugindir = $(libdir)/gedit2/plugins

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"

plugin_LTLIBRARIES = libtime.la

libtime_la_SOURCES = time.c

gladedir = $(datadir)/gedit2/glade
glade_DATA = time.glade2

EXTRA_DIST = $(glade_DATA)
