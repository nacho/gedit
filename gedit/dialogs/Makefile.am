gladedir = $(datadir)/gedit-2/glade/

INCLUDES = \
	-I$(top_srcdir)						\
	-I$(top_srcdir)/gedit 					\
	$(GEDIT_CFLAGS) 					\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-DGTK_DISABLE_DEPRECATED				\
	-DGDK_PIXBUF_DISABLE_DEPRECATED				\
	-DGNOME_DISABLE_DEPRECATED			


noinst_LTLIBRARIES = libdialogs.la

libdialogs_la_SOURCES = 			\
	gedit-dialog-uri.c		\
	gedit-dialog-goto-line.c	\
	gedit-dialog-replace.c		\
	gedit-preferences-dialog.h	\
	gedit-preferences-dialog.c	\
	gedit-plugin-manager.c		\
	gedit-plugin-manager.h	\
	gedit-plugin-program-location-dialog.c \
	gnome-print-font-picker.c	\
	gnome-print-font-picker.h	\
	gnome-print-font-dialog.c	\
	gnome-print-font-dialog.h	\
	gedit-encodings-dialog.c	\
	gedit-encodings-dialog.h	\
	gedit-page-setup-dialog.c	\
	gedit-page-setup-dialog.h	\
	gedit-dialogs.h		

glade_DATA =				\
	uri.glade2			\
	goto-line.glade2		\
	replace.glade2			\
	plugin-manager.glade2		\
	gedit-preferences.glade2	\
	program-location-dialog.glade2	\
	gedit-encodings-dialog.glade2	\
	page-setup-dialog.glade2

EXTRA_DIST = $(glade_DATA) 

