plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libdiff.la

libdiff_la_SOURCES = diff.c

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) 				\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src				\
	$(GNOME_INCLUDEDIR)