# Reverse plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libreverse.la

libreverse_la_SOURCES = reverse.c

INCLUDES = -I../../src \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR)
