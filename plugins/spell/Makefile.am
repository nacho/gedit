## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)				\
	$(top_srcdir)/plugins/

plugindir = $(libdir)/go/plugins

plugin_PROGRAMS = spell-plugin

spell_plugin_SOURCES =	\
	spell.c		\
	$(top_srcdir)/plugins/client.c	\
	$(top_srcdir)/plugins/client.h

spell_plugin_LDFLAGS = `gtk-config --libs`
spell_plugin_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic
#	-DDEBUG
