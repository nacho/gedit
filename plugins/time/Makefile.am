# Insert Time plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libtime.la

libtime_la_SOURCES = time.c

INCLUDES =						\
	-I$(top_srcdir)/src				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)	\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)

