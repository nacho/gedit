## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)

plugindir = $(prefix)/share/go/plugins

plugin_PROGRAMS = spell-plugin

spell_plugin_SOURCES =	\
	spell.c		\
	client.c	\
	client.h

spell_plugin_LDFLAGS = `gtk-config --libs`
spell_plugin_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	#-DDEBUG
