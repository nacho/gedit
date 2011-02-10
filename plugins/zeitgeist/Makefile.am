# zeitgeist plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(ZEITGEIST_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libzeitgeistplugin.la

libzeitgeistplugin_la_SOURCES = \
	gedit-zeitgeist-plugin.h	\
	gedit-zeitgeist-plugin.c

libzeitgeistplugin_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libzeitgeistplugin_la_LIBADD = $(GEDIT_LIBS) $(ZEITGEIST_LIBS)

plugin_in_files = zeitgeist.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

-include $(top_srcdir)/git.mk
