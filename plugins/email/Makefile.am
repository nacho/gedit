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

glade_msgs = email.glade.h

EXTRA_DIST = $(glade_DATA) $(glade_msgs)