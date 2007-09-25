dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

m4_define(gedit_major_version,  2)
m4_define(gedit_minor_version, 20)
m4_define(gedit_micro_version,  0)
m4_define(gedit_version, gedit_major_version.gedit_minor_version.gedit_micro_version)

AC_INIT(gedit, gedit_version, http://bugzilla.gnome.org/enter_bug.cgi?product=gedit)
AC_CONFIG_SRCDIR(gedit/gedit.c)
AC_CONFIG_MACRO_DIR([m4])

AC_DEFINE(GEDIT_MAJOR_VERSION, gedit_major_version, [Gedit major version])
AC_SUBST(GEDIT_MAJOR_VERSION, gedit_major_version)
AC_DEFINE(GEDIT_MINOR_VERSION, gedit_minor_version, [Gedit minor version])
AC_SUBST(GEDIT_MINOR_VERSION, gedit_minor_version)
AC_DEFINE(GEDIT_MICRO_VERSION, gedit_micro_version, [Gedit micro version])
AC_SUBST(GEDIT_MICRO_VERSION, gedit_micro_version)

GEDIT_API_VERSION=2.20
AC_SUBST(GEDIT_API_VERSION)

AM_INIT_AUTOMAKE([1.8 dist-bzip2 no-dist-gzip])

AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(config.h)
AC_DISABLE_STATIC

AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL([0.35.0])

AC_PROG_LIBTOOL

GNOME_DOC_INIT
GTK_DOC_CHECK([1.0])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PATH_PROG(GZIP, gzip)

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")

dnl ===============================================================
dnl Expanded dirs
dnl ===============================================================

# This macro expands DIR and assigns it to RET.
# If DIR is NONE, then it's replaced by DEFAULT.
# Based on AC_DEFINE_DIR
AC_DEFUN([GEDIT_FULLPATH], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_expand=[$]$1
  test "x$ac_expand" = xNONE && ac_expand="[$]$2"
  ac_expand=`eval echo [$]ac_expand`
  $3=`eval echo [$]ac_expand`
])

dnl FULL_LIBDIR is used for X-GNOME-Bugzilla-ExtraInfoScript expansion
dnl in data/gedit.desktop.in.in
GEDIT_FULLPATH(libdir, NONE, FULL_LIBDIR)
AC_SUBST(FULL_LIBDIR)

dnl ================================================================
dnl Gettext stuff.
dnl ================================================================

GETTEXT_PACKAGE=gedit
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT


dnl ================================================================
dnl spell plugins checks: enchant and iso-codes
dnl ================================================================

ENCHANT_REQUIRED=1.2.0
ISO_CODES_REQUIRED=0.35
	
AC_ARG_ENABLE([spell],
	AS_HELP_STRING([--disable-spell],[Disable spell plugin (default: enabled)]),
	[enable_enchant=$enableval],
	[enable_enchant=yes])

if test "x$enable_enchant" = "xyes" ; then

	PKG_CHECK_MODULES(ENCHANT, enchant >= $ENCHANT_REQUIRED, \
			  have_enchant=yes, have_enchant=no)

	if test "x$have_enchant" = "xyes"; then

		PKG_CHECK_EXISTS([iso-codes >= $ISO_CODES_REQUIRED],
				 [have_iso_codes=yes],[have_iso_codes=no])

		if test "x$have_iso_codes" = "xyes"; then
			AC_MSG_CHECKING([whether iso-codes has iso-639 and iso-3166 domains])
			if $PKG_CONFIG --variable=domains iso-codes | grep 639 > /dev/null && \
			   $PKG_CONFIG --variable=domains iso-codes | grep 3166 > /dev/null ; then
				result=yes
			else
				result=no
				have_iso_codes=no
			fi
			AC_MSG_RESULT([$result])
		fi

		if test "x$have_iso_codes" = "xyes"; then
			AC_DEFINE_UNQUOTED([ISO_CODES_PREFIX],["`$PKG_CONFIG --variable=prefix iso-codes`"],[ISO codes prefix])
			AC_DEFINE([HAVE_ISO_CODES],[1],[Define if you have the iso-codes package])
		else
			AC_MSG_ERROR([iso-codes is required to build the spell plugin. Use --disable-spell to build without spell plugin.])
		fi
	
		enable_enchant=yes
		ENCHANT_CFLAGS="${ENCHANT_CFLAGS}"
		ENCHANT_LIBS="${ENCHANT_LIBS}"
		SPELL_PLUGIN_DIR="spell"
	else
		AC_MSG_ERROR([Enchant library not found or too old. Use --disable-spell to build without spell plugin.])
	fi
