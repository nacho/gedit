# Document Info plugin
plugindir = $(libdir)/gedit-2/plugins

INCLUDES = \
	-I$(top_srcdir)/src 				\
	$(GEDIT_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			

plugin_LTLIBRARIES = libdocinfo.la

libdocinfo_la_SOURCES = docinfo.c
libdocinfo_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

gladedir = $(datadir)/gedit-2/glade
glade_DATA = docinfo.glade2

EXTRA_DIST = $(glade_DATA)
