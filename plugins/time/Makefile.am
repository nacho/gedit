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

gsettingsschema_DATA = org.gnome.gedit.time.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@

if GSETTINGS_SCHEMAS_INSTALL
install-data-hook:
	$(GLIB_COMPILE_SCHEMAS) $(gsettingsschemadir)
endif

EXTRA_DIST = $(ui_DATA) $(plugin_in_files) $(gsettingsschema_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

-include $(top_srcdir)/git.mk
