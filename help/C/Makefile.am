## Process this file with automake to produce Makefile.in.

EXTRA_DIST = index.html \
	     topic.dat  \ 
	     gedit_help.txt

gedit_helpdir = $(datadir)/gnome/help/gedit/C

install-data-local:
	$(mkinstalldirs) $(gedit_helpdir)
	$(INSTALL_DATA)  $(srcdir)/index.html $(gedit_helpdir)
	$(INSTALL_DATA)  $(srcdir)/topic.dat $(gedit_helpdir)

