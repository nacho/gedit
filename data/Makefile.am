desktopdir 	 = $(datadir)/applications
desktop_in_files = gedit.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

keysdir 	= $(datadir)/mime-info
keys_in_files 	= gedit.keys.in
keys_DATA 	= $(keys_in_files:.keys.in=.keys) 
@INTLTOOL_KEYS_RULE@

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gedit.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

bonobodir = $(libdir)/bonobo
serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_Gedit.server.in.in
server_DATA = $(server_in_files:.server.in.in=.server)
$(server_in_files:.server.in.in=.server.in): $(server_in_files)
	sed -e "s|\@BONOBODIR\@|$(bonobodir)|" $< > $@
CLEANFILES = GNOME_Gedit.server.in GNOME_Gedit.server
@INTLTOOL_SERVER_RULE@

man_MANS = gedit.1

app_DATA = gedit.applications
appdir = $(datadir)/application-registry

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gedit-@GEDIT_MAJOR@.pc

gedit-@GEDIT_MAJOR@.pc: gedit.pc
	cp gedit.pc gedit-@GEDIT_MAJOR@.pc

EXTRA_DIST = $(desktop_in_files)	\
	$(keys_in_files)		\
	$(schemas_in_files)		\
	$(server_in_files)		\
	$(man_MANS)			\
	$(app_DATA)			\
	$(desktop_DATA)			\
	$(keys_DATA)			\
	$(schemas_DATA)			\
	$(server_DATA)			\
	gedit.pc.in

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p ; \
	                done \
	        fi
else
install-data-local:
endif

DISTCLEANFILES = 			\
	$(desktop_DATA)			\
	$(keys_DATA)			\
	$(schemas_DATA)			\
	$(server_DATA)			\
	$(pkgconfig_DATA)


