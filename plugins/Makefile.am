DIST_SUBDIRS =		\
	changecase 	\
	checkupdate	\
	docinfo 	\
	externaltools	\
	filebrowser 	\
	indent 		\
	modelines	\
	pythonconsole	\
	quickopen	\
	snippets	\
	sort 		\
	spell 		\
	taglist 	\
	time

SUBDIRS = 		\
	changecase	\
	docinfo		\
	filebrowser	\
	indent		\
	modelines	\
	sort		\
	taglist		\
	time

if ENABLE_PYTHON
SUBDIRS      += externaltools pythonconsole snippets quickopen
endif

if ENABLE_ENCHANT
SUBDIRS      += spell
endif

if ENABLE_UPDATER
SUBDIRS      += checkupdate
endif

-include $(top_srcdir)/git.mk
