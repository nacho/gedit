## Process this file with automake to produce Makefile.in

SUBDIRS = dialogs

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\" 		\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit/plugins"\"	\
	-I$(includedir) 				\
	-I$(top_srcdir) 				\
	-I$(srcdir)/dialogs				\
	-I$(top_srcdir)/intl 				\
	-I$(top_builddir)/intl 				\
	$(GNOME_INCLUDEDIR) 				\
	$(EXTRA_GNOME_CFLAGS)

if HAVE_ORBIT
corba_libs = @ORBIT_LIBS@
endif

if HAVE_LIBGNORBA
libgnorba_libs = -lgnorba
endif

bin_PROGRAMS = gedit

gedit_SOURCES = 		\
	commands.c		\
	commands.h		\
	document.c		\
	document.h		\
	file.c 			\
	file.h		 	\
	gedit.c			\
	menus.c 		\
	menus.h 		\
	plugin.c 		\
	plugin.h 		\
	prefs.c			\
	prefs.h			\
	print.c 		\
	print.h 		\
	recent.c		\
	recent.h		\
	search.c 		\
	search.h		\
	undo.c 			\
	undo.h	 		\
	utils.c 		\
	utils.h 		\
	view.c			\
	view.h			\
	window.c 		\
	window.h


gedit_LDADD = 				\
	dialogs/libdialogs.a		\
	$(GNOME_LIBDIR) 		\
	$(GNOMEUI_LIBS) 		\
	$(corba_libs) 			\
	$(libgnorba_libs) 		\
	$(INTLLIBS) 			\
	$(EXTRA_GNOME_LIBS)

EXTRA_DIST =

