## Process this file with automake to produce Makefile.in
SUBDIRS= dialogs recent-files

bin_PROGRAMS = gedit

uidir = $(datadir)/gedit-2/ui/


INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/dialogs					\
	-I$(top_srcdir)/recent-files				\
	$(GEDIT_CFLAGS)						\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\"		\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-DGTK_DISABLE_DEPRECATED				\
	-DGDK_PIXBUF_DISABLE_DEPRECATED				\
	-DGNOME_DISABLE_DEPRECATED

gedit_LDFLAGS = -export-dynamic

gedit_LDADD = 				\
	dialogs/libdialogs.a		\
	recent-files/librecent.a	\
	$(GEDIT_LIBS)			

IDL_FILE = GNOME_Gedit.idl

IDL_SOURCES = \
	GNOME_Gedit-stubs.c			\
	GNOME_Gedit-common.c			\
	GNOME_Gedit-skels.c			\
	GNOME_Gedit.h		

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h	

gedit_SOURCES = 			\
	$(IDL_SOURCES)			\
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
	gedit-file-selector-util.h 	\
	gedit-file-selector-util.c 	\
	gedit-plugins-engine.h		\
	gedit-plugins-engine.c		\
	gedit-plugin.h			\
	gedit-plugin.c			\
	gedit-prefs-manager.c		\
	gedit-prefs-manager.h		\
	gedit-recent.c			\
	gedit-recent.h			\
	gedit-application-server.c	\
	gedit-application-server.h	\
	gedit-document-server.c		\
	gedit-document-server.h		\
	gedit-window-server.c		\
	gedit-window-server.h		\
	gedit-encodings.c		\
	gedit-encodings.h		\
	gedit-output-window.c		\
	gedit-output-window.h		\
	$(BUILT_SOURCES)

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

$(IDL_SOURCES): $(IDL_FILE)
	$(ORBIT_IDL) -I $(LIBBONOBO_IDL) -I $(BONOBO_ACTIVATION_IDL) $(IDL_FILE)

ui_DATA = gedit-ui.xml

idl_DATA = $(IDL_FILE)
idldir = $(datadir)/idl

EXTRA_DIST = 				\
	$(ui_DATA)			\
	gedit-marshal.list		\
	$(IDL_FILE)

CLEANFILES = $(BUILT_SOURCES) $(IDL_SOURCES)

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor

