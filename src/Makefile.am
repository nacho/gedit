## Process this file with automake to produce Makefile.in
SUBDIRS= dialogs

bin_PROGRAMS = gedit2

uidir = $(datadir)/gedit2/ui/


INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	-I$(top_srcdir)/dialogs				\
	$(GEDIT_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_LOCALEDIR=\""$(datadir)/locale"\" 	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit2/ui/"\" 	\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit2/plugins"\"\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\" 

gedit2_LDFLAGS = -export-dynamic

gedit2_LDADD = 				\
	dialogs/libdialogs.a		\
	$(GEDIT_LIBS)			

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h

gedit2_SOURCES = 			\
	gedit2.c			\
	gedit2.h			\
	gedit-mdi.c			\
	gedit-mdi.h			\
	gedit-mdi-child.c		\
	gedit-mdi-child.h		\
	gedit-document.c 		\
	gedit-document.h 		\
	gedit-view.c 			\
	gedit-view.h 			\
	gedit-menus.h			\
	gedit-menus.c			\
	gedit-debug.c 			\
	gedit-debug.h 			\
	gedit-prefs.c 			\
	gedit-prefs.h 			\
	gedit-utils.c 			\
	gedit-utils.h 			\
	gedit-commands.h		\
	gedit-commands.c		\
	gedit-file.h			\
	gedit-file.c			\
	gedit-recent.h			\
	gedit-recent.c			\
	gedit-undo-manager.h		\
	gedit-undo-manager.c		\
	gedit-print.h			\
	gedit-print.c			\
	bonobo-mdi-child.c		\
	bonobo-mdi-child.h		\
	bonobo-mdi.c			\
	bonobo-mdi.h			\
	gnome-vfs-helpers.c		\
	gnome-vfs-helpers.h		\
	gedit-file-selector-util.h 	\
	gedit-file-selector-util.c 	\
	gedit-plugins-engine.h		\
	gedit-plugins-engine.c		\
	gedit-plugin.h			\
	$(BUILT_SOURCES)

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

ui_DATA = gedit-ui.xml

EXTRA_DIST = \
	$(ui_DATA)	\
	gedit-marshal.list

CLEANFILES = $(BUILT_SOURCES)
