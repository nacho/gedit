pluginlibdir = $(libexecdir)/gedit/plugins

pluginlib_LTLIBRARIES = liblauncher_plugin.la liblauncher_corba.la

bin_PROGRAMS = gedit-launcher

plugin_FILES = launcher.plugin

EXTRA_DIST = launcher.plugin.in

pluginlib_DATA = $(plugin_FILES)

CLEANFILES = $(plugin_FILES)

INCLUDES = 						\
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	-I$(includedir) $(GNOME_INCLUDEDIR)

CPPFLAGS = \
	-DBINDIR=\""$(bindir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DPLUGINLIBDIR=\""$(pluginlibdir)"\"

CORBA_SRCLIST = launcher-skels.c launcher-stubs.c launcher-common.c launcher.h

BUILT_SOURCES = $(CORBA_SRCLIST)

MAINTAINERCLEANFILES = $(CORBA_SRCLIST)
CLEANFILES = $(CORBA_SRCLIST)

liblauncher_corba_la_SOURCES = $(CORBA_SRCLIST)

gedit_launcher_SOURCES = \
	main.c

gedit_launcher_LDADD = \
	@GNOME_LIBDIR@ -lgnorba @GNOMEUI_LIBS@ @ORBIT_LIBS@ \
	liblauncher_corba.la

liblauncher_plugin_la_SOURCES = \
	launcher-server.c	\
	launcher-impl.c		\

liblauncher_plugin_la_LIBADD = \
	@GNOME_LIBDIR@ -lgnorba @GNOMEUI_LIBS@ @ORBIT_LIBS@ \
	liblauncher_corba.la

$(CORBA_SRCLIST): $(top_srcdir)/plugins/idl/launcher.idl
	orbit-idl $<

## We create the plugin files here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).

%.plugin:%.plugin.in
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's,\@bindir\@,$(bindir),g' \
	    -e 's,\@libdir\@,$(libdir),g' \
	    -e 's,\@plugindir\@,$(libexecdir)/go/plugins,g' \
	    -e 's,\@pluginlibdir\@,$(libexecdir)/gedit/plugins,g' \
	      < $< > tmp  && mv tmp $@

