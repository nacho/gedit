gladedir = $(datadir)/gedit-2/glade/

INCLUDES = \
	-I$(top_srcdir)/src 					\
	$(GEDIT_CFLAGS) 					\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-DGTK_DISABLE_DEPRECATED				\
	-DGDK_PIXBUF_DISABLE_DEPRECATED				\
	-DGNOME_DISABLE_DEPRECATED			


noinst_LIBRARIES = libdialogs.a

libdialogs_a_SOURCES = 			\
	gedit-dialog-uri.c		\
	gedit-dialog-goto-line.c	\
	gedit-dialog-replace.c		\
	gedit-preferences-dialog.h	\
	gedit-preferences-dialog.c	\
	gedit-dialog-plugin-manager.c	\
	gedit-plugin-program-location-dialog.c \
	gedit-dialogs.h		

glade_DATA =				\
	uri.glade2			\
	goto-line.glade2		\
	replace.glade2			\
	plugin-manager.glade2		\
	gedit-preferences.glade2	\
	program-location-dialog.glade2

EXTRA_DIST = $(glade_DATA) 

