# Shell output plugin
plugindir = $(libdir)/gedit2/plugins

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"

plugin_LTLIBRARIES = libshell_output.la

libshell_output_la_SOURCES = shell_output.c

gladedir = $(datadir)/gedit2/glade
glade_DATA = shell_output.glade2

EXTRA_DIST = $(glade_DATA)
