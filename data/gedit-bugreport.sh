#!/bin/sh
echo "Active plugins:"
gconftool-2 --get /apps/gedit-2/plugins/active-plugins			\
	| sed -r -e 's/^\[(.*)\]$/\1/' -e 's/,/\n/g'			\
	| sed -e 's/^.*$/  - \0/'
echo

# Manually installed plugins (in $HOME)
if [ -d $HOME/.gnome2/gedit/plugins ]
then
	echo "Plugins in \$HOME:"
	ls $HOME/.gnome2/gedit/plugins/*.gedit-plugin			\
		| sed -r -e 's#.*/([^/]*)\.gedit-plugin$#  - \1#'
else
	echo "No plugin installed in \$HOME."
fi
echo
