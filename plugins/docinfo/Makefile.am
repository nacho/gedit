# Document Info plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libdocinfo.la

libdocinfo_la_SOURCES = docinfo.c


INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

gladedir = $(datadir)/gedit/glade
glade_DATA = docinfo.glade

glade_msgs = docinfo.glade.h 

EXTRA_DIST = $(glade_DATA) $(glade_msgs)
