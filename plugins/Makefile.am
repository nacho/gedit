DIST_SUBDIRS = \
	changecase	\
	checkupdate	\
	docinfo		\
	externaltools	\
	filebrowser	\
	modelines	\
	pythonconsole	\
	quickopen	\
	snippets	\
	sort		\
	spell 		\
	time

SUBDIRS = \
	changecase	\
	docinfo		\
	filebrowser	\
	modelines	\
	pythonconsole	\
	quickopen	\
	sort		\
	time

#	snippets

#if !OS_WIN32
#SUBDIRS      += externaltools
#endif

if ENABLE_ENCHANT
SUBDIRS      += spell
endif

if ENABLE_UPDATER
SUBDIRS      += checkupdate
endif

-include $(top_srcdir)/git.mk
