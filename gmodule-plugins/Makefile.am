## Process this file with automake to produce Makefile.in

if WITH_GMODULE_PLUGINS

if HAVE_LIBZVT
shell = shell
endif

if HAVE_LIBGNORBA
corba = idl goad launcher
endif

gmodule_plugins = $(shell) $(corba)
endif

SUBDIRS = client $(gmodule_plugins)

EXTRA_DIST = plugin_howto gmodule_plugin_howto ChangeLog
