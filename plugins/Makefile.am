## Process this file with automake to produce Makefile.in

if FALSE
extra_dirs = hello reverse testselect
endif

if HAVE_GNOME
spell = spell
prm = prm
ctag = ctag
endf

if WITH_GMODULE_PLUGINS

if HAVE_LIBZVT
shell = shell
endif

if HAVE_LIBGNORBA
corba = idl goad launcher
endif

gmodule_plugins = $(shell) $(corba)
endif

SUBDIRS = client $(ctag) cvsdiff diff email browse encrypt $(spell) \
	  $(prm) convert $(gmodule_plugins) $(extra_dirs)

EXTRA_DIST = plugin_howto gmodule_plugin_howto ChangeLog
