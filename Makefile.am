## Process this file with automake to produce Makefile.in

SUBDIRS = macros po intl pixmaps plugins src help

EXTRA_DIST = BUGS FAQ ChangeLog README README.plugins \
	INSTALL gedit.1 gedit.desktop.in \
	ABOUT-NLS TODO gedit.spec.in gedit.keys.in app-docs.make \
	intltool-extract.in intltool-merge.in intltool-update.in

man_MANS = gedit.1

sysdir   = $(datadir)/gnome/apps/Applications
sys_in_files = gedit.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

mimedir = $(datadir)/mime-info
mime_in_files = gedit.keys.in
mime_DATA = $(mime_in_files:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@

dist-hook: gedit.spec
	cp gedit.spec $(distdir)

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
