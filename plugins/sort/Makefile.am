# Sort plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libsort.la

libsort_la_SOURCES = sort.c

INCLUDES = -I$(top_srcdir)/src \
	   -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	   -DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	   -I$(includedir)	\
		$(GNOME_INCLUDEDIR) \
		$(EXTRA_GNOME_CFLAGS)


gladedir = $(datadir)/gedit/glade
glade_DATA = sort.glade

EXTRA_DIST = $(glade_DATA)
