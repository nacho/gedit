# Python console plugin

plugindir = $(GEDIT_PLUGINS_LIBS_DIR)/pythonconsole
plugin_PYTHON =		\
	__init__.py	\
	console.py	\
	config.py

uidir = $(GEDIT_PLUGINS_DATA_DIR)/pythonconsole/ui
ui_DATA = config.ui

gsettings_SCHEMAS = org.gnome.gedit.plugins.pythonconsole.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST =		\
	$(ui_DATA)	\
	org.gnome.gedit.plugins.pythonconsole.gschema.xml.in.in

CLEANFILES = $(gsettings_SCHEMAS)
DISTCLEANFILES = $(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

-include $(top_srcdir)/git.mk
