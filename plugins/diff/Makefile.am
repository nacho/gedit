plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libdiff.la

libdiff_la_SOURCES = diff.c

INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

gladedir = $(datadir)/gedit/glade
glade_DATA = diff.glade
glade_msgs = diff.glade.h

EXTRA_DIST = $(glade_DATA) $(glade_msgs)

