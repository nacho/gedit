INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/gedit 					\
	-I$(top_builddir)/gedit					\
	$(GEDIT_CFLAGS) 					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)

noinst_LTLIBRARIES = libosx.la

libosx_la_LDFLAGS = -framework Carbon -framework ApplicationServices
libosx_la_LIBADD = -lobjc
libosx_la_CFLAGS = -xobjective-c

libosx_la_SOURCES = 	\
	gedit-osx.c 	\
	gedit-osx.h

-include $(top_srcdir)/git.mk
