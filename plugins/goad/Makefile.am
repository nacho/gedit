corbaserversdir = $(sysconfdir)/CORBA/servers

goad_FILES = gedit.goad

EXTRA_DIST = gedit.goad.in

corbaservers_DATA = $(goad_FILES)

CLEANFILES = $(goad_FILES)

## We create the goad files here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).

%.goad:%.goad.in
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's,\@bindir\@,$(bindir),g' \
	    -e 's,\@plugindir\@,$(libexecdir)/go/plugins,g' \
	    -e 's,\@pluginlibdir\@,$(libexecdir)/gedit/plugins,g' \
	      < $< > tmp  && mv tmp $@

