## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)				\
	$(top_srcdir)/plugins/

plugindir = $(libdir)/go/plugins

plugin_PROGRAMS = encrypt_plugin

encrypt_plugin_SOURCES =	\
	encrypt.c		\
	$(top_srcdir)/plugins/client.c	\
	$(top_srcdir)/plugins/client.h

encrypt_plugin_LDFLAGS = `gtk-config --libs`

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic
#	-DDEBUG
