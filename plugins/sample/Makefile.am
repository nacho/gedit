# Sample plugin
plugindir = $(libdir)/gedit2/plugins

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"

plugin_LTLIBRARIES = libsample.la

libsample_la_SOURCES = sample.c

