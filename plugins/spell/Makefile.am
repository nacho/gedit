# Spell checker plugin
plugindir = $(libdir)/gedit-2/plugins

BUILT_SOURCES = \
	gedit-spell-checker-dialog-marshal.c		\
	gedit-spell-checker-dialog-marshal.h

	
INCLUDES = \
	-I$(top_srcdir)/src 				\
	$(GEDIT_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			

plugin_LTLIBRARIES = libspell.la

libspell_la_SOURCES = spell.c 				\
	gedit-spell-checker.c 				\
	gedit-spell-checker.h				\
	gedit-spell-checker-dialog.c			\
	gedit-spell-checker-dialog.h			\
	gedit-spell-language-dialog.c			\
	gedit-spell-language-dialog.h			\
	gedit-automatic-spell-checker.c			\
	gedit-automatic-spell-checker.h			\
	$(BUILT_SOURCES)				\
	preload.c					\
	preload.h					

libspell_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) 
libspell_la_LIBADD =  $(PSPELL_LIBS)

gladedir = $(datadir)/gedit-2/glade
glade_DATA = spell-checker.glade2 languages-dialog.glade2

gedit-spell-checker-dialog-marshal.h: gedit-spell-checker-dialog-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-spell-checker-dialog-marshal.c: gedit-spell-checker-dialog-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-spell-checker-dialog-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

plugin_in_files = spell.gedit-plugin.in

%.gedit-plugin: %.gedit-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.gedit-plugin.in=.gedit-plugin)

EXTRA_DIST = $(glade_DATA) $(plugin_DATA) gedit-spell-checker-dialog-marshal.list 

CLEANFILES = $(BUILT_SOURCES) 
