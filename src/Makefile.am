## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	$(GNOME_INCLUDEDIR)

EXTRA_DIST = KNOWNBUGS ChangeLog README INSTALL gedit.1 gE-icon-1.xpm gE-icon-2.xpm gE_icon.xpm gedit.desktop

man_MANS = gedit.1

plugindir = $(prefix)/share/go/plugins

bin_PROGRAMS = gedit

plugin_PROGRAMS = diff-plugin cvsdiff-plugin reverse-plugin hello_plugin #email-plugin

gedit_SOURCES = 	\
	gedit.c		\
	commands.c	\
	gE_document.c	\
	gE_about.c	\
	gE_files.c	\
	gE_prefs.c	\
	gE_init.c	\
	toolbar.c	\
	plugin.c        \
	gE_plugin_api.c	\
	gE_print.c \
	gE_print.h \
	gE_prefs.h	\
	main.h		\
	toolbar.h	\
	plugin.h        \
	gE_plugin_api.h	\
	menus.h		\
	menus.c

#gedit_LDADD = -lm -lgtk -lgdk -lglib -lXext -lX11 -L/usr/X11R6/lib -L/usr/local/lib

gedit_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

diff_plugin_SOURCES =	\
	diff.c		\
	client.c	\
	client.h

diff_plugin_LDFLAGS = `gtk-config --libs`

cvsdiff_plugin_SOURCES=	\
	cvsdiff.c	\
	client.c	\
	client.h

cvsdiff_plugin_LDFLAGS = `gtk-config --libs`

reverse_plugin_SOURCES=	\
	reverse.c	\
	client.c	\
	client.h

reverse_plugin_LDFLAGS = `gtk-config --libs`

# To stop this compiling, add #'s to the following lines...
hello_plugin_SOURCES =  \
	hello.c		\
	client.c	\
	client.h	
	
hello_plugin_LDFLAGS = `gtk-config --libs`
# Stop putting the #'s here if you dont want this plugin


#email_plugin_SOURCES =	\
#	email.c		\
#	client.c	\
#	client.h

#email_plugin_LDFLAGS = `gtk-config --libs`

sysdir   = $(datadir)/apps/Applications
sys_DATA = gedit.desktop

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	#-DDEBUG
