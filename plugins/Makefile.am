## Process this file with automake to produce Makefile.in

if HAVE_GNOME
spell = spell
endif

if WITH_GMODULE_PLUGINS

if HAVE_LIBZVT
shell = shell
endif

gmodule_plugins = $(shell)
endif

SUBDIRS = client cvsdiff diff email encrypt reverse $(spell) \
	  prm convert testselect $(gmodule_plugins)

EXTRA_DIST = plugin_howto gmodule_plugin_howto ChangeLog
