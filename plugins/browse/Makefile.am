plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libbrowse.la

libbrowse_la_SOURCES = browse.c

INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)

gladedir = $(datadir)/gedit/glade
glade_DATA = browse.glade
glade_msgs = browse.glade.h

EXTRA_DIST = $(glade_DATA) $(glade_msgs)

