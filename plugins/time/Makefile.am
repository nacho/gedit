# time plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)			

plugin_LTLIBRARIES = libtime.la

libtime_la_SOURCES = \
	gedit-time-plugin.h	\
	gedit-time-plugin.c

libtime_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libtime_la_LIBADD  = $(GEDIT_LIBS)

uidir = $(GEDIT_PLUGINS_DATA_DIR)/time
ui_DATA = \
	gedit-time-dialog.ui				\
	gedit-time-setup-dialog.ui

plugin_in_files = time.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

gsettings_ENUM_NAMESPACE = org.gnome.gedit.plugins.time
gsettings_ENUM_FILES = $(top_srcdir)/plugins/time/gedit-time-plugin.c
gsettings_SCHEMAS = org.gnome.gedit.time.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST =					\
	$(ui_DATA)				\
	$(plugin_in_files)			\
	org.gnome.gedit.time.gschema.xml.in.in

CLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

-include $(top_srcdir)/git.mk
