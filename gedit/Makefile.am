## Process this file with automake to produce Makefile.in
SUBDIRS = dialogs smclient

bin_PROGRAMS = gedit

noinst_LTLIBRARIES = libgedit.la

INCLUDES =								\
	-I$(top_srcdir)							\
	-I$(srcdir)							\
	-I$(srcdir)/smclient						\
	$(GEDIT_CFLAGS)							\
	$(IGE_MAC_CFLAGS)						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)					\
	$(INTROSPECTION_CFLAGS)						\
	-DDATADIR=\""$(datadir)"\"					\
	-DLIBDIR=\""$(libdir)"\"

gedit_SOURCES = \
	gedit.c

gedit_LDADD =			\
	libgedit.la		\
	$(GEDIT_LIBS)		\
	$(IGE_MAC_LIBS)		\
	$(EGG_SMCLIENT_LIBS)	\
	$(INTROSPECTION_LIBS)

if PLATFORM_WIN32
gedit_LDFLAGS = -Wl,--export-all-symbols -Wl,--out-implib,libgedit-$(GEDIT_API_VERSION).a
else
gedit_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"
endif

libgedit_la_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

libgedit_la_LIBADD = \
	dialogs/libdialogs.la		\
	smclient/libeggsmclient.la

# GEDIT_LIBS must be the last to ensure correct order on some platforms
libgedit_la_LIBADD += $(GEDIT_LIBS)

# Mac OSX convenience library and ldflags
if OS_OSX
gedit_LDFLAGS += -framework Carbon

noinst_LTLIBRARIES += libosx.la

libosx_la_LDFLAGS = -framework Carbon -framework ApplicationServices -framework Cocoa
libosx_la_LIBADD = -lobjc
libosx_la_CFLAGS = -xobjective-c

libosx_la_SOURCES = 		\
	gedit-app-osx.c		\
	gedit-app-osx.h		\
	gedit-osx-delegate.m 	\
	gedit-osx-delegate.h

libgedit_la_LIBADD += libosx.la
endif

# Win32 convenience library and ldflags
if OS_WIN32
gedit_LDFLAGS += -mwindows

noinst_LTLIBRARIES += libwin32.la

libwin32_la_SOURCES =		\
	gedit-app-win32.c	\
	gedit-app-win32.h

libgedit_la_LIBADD += libwin32.la

gedit-res.o: gedit.rc
	$(WINDRES) -i gedit.rc --input-format=rc -o gedit-res.o -O coff

gedit_LDADD += gedit-res.o
endif

# X11 convenience library
if GDK_WINDOWING_X11
noinst_LTLIBRARIES += libx11.la

libx11_la_SOURCES =		\
	gedit-app-x11.c		\
	gedit-app-x11.h

libgedit_la_LIBADD += libx11.la
endif

BUILT_SOURCES = 			\
	gedit-enum-types.c		\
	gedit-enum-types.h		\
	gedit-marshal.c			\
	gedit-marshal.h

NOINST_H_FILES =			\
	gedit-close-button.h		\
	gedit-command-line.h		\
	gedit-dbus.h			\
	gedit-dirs.h			\
	gedit-document-input-stream.h	\
	gedit-document-loader.h		\
	gedit-document-output-stream.h	\
	gedit-document-saver.h		\
	gedit-documents-panel.h		\
	gedit-history-entry.h		\
	gedit-io-error-info-bar.h	\
	gedit-language-manager.h	\
	gedit-multi-notebook.h		\
	gedit-notebook.h		\
	gedit-object-module.h		\
	gedit-plugin-info.h		\
	gedit-plugin-info-priv.h	\
	gedit-plugin-loader.h		\
	gedit-plugin-manager.h		\
	gedit-plugins-engine.h		\
	gedit-print-job.h		\
	gedit-print-preview.h		\
	gedit-session.h			\
	gedit-settings.h		\
	gedit-smart-charset-converter.h	\
	gedit-style-scheme-manager.h	\
	gedit-tab-label.h		\
	gedittextregion.h		\
	gedit-ui.h			\
	gedit-window-private.h

INST_H_FILES =				\
	gedit-app.h			\
	gedit-commands.h		\
	gedit-debug.h			\
	gedit-document.h 		\
	gedit-encodings.h		\
	gedit-encodings-combo-box.h	\
	gedit-file-chooser-dialog.h	\
	gedit-message-bus.h		\
	gedit-message-type.h		\
	gedit-message.h			\
	gedit-panel.h			\
	gedit-plugin.h			\
	gedit-progress-info-bar.h	\
	gedit-statusbar.h		\
	gedit-status-combo-box.h	\
	gedit-tab.h 			\
	gedit-utils.h 			\
	gedit-view.h 			\
	gedit-window.h

