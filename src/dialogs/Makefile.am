gladedir = $(datadir)/gedit/glade

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
	dialog-search.c

glade_DATA =		\
	find-line.glade \
	prefs.glade	\
	search.glade	\
	replace.glade

EXTRA_DIST = $(glade_DATA)



