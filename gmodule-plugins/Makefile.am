## Process this file with automake to produce Makefile.in

if HAVE_GNOME
spell = spell
endif

if HAVE_GNOME
prm = prm
endif

if WITH_GMODULE_PLUGINS

if HAVE_LIBZVT
shell = shell
endif

if HAVE_LIBGNORBA
corba = idl goad launcher
endif

gmodule_plugins = $(shell) $(corba)
endif

SUBDIRS = client cvsdiff diff email encrypt reverse $(spell) \
	  $(prm) convert testselect $(gmodule_plugins)

EXTRA_DIST = plugin_howto gmodule_plugin_howto ChangeLog
