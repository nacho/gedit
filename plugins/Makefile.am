DIST_SUBDIRS =		\
	changecase 	\
	docinfo 	\
	filebrowser 	\
	indent 		\
	modelines	\
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
	time		\
	$(SPELL_PLUGIN_DIR)

if ENABLE_PYTHON
DIST_SUBDIRS += externaltools pythonconsole snippets quickopen
SUBDIRS      += externaltools pythonconsole snippets quickopen
endif


-include $(top_srcdir)/git.mk
