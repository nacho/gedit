## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)

plugindir = $(prefix)/share/go/plugins

plugin_PROGRAMS = cvsdiff-plugin

cvsdiff_plugin_SOURCES=	\
	cvsdiff.c	\
	client.c	\
	client.h

cvsdiff_plugin_LDFLAGS = `gtk-config --libs`

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	#-DDEBUG
