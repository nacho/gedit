## Process this file with automake to produce Makefile.in
SUBDIRS= dialogs

bin_PROGRAMS = gedit

uidir = $(datadir)/gedit-2/ui/


INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	-I$(top_srcdir)/dialogs				\
	$(GEDIT_CFLAGS) 				\
	-DDATADIR=\""$(datadir)"\"			\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_LOCALEDIR=\""$(datadir)/locale"\" 	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\" 	\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\" \
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED

gedit_LDFLAGS = -export-dynamic

gedit_LDADD = 				\
	dialogs/libdialogs.a		\
	$(GEDIT_LIBS)			

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h			\
	gnome-recent-marshal.c		\
	gnome-recent-marshal.h

gedit_SOURCES = 			\
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
	gedit-undo-manager.h		\
	gedit-undo-manager.c		\
	gedit-print.h			\
	gedit-print.c			\
	gedit-session.c			\
	gedit-session.h			\
	bonobo-mdi.c			\
	bonobo-mdi.h			\
	bonobo-mdi-child.c		\
	bonobo-mdi-child.h		\
	bonobo-mdi-session.c		\
	bonobo-mdi-session.h		\
	gnome-vfs-helpers.c		\
	gnome-vfs-helpers.h		\
	gedit-file-selector-util.h 	\
	gedit-file-selector-util.c 	\
	gedit-plugins-engine.h		\
	gedit-plugins-engine.c		\
	gedit-plugin.h			\
	gedit-plugin.c			\
	gnome-recent-model.c		\
	gnome-recent-model.h		\
	gnome-recent-view.c		\
	gnome-recent-view.h		\
	gnome-recent-view-bonobo.c	\
	gnome-recent-view-bonobo.h	\
	gnome-recent-util.c		\
	gnome-recent-util.h		\
	gedit-recent.c			\
	gedit-recent.h			\
	$(BUILT_SOURCES)

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

gnome-recent-marshal.h: gnome-recent-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gnome_recent > $@

gnome-recent-marshal.c: gnome-recent-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gnome-recent-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gnome_recent >> $@

ui_DATA = gedit-ui.xml

EXTRA_DIST = \
	$(ui_DATA)	\
	gedit-marshal.list	\
	gnome-recent-marshal.list

CLEANFILES = $(BUILT_SOURCES)
