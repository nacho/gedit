# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libemail.la

libemail_la_SOURCES = email.c

INCLUDES = -I../../src \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR)
