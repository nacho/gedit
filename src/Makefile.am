## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	$(GNOME_INCLUDEDIR)

EXTRA_DIST = KNOWNBUGS ChangeLog README INSTALL gedit.1 gE-icon-1.xpm gE-icon-2.xpm gE_icon.xpm gedit.desktop

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
	gE_prefs.h	\
	main.h		\
	menus.h

#	menus.c		\

#gedit_LDADD = -lm -lgtk -lgdk -lglib -lXext -lX11 -L/usr/X11R6/lib -L/usr/local/lib

gedit_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

sysdir   = $(datadir)/apps/Applications
sys_DATA = gedit.desktop
