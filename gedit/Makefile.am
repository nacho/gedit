## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	$(GNOME_INCLUDEDIR)

plugindir = $(prefix)/share/go/plugins

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
	gE_init.c	\
	gE_init.h	\
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
	dialog.c

#gedit_LDADD = -lm -lgtk -lgdk -lglib -lXext -lX11 -L/usr/X11R6/lib -L/usr/local/lib

gedit_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)



CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	#-DDEBUG

