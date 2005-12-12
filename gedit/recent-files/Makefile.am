INCLUDES =				\
	$(GEDIT_CFLAGS)			\
	$(WARN_CFLAGS)			\
	$(DISABLE_DEPRECATED_CFLAGS)

headerdir = $(prefix)/include/gedit-@GEDIT_MAJOR@/gedit/recent-files
header_DATA =				\
        egg-recent-model.h		\
        egg-recent-item.h		\
        egg-recent-view.h		\
        egg-recent-view-gtk.h		\
        egg-recent-view-uimanager.h	\
	egg-recent-util.h

EGG_FILES =				\
        egg-recent-model.c		\
        egg-recent-item.c		\
        egg-recent-view.c		\
        egg-recent-view-gtk.c		\
        egg-recent-view-uimanager.c	\
        egg-recent-util.c		\
	$(header_DATA)

noinst_LTLIBRARIES = librecent.la

librecent_la_SOURCES = $(EGG_FILES)

EGGDIR=$(srcdir)/../../../libegg/libegg/recent-files
regenerate-built-sources:
	EGGFILES="$(EGG_FILES)" EGGDIR="$(EGGDIR)" $(srcdir)/update-from-egg.sh

