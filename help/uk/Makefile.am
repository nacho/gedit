## Process this file with automake to produce Makefile.in.

EXTRA_DIST = index.html topic.dat \
	     gedit_help.txt

gedit_helpdir = $(datadir)/gnome/help/gedit/no

dist-hook:
	-cp $(EXTRA_DIST) $(distdir)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gedit_helpdir)
	$(INSTALL_DATA)  $(srcdir)/index.html $(DESTDIR)$(gedit_helpdir)
	$(INSTALL_DATA)  $(srcdir)/topic.dat $(DESTDIR)$(gedit_helpdir)

