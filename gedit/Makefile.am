## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	$(GNOME_INCLUDEDIR)

plugindir = $(prefix)/share/go/plugins

SUBDIRS = plugins

EXTRA_DIST = KNOWNBUGS ChangeLog README README.plugins INSTALL gedit.1 gE_icon.xpm gedit.desktop xpm/*

man_MANS = gedit.1

bin_PROGRAMS = gedit

gedit_SOURCES = 	\
	gedit.c		\
	commands.c	\
	gE_document.c	\
	gE_about.c	\
	gE_files.c	\
	gE_prefs.c	\
	gE_init.c	\
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
	msgbox.c

#gedit_LDADD = -lm -lgtk -lgdk -lglib -lXext -lX11 -L/usr/X11R6/lib -L/usr/local/lib

gedit_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

sysdir   = $(datadir)/apps/Applications
sys_DATA = gedit.desktop

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	#-DDEBUG
