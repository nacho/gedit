plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libdiff.la

libdiff_la_SOURCES = diff.c

INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)

gladedir = $(datadir)/gedit/glade
glade_DATA = diff.glade

EXTRA_DIST = $(glade_DATA)
