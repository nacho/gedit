gladedir = $(datadir)/gedit/glade/

INCLUDES = 						\
	-I$(top_srcdir)/src				\
	-I$(includedir)/src				\
	$(GNOME_INCLUDEDIR)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGEDIT_GLADEDIR=\""$(gladedir)"\"		\
	$(EXTRA_GNOME_CFLAGS)

noinst_LIBRARIES = libdialogs.a

libdialogs_a_SOURCES = 		\
	dialog-about.c		\
	dialog-goto-line.c	\
	dialog-prefs.c		\
	dialog-plugins.c	\
	dialog-replace.c	\
	dialog-uri.c		\
	dialogs.h		\
	gtkchecklist.c		\
	gtkchecklist.h

glade_DATA =			\
	about.glade		\
	goto-line.glade 	\
	prefs.glade		\
	replace.glade		\
	program.glade		\
	plugin-manager.glade	\
	uri.glade

EXTRA_DIST = $(glade_DATA)
