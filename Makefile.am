## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = bindings gedit pixmaps po help data plugins

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
