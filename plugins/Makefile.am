## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)

plugindir = $(prefix)/share/go/plugins

SUBDIRS = cvsdiff diff email encrypt reverse spell prm
