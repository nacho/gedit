desktopdir 	 = $(datadir)/applications
desktop_in_files = gedit.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

keysdir 	= $(datadir)/mime-info
keys_in_files 	= gedit.keys.in
keys_DATA 	= $(keys_in_files:.keys.in=.keys) 
@INTLTOOL_KEYS_RULE@

schemasdir	 = $(sysconfdir)/gconf/schemas
schemas_in_files = gedit.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_Gedit.server.in
server_DATA = $(server_in_files:.server.in=.server)
@INTLTOOL_SERVER_RULE@

man_MANS = gedit.1

app_DATA = gedit.applications
appdir = $(datadir)/application-registry

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gedit-@GEDIT_MAJOR@.pc

gedit-@GEDIT_MAJOR@.pc: gedit.pc
	cp gedit.pc gedit-@GEDIT_MAJOR@.pc

EXTRA_DIST = $(desktop_in_files)	\
	$(desktop_DATA)			\
	$(keys_in_files)		\
	$(keys_DATA)			\
	$(schemas_in_files)		\
	$(schemas_DATA)			\
	$(server_in_files)		\
	$(server_DATA)			\
	$(man_MANS)			\
	$(app_DATA)			\
	gedit.pc.in
	
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p ; \
	                done \
	        fi

