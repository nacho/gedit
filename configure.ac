dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT(gedit, 2.15.1, http://bugzilla.gnome.org/enter_bug.cgi?product=gedit)
AC_CONFIG_SRCDIR(gedit/gedit.c)
AC_CONFIG_MACRO_DIR([m4])

GEDIT_MAJOR=2.14
AC_SUBST(GEDIT_MAJOR)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

GNOME_DOC_INIT

AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)
AM_DISABLE_STATIC

AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL([0.34.90])

AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")


dnl ================================================================
dnl Gettext stuff.
dnl ================================================================

GETTEXT_PACKAGE=gedit
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT


dnl ================================================================
dnl pspell checks
dnl ================================================================

AC_MSG_CHECKING([for aspell/pspell])
if pspell-config version > /dev/null 2>&1; then
	pspell_ver=`pspell-config version`
	pspell_num=`echo ${pspell_ver} | awk 'BEGIN {FS="."}{printf $1*10000 + $2*100 + $3;}'`
	if test ${pspell_num} -lt 1200; then
		AC_MSG_RESULT([pspell version .12 or later required, you have ${pspell_ver} installed])
	else
		AC_MSG_RESULT([found ($pspell_ver)])

		AC_CHECK_HEADER(pspell/pspell.h,
			[AC_CHECK_LIB(aspell, new_aspell_speller, 
				[PSPELL_LIBS="-laspell" SPELL_PLUGIN_DIR="spell" AC_SUBST(SPELL_PLUGIN_DIR)],
   				[AC_CHECK_LIB(pspell, new_pspell_manager, 
					[PSPELL_LIBS="-lpspell" SPELL_PLUGIN_DIR="spell" AC_SUBST(SPELL_PLUGIN_DIR)],
					[AC_MSG_WARN([You must have the aspell or pspell dev packages to build the spell plugin.])] 
				)]
			)],
			[AC_MSG_WARN([You must have the aspell or pspell dev packages to build the spell plugin.])] 
		)
	fi
else
	AC_MSG_RESULT([not found])
fi

AC_SUBST(PSPELL_LIBS)


dnl ================================================================
dnl Start of pkg-config checks
dnl ================================================================

PKG_CHECK_MODULES(GEDIT, [
	glib-2.0 >= 2.8.0
	gtk+-2.0 >= 2.8.0
	gtksourceview-1.0 >= 1.2.0
	libgnomeui-2.0 >= 2.13.0
	libglade-2.0 >= 2.4.0
	libgnomeprintui-2.2 >= 2.6.0
	gnome-vfs-2.0 >= 2.13.4
])
GEDIT_LIBS="${GEDIT_LIBS}"
AC_SUBST(GEDIT_LIBS)
AC_SUBST(GEDIT_CFLAGS)


dnl ================================================================
dnl GConf related settings
dnl ================================================================

AM_GCONF_SOURCE_2

dnl ================================================================
dnl Python
dnl ================================================================

AC_MSG_CHECKING([whether Python support is requested])
AC_ARG_ENABLE([python],
	AS_HELP_STRING([--enable-python],[Enable python support]),
	[enable_python=$enableval have_python=$enableval],
	[enable_python=autodetect have_python=yes])
AC_MSG_RESULT([$enable_python])

if test "x$have_python" != "xno"; then
	AM_PATH_PYTHON([2.3],[],[no])
	if test "x$PYTHON" = "x:"; then
		have_python=no
	fi
fi

if test "x$have_python" != "xno"; then
	AM_CHECK_PYTHON_HEADERS([],[have_python=no])
fi

if test "x$have_python" != "xno"; then
	PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
	PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
	PYTHON_LIBS="-lpython$PYTHON_VERSION"
	PYTHON_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
	PYTHON_CFLAGS="-I$PY_PREFIX/include/python$PYTHON_VERSION"
	PYTHON_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"
	PYTHON_LOCALMODLIBS=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_EXTRA_LIBS="$PYTHON_LOCALMODLIBS $PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS"
	AC_SUBST([PYTHON_LIBS])
	AC_SUBST([PYTHON_LIB_LOC])
	AC_SUBST([PYTHON_CFLAGS])
	AC_SUBST([PYTHON_EXTRA_LIBS])

	dnl FIXME: do we really need this test?
	AC_MSG_CHECKING([whether we can build a shared library depending on libpython])
	rm -rf testpython
	mkdir testpython
	cd testpython
	cat > testpython.c <<EOF
#include <Python.h>
int testpython (void)
{
Py_Exit (0);
}
EOF

	if /bin/sh ../libtool --mode=compile ${CC} $PYTHON_CFLAGS -c testpython.c >/dev/null 2>&1 && \
		/bin/sh ../libtool --mode=link ${CC} -o testpython.la -rpath `pwd` -module -avoid-version $PYTHON_LIB_LOC testpython.lo $PYTHON_LIBS $PYTHON_EXTRA_LIBS >/dev/null 2>&1 && \
		grep 'dlname.*testpython' testpython.la >/dev/null 2>&1; then
		result=yes
	else
		result=no
		have_python=no
	fi
	cd ..
	rm -rf testpython
	AC_MSG_RESULT([$result])
