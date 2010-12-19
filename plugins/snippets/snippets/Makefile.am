# Python snippets plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)/snippets

plugin_PYTHON = \
	__init__.py \
	windowactivatable.py \
	document.py \
	library.py \
	snippet.py \
	parser.py \
	placeholder.py \
	manager.py \
	helper.py \
	substitutionparser.py \
	importer.py \
	exporter.py \
	languagemanager.py \
	completion.py

uidir = $(GEDIT_PLUGINS_DATA_DIR)/snippets/ui
ui_DATA = snippets.ui

EXTRA_DIST = $(ui_DATA)

CLEANFILES = *.bak *.gladep *.pyc
DISTCLEANFILES = *.bak *.gladep *.pyc

-include $(top_srcdir)/git.mk
