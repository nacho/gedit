# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libsample.la

libsample_la_SOURCES = sample.c

INCLUDES =						\
	-I$(top_srcdir)/src 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)

