## Process this file with automake to produce Makefile.in

INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(srcdir)						\
	-I$(builddir)						\
	$(GEDIT_CFLAGS)						\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\"		\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"

bonobodir = $(libdir)/bonobo
bonobo_LTLIBRARIES = libgedit-control.la

libgedit_control_la_SOURCES = 					\
	gedit-viewer.h						\
	gedit-factory.c 					\
	gedit-viewer.c 						\
	gedit-persist-stream.h					\
	gedit-persist-stream.c

libgedit_control_la_LIBADD =					\
	../gedit/libgedit.la 					\
	$(GEDIT_LIBS)

libgedit_control_la_LDFLAGS = -module -avoid-version

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = gedit-viewer-ui.xml

EXTRA_DIST = $(ui_DATA) ChangeLog
