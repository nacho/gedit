# Sample plugin

plugindir = $(libdir)/gedit/plugins

plugin_LTLIBRARIES = libcvschangelog.la

libcvschangelog_la_SOURCES = cvschangelog.c

INCLUDES =						\
	-I$(top_srcdir)/src 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)					\
	$(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS)

