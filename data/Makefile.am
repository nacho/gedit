desktopdir 	 = $(datadir)/applications
desktop_in_files = gedit.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

gsettingsschema_in_files = org.gnome.gedit.gschema.in
gsettingsschema_DATA = $(gsettingsschema_in_files:.gschema.in=.gschema.xml)
org.gnome.gedit.gschema.xml: org.gnome.gedit.gschema.in
	gsettings-schema-convert -f $< -o $@

servicedir = $(datadir)/dbus-1/services
service_in_files = org.gnome.gedit.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $<> $@

man_MANS = gedit.1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gedit-@GEDIT_API_VERSION@.pc

gedit-@GEDIT_API_VERSION@.pc: gedit.pc
	cp gedit.pc gedit-@GEDIT_API_VERSION@.pc

bugreportdir = $(libexecdir)/gedit-2
bugreport_SCRIPTS = gedit-bugreport.sh

if GSETTINGS_SCHEMAS_INSTALL
install-data-hook:
	$(GLIB_COMPILE_SCHEMAS) $(gsettingsschemadir)
endif

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(service_in_files)		\
	$(man_MANS)			\
	gedit.pc.in			\
	gedit-bugreport.sh.in		\
	$(gsettingsschema_DATA)

CLEANFILES =	 			\
	$(desktop_DATA)			\
	$(service_DATA)			\
	$(pkgconfig_DATA)




-include $(top_srcdir)/git.mk
