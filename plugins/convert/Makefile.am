# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libconvert.la

libconvert_la_SOURCES = convert.c

INCLUDES = 						\
	-I../../src 					\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)
