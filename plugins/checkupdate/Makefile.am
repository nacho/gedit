# gedit win32 updater

plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 						\
	$(GEDIT_CFLAGS) 						\
	$(LIBSOUP_CFLAGS)						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libcheckupdate.la

libcheckupdate_la_SOURCES = \
	gedit-check-update-plugin.h	\
	gedit-check-update-plugin.c

libcheckupdate_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libcheckupdate_la_LIBADD = $(GEDIT_LIBS) $(LIBSOUP_LIBS)

plugin_in_files = checkupdate.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA)

DISTCLEANFILES = $(plugin_DATA)

-include $(top_srcdir)/git.mk
