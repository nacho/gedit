# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libsample.la

libsample_la_SOURCES = sample.c

INCLUDES = -I../../src \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR)
