## Process this file with automake to produce Makefile.in

INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/gedit					\
	$(GEDIT_CFLAGS)						\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\"		\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-DGTK_DISABLE_DEPRECATED				\
	-DGDK_PIXBUF_DISABLE_DEPRECATED				\
	-DGNOME_DISABLE_DEPRECATED

lib_LTLIBRARIES = libgedit-control.la

libgedit_control_la_SOURCES = \
	gedit-factory.c \
	gedit-viewer.c \
	gedit-persist-stream.c

libgedit_control_la_LIBADD = \
	../gedit/libgedit.la \
	$(GEDIT_LIBS)

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = gedit-viewer-ui.xml

EXTRA_DIST = $(ui_DATA)