fi

if test "x$have_python" != "xno"; then
	PYGTK_REQUIRED=2.8.0
	GNOME_PYTHON_DESKTOP_REQUIRED=2.13.3

	PKG_CHECK_MODULES([PYGTK], [
		pygtk-2.0 >= $PYGTK_REQUIRED
		gnome-python-desktop-2.0 >= $GNOME_PYTHON_DESKTOP_REQUIRED],
		[],[have_python=no])

	AC_SUBST([PYGTK_CFLAGS])
	AC_SUBST([PYGTK_LIBS])
fi

if test "x$have_python" != "xno"; then
	AC_MSG_CHECKING([for pygtk defs])
	PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
	AC_MSG_RESULT([$PYGTK_DEFSDIR])

	AC_MSG_CHECKING([for pygtk codegen])
	PYGTK_CODEGEN="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/codegen.py"
	AC_MSG_RESULT([$PYGTK_CODEGEN])

	AC_MSG_CHECKING([for pygtk h2def])
	PYGTK_H2DEF="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/h2def.py"
	AC_MSG_RESULT([$PYGTK_H2DEF])

	AC_SUBST([PYGTK_DEFSDIR])
	AC_SUBST([PYGTK_CODEGEN])
	AC_SUBST([PYGTK_H2DEF])

	dnl Check for -fno-strict-aliasing
	FLAGS="-fno-strict-aliasing"
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $FLAGS"
	AC_MSG_CHECKING([whether [$]CC understands $FLAGS])
	AC_TRY_COMPILE([], [], [compiler_has_option=yes], [compiler_has_option=no])
	CFLAGS="$save_CFLAGS"
	AC_MSG_RESULT($compiler_has_option)
	if test $compiler_has_option = yes; then
		NO_STRICT_ALIASING_CFLAGS="$FLAGS"
	fi
	AC_SUBST([NO_STRICT_ALIASING_CFLAGS])
fi

if test "x$have_python" != "xyes"; then
	if test "x$enable_python" = "xyes"; then
		AC_MSG_ERROR([Python not found])
	elif test "x$enable_python" = "xautodetect"; then
		enable_python=no
		AC_MSG_WARN([Python not found, disabling python support])
	fi
elif test "x$enable_python" != "xno"; then
	enable_python=yes
	AC_DEFINE([ENABLE_PYTHON],[1],[Define to compile with python support])
fi

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

dnl ================================================================
dnl Misc
dnl ================================================================
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
			      
GNOME_COMPILE_WARNINGS(yes)

AC_ARG_ENABLE(deprecations,
              [AC_HELP_STRING([--enable-deprecations],
                              [warn about deprecated usages [default=no]])],,
              [enable_deprecations=no])

if test "x$enable_deprecations" = "xyes"; then
   DISABLE_DEPRECATED_CFLAGS="\
-DG_DISABLE_DEPRECATED \
-DGDK_DISABLE_DEPRECATED \
-DGTK_DISABLE_DEPRECATED \
-DGDK_PIXBUF_DISABLE_DEPRECATED \
-DGNOME_DISABLE_DEPRECATED"
   AC_SUBST(DISABLE_DEPRECATED_CFLAGS)
fi

PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

AC_CONFIG_FILES([
Makefile
bindings/Makefile
bindings/python/Makefile
data/gedit.desktop.in
data/gedit.pc
data/Makefile
gedit/dialogs/Makefile
gedit/Makefile
gedit/recent-files/Makefile
help/Makefile
help/eu/Makefile
help/ko/Makefile
help/ro/Makefile
help/sv/Makefile
pixmaps/Makefile
plugins/changecase/Makefile
plugins/docinfo/Makefile
plugins/externaltools/Makefile
plugins/externaltools/tools/Makefile
plugins/indent/Makefile
plugins/Makefile
plugins/modelines/Makefile
plugins/pythonconsole/Makefile
plugins/pythonconsole/pythonconsole/Makefile
plugins/sample/Makefile
plugins/snippets/Makefile
plugins/snippets/snippets/Makefile
plugins/snippets/data/Makefile
plugins/sort/Makefile
plugins/spell/Makefile
plugins/taglist/Makefile
plugins/time/Makefile
po/Makefile.in])

AC_OUTPUT

echo "

Configuration:

	Source code location:	${srcdir}
	Compiler:		${CC}
	Python Plugins Support: $enable_python

"

dnl uncomment this in developement releases
dnl cat $srcdir/message-of-doom
