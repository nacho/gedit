platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP
platform_ltlibraries = libeggdesktopfile.la
platform_libs = libeggdesktopfile.la -lSM -lICE
platform_sources = eggsmclient-xsmp.c
platform_app_ldflags =
platform_programs = egg-launch

INCLUDES =                               \
	-DG_LOG_DOMAIN=\""EggSMClient"\" \
	$(platform_defines)              \
	$(GEDIT_CFLAGS)

noinst_LTLIBRARIES =                     \
	libeggsmclient.la                \
	$(platform_ltlibraries)

libeggsmclient_la_LIBADD =               \
	$(GEDIT_LIBS)             \
	$(platform_libs)

libeggsmclient_la_SOURCES =              \
	eggsmclient.c                    \
	eggsmclient.h                    \
	eggsmclient-private.h            \
	$(platform_sources)

libeggdesktopfile_la_LIBADD =            \
	$(GEDIT_LIBS)

libeggdesktopfile_la_SOURCES =           \
	eggdesktopfile.c                 \
	eggdesktopfile.h

EXTRA_DIST =                             \
	README                           \
	gedit.diff                       \
	eggsmclient-osx.c                \
	eggsmclient-win32.c              \
	eggsmclient-xsmp.c
