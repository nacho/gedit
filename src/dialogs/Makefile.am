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
	dialogs.h

glade_DATA =			\
	about.glade		\
	goto-line.glade 	\
	prefs.glade		\
	replace.glade		\
	program.glade		\
	plugin-manager.glade	

glade_msgs =			\
	about.glade.h		\
	goto-line.glade.h	\
	prefs.glade.h		\
	replace.glade.h		\
	program.glade.h		\
	plugin-manager.glade.h	

EXTRA_DIST = $(glade_DATA) $(glade_msgs)
