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
	dialog-find-line.c	\
	dialog-prefs.c		\
	dialog-replace.c	\
	dialog-search.c		\
	dialogs.h

glade_DATA =		\
	about.glade	\
	find-line.glade \
	prefs.glade	\
	search.glade	\
	replace.glade

glade_msgs =			\
	about.glade.h		\
	find-line.glade.h	\
	prefs.glade.h		\
	search.glade.h		\
	replace.glade.h

EXTRA_DIST = $(glade_DATA) $(glade_msgs)
