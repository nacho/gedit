# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libtime.la

libtime_la_SOURCES = time.c

INCLUDES = -I../../src \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR)
