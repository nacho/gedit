# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libemail.la

libemail_la_SOURCES = email.c


INCLUDES =						\
	-I../../src 					\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)

gladedir = $(datadir)/gedit/glade
glade_DATA = email.glade

EXTRA_DIST = $(glade_DATA)