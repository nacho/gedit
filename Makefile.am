## Process this file with automake to produce Makefile.in

SUBDIRS = gedit pixmaps po help data plugins viewer

distcleancheck_listfiles = find . -type f -print | grep -v 'omf\.out'

distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper' | grep -v 'omf' | grep -v 'figures'

EXTRA_DIST = BUGS		\
	ChangeLog  		\
	message-of-doom		\
	MAINTAINERS		\
	README 			\
	TODO 			\
	xmldocs.make 		\
	omf.make		\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in	

DISTCLEANFILES = 		\
	intltool-extract 	\
	intltool-merge 		\
	intltool-update
