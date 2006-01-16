# Python snippets plugin

plugin_PYTHON =		\
	__init__.py	\
	capture.py	\
	ElementTree.py	\
	functions.py	\
	manager.py	\
	outputpanel.py

plugin_DATA = tools.glade
plugindir = $(libdir)/gedit-2/plugins/externaltools

tools_DATA = stock-tools.xml
toolsdir = $(datadir)/gedit-2/plugins/externaltools

EXTRA_DIST = $(plugin_DATA) $(tools_DATA)

CLEANFILES = *.bak *.gladep
DISTCLEANFILES = *.bak *.gladep
