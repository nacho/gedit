desktopdir 	 = $(datadir)/applications
desktop_in_files = gedit.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gedit.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

man_MANS = gedit.1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gedit-@GEDIT_MAJOR@.pc

gedit-@GEDIT_MAJOR@.pc: gedit.pc
	cp gedit.pc gedit-@GEDIT_MAJOR@.pc

bugreportdir      = $(libdir)/gedit-2
bugreport_SCRIPTS = gedit-bugreport.sh

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p ; \
	                done \
	        fi
else
install-data-local:
endif

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(schemas_in_files)		\
	$(man_MANS)			\
	gedit.pc.in			\
	gedit-bugreport.sh.in

CLEANFILES =	 			\
	$(desktop_DATA)			\
	$(schemas_DATA)			\
	$(pkgconfig_DATA)



