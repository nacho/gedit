## Process this file with automake to produce Makefile.in

SUBDIRS = macros po intl pixmaps plugins src help

EXTRA_DIST = BUGS FAQ ChangeLog README README.plugins \
	INSTALL gedit.1 gedit.desktop \
	ABOUT-NLS TODO gedit.spec.in gedit.keys app-docs.make

man_MANS = gedit.1

sysdir   = $(datadir)/gnome/apps/Applications
sys_DATA = gedit.desktop

mimedir = $(datadir)/mime-info
mime_DATA = gedit.keys

dist-hook: gedit.spec
	cp gedit.spec $(distdir)

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