else
	enable_enchant=no
	ENCHANT_CFLAGS=
	ENCHANT_LIBS=
	SPELL_PLUGIN_DIR=
fi

AC_SUBST(ENCHANT_CFLAGS)
AC_SUBST(ENCHANT_LIBS)
AC_SUBST(SPELL_PLUGIN_DIR)

dnl ================================================================
dnl libattr checks
dnl ================================================================

AC_CHECK_LIB(attr, attr_copy_fd)

AC_MSG_CHECKING([whether libattr support is requested])
AC_ARG_ENABLE([attr],
	AS_HELP_STRING([--enable-attr],[Enable libattr support]),
	[enable_attr=$enableval have_attr=$enableval],
	[enable_attr=autodetect have_attr=yes])
AC_MSG_RESULT([$enable_attr])

if test "x$have_attr" != "xno"; then
	AC_CHECK_LIB([attr], [attr_copy_fd], [],
                [
                have_attr=no
                if test "x$enable_attr" = "xyes"; then
                        AC_MSG_ERROR([libattr not found])
                elif test "x$enable_attr" = "xautodetect"; then
                        AC_MSG_WARN([libattr not found, disabling libattr support])
                fi
                ])
fi

dnl ================================================================
dnl Start of pkg-config checks
dnl ================================================================

PKG_CHECK_MODULES(GEDIT, [
	glib-2.0 >= 2.14.0
	gtk+-2.0 >= 2.12.0
	gtksourceview-2.0 >= 2.0.0
	libgnomeui-2.0 >= 2.16.0
	libglade-2.0 >= 2.5.1
	libgnomeprintui-2.2 >= 2.12.1
	gnome-vfs-2.0 >= 2.16.0
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
		/bin/sh ../libtool --mode=link ${CC} -o testpython.la -rpath `pwd` -module -avoid-version -no-undefined $PYTHON_LIB_LOC testpython.lo $PYTHON_LIBS $PYTHON_EXTRA_LIBS >/dev/null 2>&1 && \
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

if test "x$have_python" != "xyes"; then
	if test "x$enable_python" = "xyes"; then
		AC_MSG_ERROR([Python not found])
	elif test "x$enable_python" = "xautodetect"; then
		enable_python=no
		AC_MSG_WARN([Python not found, disabling python support])
	fi
fi

if test "x$have_python" != "xno"; then
	PYGOBJECT_REQUIRED=2.11.5
	PYGTK_REQUIRED=2.9.7
	PYGTKSOURCEVIEW_REQUIRED=2.0.0

	PKG_CHECK_MODULES([PYGTK], [
		pygobject-2.0 >= $PYGOBJECT_REQUIRED
		pygtk-2.0 >= $PYGTK_REQUIRED
		pygtksourceview-2.0 >= $PYGTKSOURCEVIEW_REQUIRED],
		[],
		[
		have_python=no
		if test "x$enable_python" = "xyes"; then
			AC_MSG_ERROR([$PYGTK_PKG_ERRORS])
		elif test "x$enable_python" = "xautodetect"; then
                        enable_python=no
			AC_MSG_WARN([$PYGTK_PKG_ERRORS])
			AC_MSG_WARN([Disabling python support])
		fi
		])

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

if test "x$have_python" != "xno" -a "x$enable_python" != "xno"; then
	enable_python=yes
	AC_DEFINE([ENABLE_PYTHON],[1],[Define to compile with python support])
fi

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

dnl This allows the bug-report script to know whether python has been enabled
AC_SUBST(enable_python)

dnl ================================================================
dnl Misc
dnl ================================================================
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)
			      
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
data/gedit-bugreport.sh
data/gedit.pc
data/Makefile
docs/Makefile
docs/reference/Makefile
gedit/dialogs/Makefile
gedit/Makefile
help/Makefile
help/eu/Makefile
help/ro/Makefile
pixmaps/Makefile
plugins/changecase/Makefile
plugins/docinfo/Makefile
plugins/externaltools/Makefile
plugins/externaltools/tools/Makefile
plugins/externaltools/data/Makefile
plugins/externaltools/scripts/Makefile
plugins/filebrowser/Makefile
plugins/indent/Makefile
plugins/Makefile
plugins/modelines/Makefile
plugins/pythonconsole/Makefile
plugins/pythonconsole/pythonconsole/Makefile
plugins/sample/Makefile
plugins/snippets/Makefile
plugins/snippets/snippets/Makefile
plugins/snippets/data/Makefile
plugins/snippets/data/lang/Makefile
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
	Libattr enabled:	$have_attr
	Python Plugins Support: $enable_python
	Spell Plugin enabled:	$enable_enchant
"

dnl uncomment this in developement releases
dnl cat $srcdir/message-of-doom
