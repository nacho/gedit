## Process this file with automake to produce Makefile.in

SUBDIRS = gedit pixmaps po help data plugins viewer

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = BUGS		\
	ChangeLog  		\
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
