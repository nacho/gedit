## Process this file with automake to produce Makefile.in
SUBDIRS= dialogs

bin_PROGRAMS = gedit2

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/dialogs	\
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
	-DGEDIT_LOCALEDIR=\""$(datadir)/locale"\"


CFLAGS = -g -O2 -Wall 

gedit2_LDADD = \
	dialogs/libdialogs.a	\
	$(GEDIT_LIBS)

gedit2_SOURCES = 		\
	gedit2.c		\
	gedit2.h		\
	gedit-mdi.c		\
	gedit-mdi.h		\
	gedit-mdi-child.c	\
	gedit-mdi-child.h	\
	gedit-document.c 	\
	gedit-document.h 	\
	gedit-view.c 		\
	gedit-view.h 		\
	gedit-menus.h		\
	gedit-menus.c		\
	gedit-debug.c 		\
	gedit-debug.h 		\
	gedit-prefs.c 		\
	gedit-prefs.h 		\
	gedit-utils.c 		\
	gedit-utils.h 		\
	gedit-commands.h	\
	gedit-commands.c	\
	gedit-file.h		\
	gedit-file.c		\
	gedit-recent.h		\
	gedit-recent.c		\
	gedit-undo-manager.h	\
	gedit-undo-manager.c	\
	bonobo-mdi-child.c	\
	bonobo-mdi-child.h	\
	bonobo-mdi.c		\
	bonobo-mdi.h		\
	gnome-vfs-helpers.c	\
	gnome-vfs-helpers.h

# FIXME
EXTRA_DIST = gedit-ui.xml	


