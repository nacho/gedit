#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
export srcdir

PKG_NAME="gEdit"

(test -f $srcdir/makeconfig.pl \
  && test -f $srcdir/src/gedit.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

#perl $srcdir/editor-plugins/plugins.pl
#perl $srcdir/makeconfig.pl

. $srcdir/macros/autogen.sh