## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) -I$(top_srcdir) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
	$(GNOME_INCLUDEDIR) $(GNOME_PRINT_CFLAGS)

## This need to be two distinct directories
plugindir = $(libexecdir)/go/plugins
pluginlibdir = $(libexecdir)/gedit/plugins
geditincludedir = $(includedir)/gedit

EXTRA_gedit_SOURCES = gE_plugin.h gE_plugin.c

if WITH_GMODULE_PLUGINS
geditinclude_HEADERS = gE_plugin.h
GMODULE_PLUGIN_SRCLIST = gE_plugin.h gE_plugin.c
else
GMODULE_PLUGIN_SRCLIST =
endif

if HAVE_ORBIT
corba_libs = @ORBIT_LIBS@
endif

if HAVE_LIBGNORBA
libgnorba_libs = -lgnorba
endif

bin_PROGRAMS = gedit

gedit_SOURCES = 	\
	gedit.c		\
	commands.c	\
	commands.h	\
	gE_window.c	\
	gE_window.h	\
	gE_view.c	\
	gE_view.h	\
	gE_mdi.c	\
	gE_mdi.h	\
	gedit-undo.c gedit-undo.h	\
	gedit-file-io.c gedit-file-io.h \
	gedit-utils.c gedit-utils.h \
	gE_prefs.c	\
	gE_prefs_box.c	\
	gE_prefs_box.h	\
	gedit-about.c gedit-about.h \
	gedit-print.c gedit-print.h \
	gE_prefs.h	\
	gedit-plugin.c gedit-plugin.h \
	main.h		\
	gedit-menus.c gedit-menus.h \
	gedit-search.c gedit-search.h

gedit_LDADD =	$(GNOME_LIBDIR) 	\
		$(GNOMEUI_LIBS)		\
		$(corba_libs) 		\
		$(libgnorba_libs)	\
		$(INTLLIBS) 		\
		$(GNOME_PRINT_LIBS)

EXTRA_DIST =  

CPPFLAGS = \
	-DBINDIR=\""$(bindir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-DPLUGINLIBDIR=\""$(pluginlibdir)"\" \
	-Wall -Wno-unused -ansi -D_IN_GEDIT

