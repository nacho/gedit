# Shell output plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libspell.la

libspell_la_SOURCES = spell.c


INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

gladedir = $(datadir)/gedit/glade
glade_DATA = spell.glade

glade_msgs = spell.glade.h

EXTRA_DIST = $(glade_DATA) $(glade_msgs)