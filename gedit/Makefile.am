## Process this file with automake to produce Makefile.in
SUBDIRS= dialogs recent-files

bin_PROGRAMS = gedit

INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(srcdir)						\
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

noinst_LTLIBRARIES = libgedit.la

gedit_LDFLAGS = -export-dynamic

gedit_LDADD =				\
	$(GEDIT_LIBS)			\
	libgedit.la			\
	dialogs/libdialogs.la		\
	recent-files/librecent.la	

IDL_FILE = GNOME_Gedit.idl

IDL_SOURCES = 				\
	GNOME_Gedit-stubs.c		\
	GNOME_Gedit-common.c		\
	GNOME_Gedit-skels.c		\
	GNOME_Gedit.h		

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h	

NON_INST_H_FILS =			\
	gedit-application-server.h	\
	bonobo-mdi-session.h		\
	gedit-commands.h		\
	gedit-file-selector-util.h 	\
	gedit-plugins-engine.h		\
	gedit-session.h			\
	gedit-languages-manager.h	\
	gedit-window-server.h		\
	gedit-encodings-option-menu.h	

INST_H_FILES =				\
	gedit2.h			\
	gedit-file.h			\
	gedit-utils.h 			\
	gedit-mdi.h			\
	gedit-mdi-child.h		\
	gedit-document.h 		\
	gedit-view.h 			\
	gedit-menus.h			\
	bonobo-mdi.h			\
	gedit-print.h			\
	bonobo-mdi-child.h		\
	gedit-plugin.h			\
	gedit-prefs-manager-app.h	\
	gedit-recent.h			\
	gedit-output-window.h

LIB_H_FILES =				\
	gedit-encodings.h		\
	gedit-convert.h			\
	gedit-debug.h			\
	gedit-prefs-manager.h		\
	gedit-metadata-manager.h	


headerdir = $(prefix)/include/gedit-@GEDIT_MAJOR@/gedit

header_DATA = 				\
	$(INST_H_FILES)			\
	$(LIB_H_FILES)

gedit_SOURCES = 			\
	$(IDL_SOURCES)			\
	$(BUILT_SOURCES)		\
	gedit2.c			\
	gedit-mdi.c			\
	gedit-mdi-child.c		\
	gedit-document.c 		\
	gedit-view.c 			\
	gedit-menus.c			\
	gedit-utils.c 			\
	gedit-commands.c		\
	gedit-file.c			\
	gedit-print.c			\
	gedit-session.c			\
	bonobo-mdi.c			\
	bonobo-mdi-child.c		\
	bonobo-mdi-session.c		\
	gedit-file-selector-util.c 	\
	gedit-plugins-engine.c		\
	gedit-plugin.c			\
	gedit-prefs-manager-app.c	\
	gedit-recent.c			\
	gedit-application-server.c	\
	gedit-document-server.c		\
	gedit-document-server.h		\
	gedit-window-server.c		\
	gedit-output-window.c		\
	gedit-languages-manager.c	\
	gedit-encodings-option-menu.c	\
	$(NON_INST_H_FILS)		\
	$(INST_H_FILES)

libgedit_la_SOURCES = 			\
	gedit-debug.c			\
	gedit-convert.c			\
	gedit-encodings.c		\
	gedit-prefs-manager.c		\
	gedit-prefs-manager-private.h	\
	gedit-metadata-manager.c	\
	$(LIB_H_FILES)

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

$(IDL_SOURCES): $(IDL_FILE)
	$(ORBIT_IDL) -I $(LIBBONOBO_IDL) -I $(BONOBO_ACTIVATION_IDL) $(IDL_FILE)

ui_DATA = gedit-ui.xml
uidir = $(datadir)/gedit-2/ui/

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

