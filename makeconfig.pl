#! /usr/bin/perl

die unless chdir( "$ENV{srcdir}/" );
open DIRLIST, "editor-plugins/plugins.dir.list";
while ( <DIRLIST> )
{
    s/#.*\n?$//; next if /^\s*$/; s/\s*$//; s/^\s*//; push @direntry, $_ if length;    
}

open MAKEFILE, "> configure.in";
print MAKEFILE << 'EOF';
dnl This is a generated file.  Please see makeconfig.pl.
AC_INIT(src/gedit.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(gedit, 0.5.4pre)

AM_MAINTAINER_MODE
AM_ACLOCAL_INCLUDE(macros)

GNOME_INIT

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_ARG_PROGRAM
AM_PROG_LIBTOOL


GNOME_X_CHECKS


AM_CONDITIONAL(FALSE, test foo = bar)

	    
dnl Let the user enable the new GModule Plugins
AC_ARG_WITH(gmodule-plugins,
	    [  --with-gmodule-plugins  Enable GModule Plugins [default=no]],
	    enable_gmodule_plugins="$withval", enable_gmodule_plugins=no)

dnl We need to have GNOME to use them
have_gmodule_plugins=no
  if test x$enable_gmodule_plugins = xyes ; then
    AC_DEFINE(WITH_GMODULE_PLUGINS)
    have_gmodule_plugins=yes
  fi

AM_CONDITIONAL(WITH_GMODULE_PLUGINS, test x$have_gmodule_plugins = xyes)

dnl Let the user disable ORBit even if it can be found
AC_ARG_ENABLE(orbit,
	      [  --enable-orbit          Try to use ORBit [default=no] ],
	      enable_orbit="$enableval", enable_orbit=no)
dnl We only need CORBA for plugins
if test x$have_gmodule_plugins = xyes ; then
  if test x$enable_orbit = xyes ; then
    GNOME_ORBIT_HOOK([have_orbit=yes])
  fi
fi
AM_CONDITIONAL(HAVE_ORBIT, test x$have_orbit = xyes)

dnl Check if we also have LibGnorba
if test x$have_orbit = xyes; then
  AC_CHECK_LIB(gnorba, gnome_CORBA_init, have_libgnorba=yes, have_libgnorba=no,
               $ORBIT_LIBS $GNOMEUI_LIBS)
fi
if test x$have_libgnorba = xyes ; then
  AC_DEFINE(HAVE_LIBGNORBA)
fi
AM_CONDITIONAL(HAVE_LIBGNORBA, test x$have_libgnorba = xyes)

dnl Check for libzvt from gnome-libs/zvt
AC_CHECK_LIB(zvt, zvt_term_new, have_libzvt=yes, have_libzvt=no, $GNOMEUI_LIBS)
AM_CONDITIONAL(HAVE_LIBZVT, test x$have_libzvt = xyes)


ALL_LINGUAS="cs de es fr ga it ko nl no pt pt_BR ru sv wa zh_TW.Big5"
AM_GNU_GETTEXT


AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

AC_OUTPUT([
gedit.spec
Makefile
intl/Makefile
po/Makefile.in
macros/Makefile
help/Makefile
help/C/Makefile
help/no/Makefile
src/Makefile
gmodule-plugins/Makefile
gmodule-plugins/idl/Makefile
gmodule-plugins/goad/Makefile
gmodule-plugins/client/Makefile
gmodule-plugins/shell/Makefile
gmodule-plugins/launcher/Makefile
editor-plugins/Makefile
EOF

if ( $#direntry + 1 > 0 )
{
    foreach $i (@direntry)
    {
	print MAKEFILE "editor-plugins/$i/Makefile\n";
    }
}

print MAKEFILE << "EOF";
],[sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])
EOF