if !ENABLE_GVFS_METADATA
INST_H_FILES += gedit-metadata-manager.h
endif

headerdir = $(prefix)/include/gedit-@GEDIT_API_VERSION@/gedit

header_DATA = 				\
	$(INST_H_FILES)

libgedit_c_files =			\
	gedit-app.c			\
	gedit-close-button.c		\
	gedit-command-line.c		\
	gedit-commands-documents.c	\
	gedit-commands-edit.c		\
	gedit-commands-file.c		\
	gedit-commands-file-print.c	\
	gedit-commands-help.c		\
	gedit-commands-search.c		\
	gedit-commands-view.c		\
	gedit-dbus.c			\
	gedit-debug.c			\
	gedit-dirs.c			\
	gedit-document.c 		\
	gedit-document-input-stream.c	\
	gedit-document-loader.c		\
	gedit-document-output-stream.c	\
	gedit-document-saver.c		\
	gedit-documents-panel.c		\
	gedit-encodings.c		\
	gedit-encodings-combo-box.c	\
	gedit-file-chooser-dialog.c	\
	gedit-history-entry.c		\
	gedit-io-error-info-bar.c	\
	gedit-language-manager.c	\
	gedit-message-bus.c		\
	gedit-message-type.c		\
	gedit-message.c			\
	gedit-multi-notebook.c		\
	gedit-object-module.c		\
	gedit-notebook.c		\
	gedit-panel.c			\
	gedit-plugin-info.c		\
	gedit-plugin.c			\
	gedit-plugin-loader.c		\
	gedit-plugin-manager.c		\
	gedit-plugins-engine.c		\
	gedit-print-job.c		\
	gedit-print-preview.c		\
	gedit-progress-info-bar.c	\
	gedit-session.c			\
	gedit-settings.c		\
	gedit-smart-charset-converter.c	\
	gedit-statusbar.c		\
	gedit-status-combo-box.c	\
	gedit-style-scheme-manager.c	\
	gedit-tab.c 			\
	gedit-tab-label.c		\
	gedit-utils.c 			\
	gedit-view.c 			\
	gedit-window.c			\
	gedittextregion.c

libgedit_la_SOURCES = 			\
	$(BUILT_SOURCES)		\
	$(libgedit_c_files)		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

if !ENABLE_GVFS_METADATA
libgedit_la_SOURCES += gedit-metadata-manager.c
endif

if !PLATFORM_WIN32
libgedit_la_SOURCES += gedit-fifo.c
NOINST_H_FILES += gedit-fifo.h
endif

ENUM_TYPES = 			\
	gedit-notebook.h	\
	$(INST_H_FILES)

gedit-enum-types.h: gedit-enum-types.h.template $(ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gedit-enum-types.h.template $(ENUM_TYPES)) > $@

gedit-enum-types.c: gedit-enum-types.c.template $(ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gedit-enum-types.c.template $(ENUM_TYPES)) > $@

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

uidir = $(datadir)/gedit-2/ui/
ui_DATA = 				\
	gedit-ui.xml			\
	gedit-print-preferences.ui

EXTRA_DIST = 				\
	$(ui_DATA)			\
	gedit-enum-types.h.template	\
	gedit-enum-types.c.template	\
	gedit-marshal.list		\
	gedit.rc

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Gedit-3.0.gir

Gedit-3.0.gir: libgedit.la
INTROSPECTION_SCANNER_ARGS = -I$(top_srcdir) --warn-all
Gedit_3_0_gir_NAMESPACE = Gedit
Gedit_3_0_gir_VERSION = 3.0
Gedit_3_0_gir_PROGRAM = $(builddir)/gedit
Gedit_3_0_gir_FILES = $(INST_H_FILES) $(libgedit_c_files)
Gedit_3_0_gir_INCLUDES = Gtk-3.0 GtkSource-3.0

girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(INTROSPECTION_TYPELIBDIR)
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES +=			\
	$(gir_DATA)	\
	$(typelib_DATA)
endif

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

install-exec-hook:
if PLATFORM_WIN32
	$(mkinstalldirs) "$(DESTDIR)$(libdir)"
	$(INSTALL_DATA) libgedit-$(GEDIT_API_VERSION).a "$(DESTDIR)$(libdir)"
else
	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor
endif

-include $(top_srcdir)/git.mk
