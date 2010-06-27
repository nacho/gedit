DIST_SUBDIRS = \
	checkupdate	\
	modelines	\
	changecase	\
	docinfo		\
	sort

#	filebrowser 	\
#	modelines	\
#	spell 		\
#	taglist 	\
#	time
#	externaltools
#	pythonconsole
#	quickopen
#	snippets

SUBDIRS = \
	modelines	\
	changecase	\
	docinfo		\
	sort

#	filebrowser
#	modelines
#	taglist
#	time
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
