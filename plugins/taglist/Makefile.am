# Tag list plugin
plugindir = $(libdir)/gedit-2/plugins

taglistdir = $(datadir)/gedit-2/taglist

taglist_in_files = \
	HTML.tags.xml.in	\
	Latex.tags.xml.in	\
	XSLT.tags.xml.in \
	XUL.tags.xml.in

taglist_DATA = $(taglist_in_files:.tags.xml.in=.tags)

INCLUDES = 							\
	-I$(top_srcdir) 					\
	$(GEDIT_CFLAGS) 					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DGEDIT_TAGLIST_DIR=\""$(taglistdir)/"\"

plugin_LTLIBRARIES = libtaglist.la

libtaglist_la_SOURCES = \
	gedit-taglist-plugin-parser.c	\
	gedit-taglist-plugin-parser.h	\
	gedit-taglist-plugin-panel.c	\
	gedit-taglist-plugin-panel.h	\
	gedit-taglist-plugin.c		\
	gedit-taglist-plugin.h

libtaglist_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libtaglist_la_LIBADD  = $(GEDIT_LIBS)

plugin_in_files = taglist.gedit-plugin.desktop.in

%.gedit-plugin: %.gedit-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

%.tags: %.tags.xml.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -x -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.desktop.in=.gedit-plugin)

EXTRA_DIST = \
	$(taglist_in_files) $(taglist_DATA)	\
	$(plugin_in_files) $(plugin_DATA)	

CLEANFILES = \
	$(taglist_DATA)	\
 	$(plugin_DATA)
DISTCLEANFILES = \
	$(taglist_DATA)	\
 	$(plugin_DATA)		



