## Process this file with automake to produce Makefile.in

SUBDIRS = src pixmaps po plugins

GEDIT_SCHEMA = gedit.schemas

desktopdir  = $(datadir)/applications
desktop_in_files = gedit.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = BUGS FAQ ChangeLog README README.plugins \
	gedit.1 \
	$(desktop_in_files) \
	$(desktop_DATA) \
	TODO gedit.keys.in app-docs.make \
	intltool-extract.in intltool-merge.in intltool-update.in \
	$(GEDIT_SCHEMA)


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
