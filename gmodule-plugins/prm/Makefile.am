## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	`gtk-config --cflags` 	\
#	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../						\
	$(top_srcdir)/plugins/


plugindir = $(libdir)/go/plugins

plugin_PROGRAMS = prm-plugin

prm_plugin_SOURCES =	\
	maincode.c	\
	maincode.h	\
	msgbox.c	\
	msgbox.h	\
	$(top_srcdir)/plugins/client.c	\
	$(top_srcdir)/plugins/client.h

prm_plugin_LDFLAGS = `gtk-config --libs`

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic  \
	-DPLUGIN

