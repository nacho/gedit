uidir = $(datadir)/gedit/ui/

INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/gedit 					\
	-I$(top_builddir)/gedit					\
	$(GEDIT_CFLAGS) 					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)

noinst_LTLIBRARIES = libdialogs.la

libdialogs_la_SOURCES = 			\
	gedit-preferences-dialog.h		\
	gedit-preferences-dialog.c		\
	gedit-close-confirmation-dialog.c 	\
	gedit-close-confirmation-dialog.h 	\
	gedit-encodings-dialog.c		\
	gedit-encodings-dialog.h		\
	gedit-replace-dialog.h			\
	gedit-replace-dialog.c

ui_DATA =					\
	gedit-encodings-dialog.ui		\
	gedit-preferences-dialog.ui		\
	gedit-replace-dialog.ui

EXTRA_DIST = $(ui_DATA) 

-include $(top_srcdir)/git.mk
