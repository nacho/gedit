## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) -I$(top_srcdir)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	$(GNOME_INCLUDEDIR)

## This need to be two distinct directories

plugindir = $(libexecdir)/go/plugins
pluginlibdir = $(libexecdir)/gedit/plugins

geditincludedir = $(includedir)/gedit

if WITH_GMODULE_PLUGINS
geditinclude_HEADERS = gE_plugin.h
GMODULE_PLUGIN_SRCLIST = gE_plugin.h gE_plugin.c
else
GMODULE_PLUGIN_SRCLIST =
endif

bin_PROGRAMS = gedit

gedit_SOURCES = 	\
	gedit.c		\
	commands.c	\
	commands.h	\
	gE_document.c	\
	gE_document.h   \
	gE_about.c	\
	gE_about.h      \
	gE_files.c	\
	gE_files.h      \
	gE_prefs.c	\
	gE_prefslib.c	\
	gE_prefslib.h	\
	gE_prefs_box.c	\
	gE_prefs_box.h	\
	toolbar.c	\
	plugin.c        \
	gE_plugin_api.c	\
	gE_print.c \
	gE_print.h \
	gE_prefs.h	\
	main.h		\
	toolbar.h	\
	plugin.h        \
	gE_plugin_api.h	\
	menus.h		\
	menus.c		\
	msgbox.h	\
	msgbox.c	\
	gtkscrollball.h	\
	gtkscrollball.c	\
	search.h	\
	search.c	\
	dialog.h	\
	dialog.c	\
	$(GMODULE_PLUGIN_SRCLIST)

#gedit_LDADD = -lm -lgtk -lgdk -lglib -lXext -lX11 -L/usr/X11R6/lib -L/usr/local/lib

gedit_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

EXTRA_DIST = gE-icon-1.xpm gE-icon-2.xpm gE_icon.xpm

CPPFLAGS = \
	-DBINDIR=\""$(bindir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-DPLUGINLIBDIR=\""$(pluginlibdir)"\" \
	-Wall -Wno-unused -ansi
#	-DDEBUG

