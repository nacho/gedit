#!/bin/sh

xgettext --default-domain=gedit --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gedit.po \
   || ( rm -f ./gedit.pot \
    && mv gedit.po ./gedit.pot )
