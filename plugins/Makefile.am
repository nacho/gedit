DIST_SUBDIRS = \
	changecase	\
	checkupdate	\
	docinfo		\
	filebrowser	\
	modelines	\
	sort		\
	taglist		\
	time

#	spell 		\
#	externaltools
#	pythonconsole
#	quickopen
#	snippets

SUBDIRS = \
	changecase	\
	docinfo		\
	filebrowser	\
	modelines	\
	sort		\
	taglist		\
	time

#	filebrowser
#	modelines
#	pythonconsole
#	quickopen
#	snippets

#if !OS_WIN32
#SUBDIRS      += externaltools
#endif

#if ENABLE_ENCHANT
#SUBDIRS      += spell
#endif

if ENABLE_UPDATER
SUBDIRS      += checkupdate
endif

-include $(top_srcdir)/git.mk
