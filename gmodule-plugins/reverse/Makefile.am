## Process this file with automake to produce Makefile.in

plugindir = $(libexecdir)/go/plugins

plugin_PROGRAMS = reverse-plugin

reverse_plugin_SOURCES = \
	$(top_srcdir)/plugins/client/client.h \
	reverse.c

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I..						\
	$(GNOME_INCLUDEDIR)				\
	-I$(top_srcdir)/plugins/client

LDADD = \
	$(top_builddir)/plugins/client/libclient.a	\
	$(GTK_LIBS)

DEPS = \
	$(top_builddir)/plugins/client/libclient.a

plugin_PROGRAMS = reverse-plugin

reverse_plugin_DEPENDENCIES = $(DEPS)

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic 
#	-DDEBUG
