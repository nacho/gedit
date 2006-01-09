#!/bin/sh

sed_it () {
	sed \
	-e 's/gtktextregion/gedittextregion/g' \
	-e 's/gtk_text_region/gedit_text_region/g' \
	-e 's/GtkTextRegion/GeditTextRegion/g' \
	-e 's/GTK_TEXT_REGION/GEDIT_TEXT_REGION/g' \
	$1
}

sed_it ../../gtksourceview/gtksourceview/gtktextregion.h > gedittextregion.h
sed_it ../../gtksourceview/gtksourceview/gtktextregion.c > gedittextregion.c
