# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libemail.la

libemail_la_SOURCES = email.c

gladedir = $(datadir)/gedit/glade

INCLUDES =						\
	-I../../src 					\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR)
