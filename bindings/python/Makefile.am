## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = \
	gedit.la

nodist_gedit_la_SOURCES = \
	gedit.c			\
	geditutils.c

gedit_la_LDFLAGS = \
	-module -avoid-version

gedit_la_LIBADD = \
	$(PYTHON_LIB_LOC)       \
	$(PYTHON_LIBS)		\
	$(PYTHON_EXTRA_LIBS)	\
	$(PYGTK_LIBS)

gedit_la_CFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)/gedit		\
	-I$(top_builddir)/gedit		\
	$(GEDIT_CFLAGS)			\
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)

$(top_builddir)/gedit/gedit-enum-types.h:
	cd $(top_builddir)/gedit && make gedit-enum-types.h
	
gedit.c: gedit.defs gedit.override $(top_builddir)/gedit/gedit-enum-types.h
	( cd $(srcdir) && $(PYGTK_CODEGEN) \
		--register $(PYGTK_DEFSDIR)/pango-types.defs \
		--register $(PYGTK_DEFSDIR)/gdk-types.defs \
		--register $(PYGTK_DEFSDIR)/gtk-types.defs \
		--register $(PYGTK_DEFSDIR)/gtksourceview.defs \
		--override $*.override \
		--prefix py$* $(<F) ) > $@ 

geditutils.c: geditutils.defs geditutils.override
	( cd $(srcdir) && $(PYGTK_CODEGEN) \
		--override $*.override \
		--prefix py$* $(<F) ) > $@ 

BINDING_GEDIT_HEADERS_SRCDIR_IN = \
	gedit/gedit-app.h		\
	gedit/gedit-document.h		\
	gedit/gedit-encodings.h		\
	gedit/gedit-plugin.h		\
	gedit/gedit-view.h		\
	gedit/gedit-tab.h 		\
	gedit/gedit-panel.h 		\
	gedit/gedit-window.h 		\
	gedit/gedit-help.h

BINDING_UTILS_HEADERS_SRCDIR_IN = \
	gedit/gedit-utils.h

BINDING_HEADERS_BUILDDIR_IN = 

BINDING_GEDIT_HEADERS_SRCDIR	:= $(addprefix $(top_srcdir)/,$(BINDING_GEDIT_HEADERS_SRCDIR_IN))
BINDING_UTILS_HEADERS_SRCDIR	:= $(addprefix $(top_srcdir)/,$(BINDING_UTILS_HEADERS_SRCDIR_IN))
BINDING_HEADERS_BUILDDIR	:= $(addprefix $(top_builddir)/,$(BINDING_HEADERS_BUILDDIR_IN))

regenerate-python-binding:
	$(PYGTK_H2DEF) $(sort $(BINDING_GEDIT_HEADERS_SRCDIR) $(BINDING_HEADERS_BUILDDIR)) > gedit.defs.new
	$(PYGTK_H2DEF) $(sort $(BINDING_UTILS_HEADERS_SRCDIR) $(BINDING_HEADERS_BUILDDIR)) > geditutils.defs.new

BUILT_SOURCES = \
	gedit.c			\
	geditutils.c

EXTRA_DIST = \
	gedit.override		\
	gedit.defs		\
	geditutils.override	\
	geditutils.defs

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
