## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)

EXTRA_DIST = client.c client.h

plugindir = $(libdir)/go/plugins

if HAVE_GNOME
spell = spell
endif

SUBDIRS = cvsdiff diff email encrypt reverse $(spell) prm convert
