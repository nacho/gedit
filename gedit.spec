# Note that this is NOT a relocatable package
%define ver      0.5.0
%define rel      1
%define prefix   /usr

Summary:   gEdit 
Name:      gedit
Version:   %ver
Release:   %rel
Copyright: GPL
Group:     Editors
Source0:   gedit-%{PACKAGE_VERSION}.tar.gz
URL:       http://gedit.pn.org
BuildRoot: /tmp/gedit-%{PACKAGE_VERSION}-root
Packager: Alex Roberts <bse@dial.pipex.com>
Requires: gtk+ >= 1.0.7
Requires: gnome-libs
#Docdir: %{prefix}/doc

%description
gEdit is a small but powerful text editor for GTK+ and/or GNOME.

%package devel
Summary: gEdit is a small but powerful text editor for GTK+ and/or GNOME.
Group: Editors

%description devel
gEdit is a small but powerful text editor for GTK+ and/or GNOME.
 
%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" LDFLAGS="-s" ./configure \
	--prefix=%{prefix} 

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
rm -rf $RPM_BUILD_ROOT
#install -d $RPM_BUILD_ROOT/etc/{rc.d/init.d,pam.d,profile.d,X11/wmconfig}

make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%doc README COPYING ChangeLog NEWS TODO AUTHORS INSTALL THANKS
%{prefix}/bin/gedit
%{prefix}/share/locale/*/*/*
%{prefix}/share/apps/*/*
%{prefix}/share/pixmaps/*
%{prefix}/share/mime-info/*
%{prefix}/share/geditrc
%{prefix}/man/*/*
%{prefix}/libexec/*/*/*


%files devel
%defattr(-, root, root)
%{prefix}/include/*/*
%{prefix}/lib/*

%changelog

* Thu Oct 22 1998 Alex Roberts <bse@dial.pipex.com>

- First try at an RPM




