gladedir = $(datadir)/gedit-2/glade/

INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/gedit 					\
	-I$(top_builddir)/gedit					\
	$(GEDIT_CFLAGS) 					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	

noinst_LTLIBRARIES = libdialogs.la

libdialogs_la_SOURCES = 			\
	gedit-page-setup-dialog.c		\
	gedit-page-setup-dialog.h		\
	gedit-preferences-dialog.h		\
	gedit-preferences-dialog.c		\
	gedit-close-confirmation-dialog.c 	\
	gedit-close-confirmation-dialog.h 	\
	gedit-encodings-dialog.c		\
	gedit-encodings-dialog.h		\
	gedit-open-location-dialog.c		\
	gedit-open-location-dialog.h		\
	gedit-search-dialog.h			\
	gedit-search-dialog.c


glade_DATA =					\
	gedit-encodings-dialog.glade		\
	gedit-open-location-dialog.glade	\
	gedit-page-setup-dialog.glade		\
	gedit-preferences-dialog.glade		\
	gedit-search-dialog.glade



EXTRA_DIST = $(glade_DATA) 

