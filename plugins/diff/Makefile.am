plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libdiff.la

libdiff_la_SOURCES = diff.c

INCLUDES = -I../../src \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) \
	$(GNOME_INCLUDEDIR)