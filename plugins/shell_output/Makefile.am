# Shell output plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libshell_output.la

libshell_output_la_SOURCES = shell_output.c


INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)

gladedir = $(datadir)/gedit/glade
glade_DATA = shell_output.glade

glade_msgs = shell_output.glade.h

EXTRA_DIST = $(glade_DATA) $(glade_msgs)