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
	gedit-theatrics-actor.h				\
	gedit-theatrics-actor.c				\
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

EXTRA_DIST =					\
	gedit-theatrics-marshal.list		\
	gedit-theatrics-enum-types.h.template	\
	gedit-theatrics-enum-types.c.template

CLEANFILES = $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
