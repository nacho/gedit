## Process this file with automake to produce Makefile.in

SUBDIRS = gedit pixmaps po help data plugins viewer

distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'

EXTRA_DIST = BUGS		\
	ChangeLog  		\
	message-of-doom		\
	README 			\
	TODO 			\
	xmldocs.make 		\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in

DISTCLEANFILES = 		\
	intltool-extract 	\
	intltool-merge 		\
	intltool-update
