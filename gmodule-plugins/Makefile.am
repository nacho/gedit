## Process this file with automake to produce Makefile.in

INCLUDES = -I$(includedir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I../	\
	$(GNOME_INCLUDEDIR)

plugindir = $(prefix)/share/go/plugins


plugin_PROGRAMS = diff-plugin cvsdiff-plugin reverse-plugin hello_plugin encrypt_plugin decrypt_plugin  #email-plugin

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

encrypt_plugin_SOURCES =  \
	encrypt.c		\
	client.c	\
	client.h	
	
encrypt_plugin_LDFLAGS = `gtk-config --libs`

decrypt_plugin_SOURCES =  \
	decrypt.c		\
	client.c	\
	client.h	
	
decrypt_plugin_LDFLAGS = `gtk-config --libs`

#email_plugin_SOURCES =	\
#	email.c		\
#	client.c	\
#	client.h

#email_plugin_LDFLAGS = `gtk-config --libs`

CPPFLAGS = \
        -DBINDIR=\""$(bindir)"\" \
        -DDATADIR=\""$(datadir)"\" \
	-DPLUGINDIR=\""$(prefix)/share/go/plugins"\" \
	-Wall -ansi -pedantic  \
	#-DDEBUG
