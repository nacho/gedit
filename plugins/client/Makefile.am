## Process this file with automake to produce Makefile.in

pluginlibdir = $(libdir)

pluginlib_LIBRARIES = libclient.a

geditincludedir = $(includedir)/gedit

libclient_a_SOURCES = \
	client.c	\
	client.h

geditinclude_HEADERS = \
	client.h

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I..						\
	$(GNOME_INCLUDEDIR)

LDADD = \
	$(GTK_LIBS)

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-Wall -ansi -pedantic 
#	-DDEBUG
