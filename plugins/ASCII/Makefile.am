# ASCII table plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libasciitable.la

libasciitable_la_SOURCES = ascii.c


INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(EXTRA_GNOME_CFLAGS) \
	$(GNOME_INCLUDEDIR)

gladedir = $(datadir)/gedit/glade
glade_DATA = asciitable.glade

EXTRA_DIST = $(glade_DATA)
