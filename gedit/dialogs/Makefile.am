gladedir = $(datadir)/gedit/glade/

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GEDIT_CFLAGS) \
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"	

noinst_LIBRARIES = libdialogs.a

libdialogs_a_SOURCES = 			\
	gedit-dialog-uri.c		\
	gedit-dialog-goto-line.c	\
	gedit-dialog-replace.c		\
	gedit-dialogs.h		

glade_DATA =			\
	uri.glade2		\
	goto-line.glade2	\
	replace.glade2

EXTRA_DIST = $(glade_DATA) 

