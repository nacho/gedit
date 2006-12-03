## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = bindings gedit pixmaps po help data plugins docs

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = BUGS		\
	ChangeLog  		\
	MAINTAINERS		\
	xmldocs.make 		\
	omf.make		\
	m4			\
	gnome-doc-utils.make    \
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in	

DISTCLEANFILES = 		\
	gnome-doc-utils.make    \
	intltool-extract 	\
	intltool-merge 		\
	intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-gtk-doc
