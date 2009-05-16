## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gedit pixmaps po help data plugin-loaders plugins docs win32

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = 			\
	BUGS			\
	ChangeLog-20011116	\
	ChangeLog-20051212	\
	ChangeLog-20090418	\
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

CHANGELOG_START = GEDIT_2_26_1

dist-hook:
	@if test -d "$(srcdir)/.git"; \
	then \
		echo Creating ChangeLog && \
		(GIT_DIR=$(top_srcdir)/.git \
		  ./missing --run git log $(CHANGELOG_START)^^.. --stat -M -C --name-status  --date=short --no-color) | \
		  fmt --split-only > ChangeLog.tmp \
		&& mv -f ChangeLog.tmp $(top_distdir)/ChangeLog \
		|| ( rm -f ChangeLog.tmp ; \
		  echo Failed to generate ChangeLog >&2 ); \
	else \
		echo A git clone is required to generate a ChangeLog >&2; \
	fi

-include $(top_srcdir)/git.mk
