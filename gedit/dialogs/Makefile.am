gladedir = $(datadir)/gedit2/glade/

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit2/glade/"\"
	

noinst_LIBRARIES = libdialogs.a

libdialogs_a_SOURCES = 			\
	gedit-dialog-uri.c		\
	gedit-dialog-goto-line.c	\
	gedit-dialog-replace.c		\
	gedit-preferences-dialog.h	\
	gedit-preferences-dialog.c	\
	gedit-dialogs.h		

glade_DATA =			\
	uri.glade2		\
	goto-line.glade2	\
	replace.glade2		\
	gedit-preferences.glade2

EXTRA_DIST = $(glade_DATA) 

