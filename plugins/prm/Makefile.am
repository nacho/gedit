## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	`gtk-config --cflags` 	\
#	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	


plugindir = $(prefix)/share/go/plugins

plugin_PROGRAMS = prm-plugin

prm_plugin_SOURCES =	\
	maincode.c	\
	maincode.h	\
	msgbox.c	\
	msgbox.h	\
	client.c	\
	client.h

prm_plugin_LDFLAGS = `gtk-config --libs`

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	-DPLUGIN

