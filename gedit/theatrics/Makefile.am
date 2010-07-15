INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/gedit					\
	-I$(top_builddir)/gedit					\
	$(GEDIT_CFLAGS)						\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)

noinst_LTLIBRARIES = libtheatrics.la

BUILT_SOURCES =					\
	gedit-theatrics-enum-types.h		\
	gedit-theatrics-enum-types.c		\
	gedit-theatrics-marshal.h		\
	gedit-theatrics-marshal.c

libtheatrics_la_SOURCES =				\
	gedit-rounded-frame.h				\
	gedit-rounded-frame.c				\
	gedit-view-container.h				\
	gedit-view-container.c				\
	gedit-theatrics-actor.h				\
	gedit-theatrics-actor.c				\
	gedit-theatrics-animated-widget.h		\
	gedit-theatrics-animated-widget.c		\
	gedit-theatrics-choreographer.h			\
	gedit-theatrics-choreographer.c			\
	gedit-theatrics-stage.h				\
	gedit-theatrics-stage.c				\
	gedit-theatrics-utils.h				\
	gedit-theatrics-utils.c				\
	$(BUILT_SOURCES)

ENUM_TYPES =					\
	gedit-theatrics-choreographer.h

gedit-theatrics-enum-types.h: gedit-theatrics-enum-types.h.template $(ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gedit-theatrics-enum-types.h.template $(ENUM_TYPES)) > $@

gedit-theatrics-enum-types.c: gedit-theatrics-enum-types.c.template $(ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gedit-theatrics-enum-types.c.template $(ENUM_TYPES)) > $@

gedit-theatrics-marshal.h: gedit-theatrics-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=gedit_theatrics_marshal > $@

gedit-theatrics-marshal.c: gedit-theatrics-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) echo "#include \"gedit-theatrics-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_theatrics_marshal >> $@

noinst_PROGRAMS = test-rounded-frame test-view-container
progs_ldadd     = $(top_builddir)/gedit/theatrics/libtheatrics.la $(GEDIT_LIBS)

test_rounded_frame_SOURCES = test-rounded-frame.c
test_rounded_frame_LDADD = $(progs_ldadd)

test_view_container_SOURCES = test-view-container.c
test_view_container_LDADD = $(progs_ldadd)

EXTRA_DIST =				\
	gedit-theatrics-marshal.list

CLEANFILES = $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
