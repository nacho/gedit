pluginlibdir = $(libexecdir)/gedit/plugins

pluginlib_LTLIBRARIES = libshell_plugin.la

plugin_FILES = shell.plugin

EXTRA_DIST = shell.plugin.in

pluginlib_DATA = $(plugin_FILES)

CLEANFILES = $(plugin_FILES)

INCLUDES = 						\
	-I$(includedir)					\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DLOCALEDIR=\"$(datadir)/locale\"		\
	$(GNOME_INCLUDEDIR)

CPPFLAGS = \
	-DBINDIR=\""$(bindir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DPLUGINLIBDIR=\""$(pluginlibdir)"\"

libshell_plugin_la_SOURCES = \
	main.c

libshell_plugin_la_LIBADD = \
	@GNOME_LIBDIR@ @GNOMEUI_LIBS@

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

