# filebrowser

plugindir = $(libdir)/gedit-2/plugins
filebrowserdir = $(datadir)/gedit-2/plugins/filebrowser
filebrowser_DATA = gedit-file-browser-widget-ui.xml

INCLUDES = \
	-I$(top_srcdir) 						\
	$(GEDIT_CFLAGS) 						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)					\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DGEDIT_FILE_BROWSER_DIR=\""$(filebrowserdir)/"\"

BUILT_SOURCES = \
	gedit-file-browser-enum-types.h		\
	gedit-file-browser-enum-types.c		\
	gedit-file-browser-marshal.h 		\
	gedit-file-browser-marshal.c

plugin_LTLIBRARIES = libfilebrowser.la

NOINST_H_FILES = \
	gedit-file-bookmarks-store.h 		\
	gedit-file-browser-store.h 		\
	gedit-file-browser-view.h 		\
	gedit-file-browser-widget.h 		\
	gedit-file-browser-error.h		\
	gedit-file-browser-utils.h		\
	gedit-file-browser-plugin.h

libfilebrowser_la_SOURCES = \
	$(BUILT_SOURCES) 			\
	gedit-file-bookmarks-store.c 		\
	gedit-file-browser-store.c 		\
	gedit-file-browser-view.c 		\
	gedit-file-browser-widget.c 		\
	gedit-file-browser-utils.c 		\
	gedit-file-browser-plugin.c		\
	$(NOINST_H_FILES)

libfilebrowser_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

# Glade files (if you use glade for your plugin, list those files here)
gladedir = $(datadir)/gedit-2/glade
glade_DATA =

plugin_in_files = filebrowser.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

gedit-file-browser-enum-types.h: gedit-file-browser-enum-types.h.template $(NOINST_H_FILES) $(GLIB_MKENUMS)
	(cd $(srcdir) && $(GLIB_MKENUMS) --template gedit-file-browser-enum-types.h.template $(NOINST_H_FILES)) > $@
	
gedit-file-browser-enum-types.c: gedit-file-browser-enum-types.c.template gedit-file-browser-enum-register.c.template $(NOINST_H_FILES) $(GLIB_MKENUMS)
	(cd $(srcdir) && \
	 $(GLIB_MKENUMS) --template gedit-file-browser-enum-types.c.template $(NOINST_H_FILES) && \
	 $(GLIB_MKENUMS) --template gedit-file-browser-enum-register.c.template $(NOINST_H_FILES)) > $@

gedit-file-browser-marshal.h: gedit-file-browser-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_file_browser_marshal > $@

gedit-file-browser-marshal.c: gedit-file-browser-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-file-browser-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_file_browser_marshal >> $@

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gedit-file-browser.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/plugins/filebrowser/$$p ; \
	                done \
	        fi
else
install-data-local:
endif


EXTRA_DIST = \
	$(filebrowser_DATA)				\
	$(plugin_in_files)				\
	$(schemas_in_files)				\
	gedit-file-browser-enum-types.h.template	\
	gedit-file-browser-enum-types.c.template	\
	gedit-file-browser-enum-register.c.template	\
	gedit-file-browser-marshal.list

CLEANFILES = \
	$(plugin_DATA)		\
	$(glade_DATA) 		\
	$(schemas_DATA)		\
	$(BUILT_SOURCES)

DISTCLEANFILES = \
	$(plugin_DATA)		\
	$(glade_DATA)		\
	$(schemas_DATA)		\
	$(BUILT_SOURCES)
