## Process this file with automake to produce Makefile.in

#plugindir = $(libdir)/go/plugins

SUBDIRS = macros po intl src plugins help

EXTRA_DIST = KNOWNBUGS FAQ ChangeLog README README.plugins			\
	INSTALL gedit.1 gE-icon-1.xpm gE-icon-2.xpm gedit.desktop	\
	ABOUT-NLS TODO TODO-road_to_1.0.0 gedit.spec.in gedit.keys

man_MANS = gedit.1

sysdir   = $(datadir)/gnome/apps/Applications
sys_DATA = gedit.desktop

mimedir = $(datadir)/mime-info
mime_DATA = gedit.keys

dist-hook: gedit.spec
	# mkdir $(distdir)/xpm
	# cp $(srcdir)/xpm/*.xpm $(distdir)/xpm
	cp gedit.spec $(distdir)

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
