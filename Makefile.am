## Process this file with automake to produce Makefile.in

SUBDIRS = src pixmaps po plugins

GEDIT_SCHEMA = gedit.schemas

EXTRA_DIST = BUGS FAQ ChangeLog README README.plugins \
	gedit.1 gedit2.desktop.in \
	TODO gedit.keys.in app-docs.make \
	intltool-extract.in intltool-merge.in intltool-update.in \
	$(GEDIT_SCHEMA)

#sysdir   = $(datadir)/gnome/apps/Applications
#sys_in_files = gedit2.desktop.in
#sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

#mimedir = $(datadir)/mime-info
#mime_in_files = gedit.keys.in
#mime_DATA = $(mime_in_files:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = 

schemadir = $(sysconfdir)/gconf/schemas
schema_DATA = gedit.schemas

install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schema_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p ; \
	                done \
	        fi
