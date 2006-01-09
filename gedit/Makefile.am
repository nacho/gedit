## Process this file with automake to produce Makefile.in
SUBDIRS = dialogs recent-files

bin_PROGRAMS = gedit

INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(srcdir)						\
	$(GEDIT_CFLAGS)						\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\"		\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"
	
if ENABLE_PYTHON
INCLUDES += \
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)
endif

gedit_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

gedit_LDADD =				\
	$(GEDIT_LIBS)			\
	dialogs/libdialogs.la		\
	recent-files/librecent.la

if ENABLE_PYTHON
gedit_LDADD += \
	$(top_builddir)/bindings/python/gedit.la
endif

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h              

NOINST_H_FILES =			\
	gedit-commands.h		\
	gedit-tooltips.h		\
	gedit-languages-manager.h	\
	gedit-plugin-manager.h		\
	gedit-plugins-engine.h		\
	gedit-module.h			\
	gedit-ui.h			\
	gedit-window-private.h		\
	gedit-documents-panel.h		\
	gedit-document-loader.h		\
	gedit-document-saver.h		\
	gedit-io-error-message-area.h	\
	gedit-spinner.h			\
	sexy-icon-entry.h		\
	gedittextregion.h		\
	gedit-session.h

if ENABLE_PYTHON
NOINST_H_FILES += \
	gedit-python-module.h		\
	gedit-python-plugin.h 
endif

INST_H_FILES =				\
	gedit-app.h			\
	gedit-convert.h			\
	gedit-debug.h			\
	gedit-document.h 		\
	gedit-encodings.h		\
	gedit-encodings-option-menu.h	\
	gedit-file-chooser-dialog.h	\
	gedit-help.h 			\
	gedit-message-area.h		\
	gedit-metadata-manager.h	\
	gedit-notebook.h		\
	gedit-panel.h			\
	gedit-plugin.h			\
	gedit-prefs-manager-app.h	\
	gedit-prefs-manager.h		\
	gedit-print.h			\
	gedit-print-job-preview.h	\
	gedit-progress-message-area.h	\
	gedit-recent.h			\
	gedit-statusbar.h		\
	gedit-tab.h 			\
	gedit-utils.h 			\
	gedit-view.h 			\
	gedit-window.h

headerdir = $(prefix)/include/gedit-@GEDIT_MAJOR@/gedit

header_DATA = 				\
	$(INST_H_FILES)	

gedit_SOURCES = 			\
	$(BUILT_SOURCES)		\
	$(BACON_FILES)			\
	gedit.c				\
	gedit-app.c			\
	gedit-commands-documents.c	\
	gedit-commands-edit.c		\
	gedit-commands-file.c		\
	gedit-commands-file-print.c	\
	gedit-commands-help.c		\
	gedit-commands-search.c		\
	gedit-commands-view.c		\
	gedit-convert.c			\
	gedit-debug.c			\
	gedit-document.c 		\
	gedit-document-loader.c		\
	gedit-document-saver.c		\
	gedit-documents-panel.c		\
	gedit-encodings.c		\
	gedit-encodings-option-menu.c	\
	gedit-file-chooser-dialog.c	\
	gedit-help.c			\
	gedit-io-error-message-area.c	\
	gedit-languages-manager.c	\
	gedit-message-area.c		\
	gedit-metadata-manager.c	\
	gedit-module.c			\
	gedit-notebook.c		\
	gedit-panel.c			\
	gedit-plugin.c			\
	gedit-plugin-manager.c		\
	gedit-plugins-engine.c		\
	gedit-prefs-manager-app.c	\
	gedit-prefs-manager.c		\
	gedit-prefs-manager-private.h	\
	gedit-print.c			\
	gedit-print-job-preview.c	\
	gedit-progress-message-area.c	\
	gedit-recent.c			\
	gedit-session.c			\
	gedit-spinner.c			\
	gedit-statusbar.c		\
	gedit-tab.c 			\
	gedit-tooltips.c		\
	gedit-utils.c 			\
	gedit-view.c 			\
	gedit-window.c			\
	sexy-icon-entry.c		\
	gedittextregion.c		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)


if ENABLE_PYTHON
gedit_SOURCES += \
	gedit-python-module.c		\
	gedit-python-module.h		\
	gedit-python-plugin.c		\
	gedit-python-plugin.h	
endif

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

ui_DATA = gedit-ui.xml
uidir = $(datadir)/gedit-2/ui/

EXTRA_DIST = 				\
	$(ui_DATA)			\
	gedit-marshal.list

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor

BACON_DIR=$(srcdir)/../../libbacon/src/
BACON_FILES=bacon-message-connection.h bacon-message-connection.c

regenerate-built-sources:
	BACONFILES="$(BACON_FILES)" BACONDIR="$(BACON_DIR)" $(top_srcdir)/gedit/update-from-bacon.sh
