# $Id: gpm.spec,v 1.2 2002/05/28 19:13:55 nico Exp $

# this defines the library version that this package builds.
%define	LIBVER		1.18.0
%define BUILD_GPM_ROOT	'no'

Summary: A mouse server for the Linux console.
Name: gpm
Version: 1.19.3
Release: 8owl
License: GPL
Group: System Environment/Daemons
Source0: ftp://ftp.systemy.it/pub/develop/%{name}-%{version}.tar.gz
Source1: gpm.init
Patch0: gpm-1.19.3-rh-install-no-root.diff
Patch1: gpm-1.19.3-rh-no-ps.diff
Patch2: gpm-1.19.3-rh-doc.diff
Patch3: gpm-1.19.3-rh-owl-socket-mode.diff
Patch4: gpm-1.19.3-rh-gpm-root.diff
Patch5: gpm-1.19.3-owl-gpm-root.diff
Patch6: gpm-1.19.3-immunix-owl-tmp.diff
Patch7: gpm-1.19.3-owl-liblow.diff
Patch8: gpm-1.19.3-owl-warnings.diff
Prereq: /sbin/chkconfig /sbin/ldconfig /sbin/install-info /etc/rc.d/init.d
BuildRequires: bison
BuildRoot: /var/rpm-buildroot/%{name}-root

%description
gpm provides mouse support to text-based Linux applications as well as
console cut-and-paste operations using the mouse.

%package devel
Requires: %{name} = %{version}-%{release}
Summary: Libraries and header files for developing mouse driven programs.
Group: Development/Libraries

%description devel
The gpm-devel package contains the libraries and header files needed
for the development of mouse driven programs for the console.

%if "%{BUILD_GPM_ROOT}"=="'yes'"
%package root
Requires: %{name} = %{version}-%{release}
Summary: A mouse server add-on which draws pop-up menus on the console.
Group: System Environment/Daemons

%description root
The gpm-root program allows pop-up menus to appear on a text console
at the click of a mouse button.
%endif

%prep
%setup -q
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1

%build
autoconf
CFLAGS="-D_GNU_SOURCE $RPM_OPT_FLAGS" \
	lispdir=%{buildroot}%{_datadir}/emacs/site-lisp \
	%configure
rm gpm-root.c doc/*.[178] doc/gpm.info
make CFLAGS="" CPPFLAGS=""

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sysconfdir}

PATH=/sbin:/usr/sbin:$PATH

mkdir -p %{buildroot}%{_datadir}/emacs/site-lisp
%makeinstall lispdir=%{buildroot}%{_datadir}/emacs/site-lisp

install -m 644 doc/gpm-root.1 %{buildroot}%{_mandir}/man1
install -m 644 gpm-root.conf %{buildroot}%{_sysconfdir}
install -s -m 755 hltest %{buildroot}%{_bindir}
make t-mouse.el t-mouse.elc
cp t-mouse.el* %{buildroot}%{_datadir}/emacs/site-lisp

pushd %{buildroot}
chmod +x .%{_libdir}/libgpm.so.%{LIBVER}
ln -sf libgpm.so.%{LIBVER} .%{_libdir}/libgpm.so
gzip -9nf .%{_infodir}/gpm.info*
popd

mkdir -p %{buildroot}%{_sysconfdir}/rc.d/init.d
install -m 755 $RPM_SOURCE_DIR/gpm.init %{buildroot}%{_sysconfdir}/rc.d/init.d/gpm

%clean
rm -rf %{buildroot}

%pre
rm -f /var/run/gpm.restart
if [ $1 -ge 2 ]; then
	/etc/rc.d/init.d/gpm status && touch /var/run/gpm.restart || :
	/etc/rc.d/init.d/gpm stop || :
fi

%post
if [ $1 -eq 1 ]; then
	/sbin/chkconfig --add gpm
fi
if [ -f /var/run/gpm.restart ]; then
	/etc/rc.d/init.d/gpm start
fi
rm -f /var/run/gpm.restart
/sbin/ldconfig
/sbin/install-info %{_infodir}/gpm.info.gz %{_infodir}/dir

%preun
if [ $1 -eq 0 ]; then
	/sbin/install-info %{_infodir}/gpm.info.gz --delete %{_infodir}/dir
	/etc/rc.d/init.d/gpm stop || :
	/sbin/chkconfig --del gpm
fi

%postun
/sbin/ldconfig

%files
%defattr(-,root,root)
%{_bindir}/mev
%{_bindir}/hltest
/usr/sbin/gpm
%{_datadir}/emacs/site-lisp/t-mouse.el
%{_datadir}/emacs/site-lisp/t-mouse.elc
%{_infodir}/gpm.info*
%{_mandir}/man1/mev.1*
%{_mandir}/man8/gpm.8*
%{_libdir}/libgpm.so.%{LIBVER}
%config %{_sysconfdir}/rc.d/init.d/gpm

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libgpm.a
%{_libdir}/libgpm.so

%if "%{BUILD_GPM_ROOT}"=="'yes'"
%files root
%defattr(-,root,root)
%config %{_sysconfdir}/gpm-root.conf
%{_bindir}/gpm-root
%{_mandir}/man1/gpm-root.1*
%endif

%changelog
* Wed Jun 27 2001 Solar Designer <solar@owl.openwall.com>
- Disabled packaging gpm-root by default.

* Tue Jun 26 2001 Solar Designer <solar@owl.openwall.com>
- Moved gpm-root to a separate subpackage.
- Disabled support for ~/.gpm-root because of too many security issues
with this feature, updated the documentation accordingly.
- Fixed many gpm-root reliability bugs including the format string bug
reported by Colin Phipps to Debian (http://bugs.debian.org/102031) and
several other bugs which were about as bad.

* Sun May 27 2001 Alexandr D. Kanevskiy <kad@owl.openwall.com>
- hack to avoid double use of $RPM_OPT_FLAGS

* Sat Jan 06 2001 Solar Designer <solar@owl.openwall.com>
- Updated the patches for fail-closeness in many cases.
- Re-generate gpm-root.c at build time, to avoid maintaining two patches.
- /tmp fixes in the documentation (don't suggest bad practices).
- More startup script cleanups.
- Restart after package upgrades in an owl-startup compatible way.

* Fri Jan 05 2001 Alexandr D. Kanevskiy <kad@owl.openwall.com>
- import mktemp patch from Immunix, fix strncpy

* Sun Dec 24 2000 Alexandr D. Kanevskiy <kad@owl.openwall.com>
- import from RH
