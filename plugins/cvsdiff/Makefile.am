## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../						\
	$(GNOME_INCLUDEDIR)				\
	$(top_srcdir)/plugins/

plugindir = $(libdir)/go/plugins

plugin_PROGRAMS = cvsdiff-plugin

cvsdiff_plugin_SOURCES=	\
	cvsdiff.c	\
	$(top_srcdir)/plugins/client.c	\
	$(top_srcdir)/plugins/client.h

cvsdiff_plugin_LDFLAGS = `gtk-config --libs`

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic 
#	-DDEBUG
