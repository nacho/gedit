## Process this file with automake to produce Makefile.in

EXTRA_DIST = KNOWNBUGS ChangeLog README INSTALL gedit.1 gE-icon-1.xpm gE-icon-2.xpm

man_MANS = gedit.1

bin_PROGRAMS = gedit
gedit_SOURCES = gedit.c commands.c gE_document.c gE_about.c gE_files.c gE_prefs.c gE_init.c menus.c gE_prefs.h main.h menus.h


gedit_LDADD = -lm -lgtk -lgdk -lglib -lXext -lX11 -L/usr/X11R6/lib -L/usr/local/lib