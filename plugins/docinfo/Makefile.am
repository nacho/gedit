# docinfo plugin
plugindir = $(libdir)/gedit-2/plugins

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libdocinfo.la

libdocinfo_la_SOURCES = \
	gedit-docinfo-plugin.h	\
	gedit-docinfo-plugin.c

libdocinfo_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdocinfo_la_LIBADD  = $(GEDIT_LIBS)

uidir = $(datadir)/gedit-2/plugins/docinfo
ui_DATA = docinfo.ui

plugin_in_files = docinfo.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(ui_DATA) $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

