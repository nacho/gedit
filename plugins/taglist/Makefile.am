# Tag list plugin
plugindir = $(libdir)/gedit-2/plugins

taglistdir = $(datadir)/gedit-2/taglist
taglist_DATA = \
	HTML.tags	\
	Latex.tags	\
	XSLT.tags

INCLUDES = 							\
	-I$(top_srcdir) 					\
	$(GEDIT_CFLAGS) 					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DGEDIT_TAGLIST_DIR=\""$(taglistdir)/"\"

plugin_LTLIBRARIES = libtaglist.la

libtaglist_la_SOURCES = \
	gedit-taglist-plugin-parser.c	\
	gedit-taglist-plugin-parser.h	\
	gedit-taglist-plugin-window.c	\
	gedit-taglist-plugin-window.h	\
	gedit-taglist-plugin.c		\
	gedit-taglist-plugin.h

libtaglist_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

plugin_in_files = taglist.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = $(taglist_DATA) $(plugin_in_files) $(plugin_DATA)

CLEANFILES = $(plugin_DATA)

