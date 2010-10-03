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

plugin_in_files = checkupdate.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

gsettings_SCHEMAS = org.gnome.gedit.plugins.checkupdate.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(plugin_in_files)	\
	org.gnome.gedit.plugins.checkupdate.gschema.xml.in.in

CLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS)

DISTCLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

-include $(top_srcdir)/git.mk
