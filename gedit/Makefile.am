## Process this file with automake to produce Makefile.in

SUBDIRS = dialogs

INCLUDES = \
	-I$(includedir) 				\
	-I$(top_srcdir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\" 		\
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
	gedit.c			\
	commands.c		\
	commands.h		\
	window.c 		\
	window.h 		\
	view.c			\
	view.h			\
	document.c		\
	document.h		\
	undo.c 			\
	undo.h	 		\
	file.c 			\
	file.h		 	\
	utils.c 		\
	utils.h 		\
	prefs.c			\
	prefs.h			\
	about.c 		\
	about.h 		\
	print.c 		\
	print.h 		\
	plugin.c 		\
	plugin.h 		\
	gedit.h			\
	menus.c 		\
	menus.h 		\
	search.c 		\
	search.h

gedit_LDADD = 			\
	dialogs/libdialogs.a	\
	$(GNOME_LIBDIR) 	\
	$(GNOMEUI_LIBS) 	\
	$(corba_libs) 		\
	$(libgnorba_libs) 	\
	$(INTLLIBS) 		\
	$(EXTRA_GNOME_LIBS)

EXTRA_DIST = 