## Process this file with automake to produce Makefile.in

plugindir = $(libexecdir)/go/plugins

plugin_PROGRAMS = ctag-plugin

ctag_plugin_SOURCES = \
	$(top_srcdir)/gmodule-plugins/client/client.h	\
	ctag.h					\
	ctag.c

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I..						\
	$(GNOME_INCLUDEDIR)				\
	-I$(top_srcdir)/gmodule-plugins/client

LDADD = \
	$(top_builddir)/gmodule-plugins/client/libclient.a	\
	$(GTK_LIBS)

DEPS = \
	$(top_builddir)/gmodule-plugins/client/libclient.a

plugin_PROGRAMS = ctag-plugin

ctag_plugin_DEPENDENCIES = $(DEPS)

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic 
#	-DDEBUG
