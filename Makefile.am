## Process this file with automake to produce Makefile.in

plugindir = $(libdir)/go/plugins

SUBDIRS = macros po intl src plugins

EXTRA_DIST = KNOWNBUGS ChangeLog README README.plugins INSTALL gedit.1 gE-icon-1.xpm gE-icon-2.xpm gedit.desktop ABOUT-NLS TODO gedit.spec

man_MANS = gedit.1

sysdir   = $(datadir)/apps/Applications
sys_DATA = gedit.desktop

dist-hook:
	mkdir $(distdir)/xpm
	cp $(srcdir)/xpm/*.xpm $(distdir)/xpm

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
