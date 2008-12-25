## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gedit pixmaps po help data plugin-loaders plugins docs win32

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = 			\
	BUGS			\
	ChangeLog  		\
	HACKING			\
	MAINTAINERS		\
	xmldocs.make 		\
	omf.make		\
	message-of-doom		\
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
