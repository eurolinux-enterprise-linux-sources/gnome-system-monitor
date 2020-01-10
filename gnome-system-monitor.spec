%global libgtop2_version 2.28.2
%global libwnck_version 2.91.0
%global desktop_file_utils_version 0.2.90

Name:           gnome-system-monitor
Version:        3.22.2
Release:        3%{?dist}
Summary:        Process and resource monitor

License:        GPLv2+
URL:            http://www.gnome.org/
Source0:        http://download.gnome.org/sources/%{name}/3.22/%{name}-%{version}.tar.xz
# https://bugzilla.redhat.com/show_bug.cgi?id=1449674
Patch0:         gnome-system-monitor-3.22.2-ja-translation.patch
Patch1:         0001-Add-a-process-GPU-memory-usage-column.patch

BuildRequires:  pkgconfig(libgtop-2.0) >= %{libgtop2_version}
BuildRequires:  pkgconfig(libwnck-3.0) >= %{libwnck_version}
BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(gtkmm-3.0)
BuildRequires:  pkgconfig(libsystemd)
BuildRequires:  pkgconfig(librsvg-2.0)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  desktop-file-utils
BuildRequires:  intltool gettext
BuildRequires:  itstool
BuildRequires:  automake, autoconf, libtool, gnome-common
BuildRequires:  yelp-tools

%description
gnome-system-monitor allows to graphically view and manipulate the running
processes on your system. It also provides an overview of available resources
such as CPU and memory.

%prep
%setup -q
%patch0 -p1
%patch1 -p1

%build
autoreconf -fiv
%configure --enable-systemd --enable-wnck
make %{?_smp_mflags}

%install
%make_install

%find_lang %{name} --with-gnome

%check
desktop-file-validate $RPM_BUILD_ROOT%{_datadir}/applications/gnome-system-monitor.desktop
desktop-file-validate $RPM_BUILD_ROOT%{_datadir}/applications/gnome-system-monitor-kde.desktop

%postun
if [ $1 -eq 0 ]; then
  glib-compile-schemas %{_datadir}/glib-2.0/schemas &>/dev/null || :
fi

%posttrans
glib-compile-schemas %{_datadir}/glib-2.0/schemas &>/dev/null || :

%files -f %{name}.lang
%license COPYING
%doc AUTHORS NEWS README
%{_bindir}/gnome-system-monitor
%{_datadir}/appdata/gnome-system-monitor.appdata.xml
%{_datadir}/applications/gnome-system-monitor.desktop
%{_datadir}/applications/gnome-system-monitor-kde.desktop
%{_datadir}/glib-2.0/schemas/org.gnome.gnome-system-monitor.enums.xml
%{_datadir}/glib-2.0/schemas/org.gnome.gnome-system-monitor.gschema.xml
%{_datadir}/polkit-1/actions/org.gnome.gnome-system-monitor.policy
%{_libexecdir}/gnome-system-monitor/gsm-*

%changelog
* Thu Nov  9 2017 Rui Matos <rmatos@redhat.com> - 3.22.2-3
- Add a process GPU memory usage column
- Related: #1300852

* Tue May 30 2017 David King <dking@redhat.com> - 3.22.2-2
- Update Japanese translation (#1449674)

* Thu Nov 24 2016 Kalev Lember <klember@redhat.com> - 3.22.2-1
- Update to 3.22.2
- Resolves: #1386962

* Tue Mar 29 2016 David King <dking@redhat.com> - 3.14.1-4
- Update translations (#1272383)

* Thu Aug 20 2015 David King <dking@redhat.com> - 3.14.1-3
- Improve detection of number of CPUs (#1254332)

* Wed May 20 2015 David King <dking@redhat.com> - 3.14.1-2
- No-change rebuild to hopefully pacify rpmdiff multilib check

* Mon Mar 23 2015 Richard Hughes <rhughes@redhat.com> - 3.14.1-1
- Update to 3.14.1
- Resolves: #1174566

* Thu Aug 21 2014 David King <dking@redhat.com> - 3.8.2.1-6
- Rebuild for libgtop2 soversion change (#1040501)

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 3.8.2.1-5
- Mass rebuild 2014-01-24

* Fri Jan 03 2014 David King <dking@redhat.com> - 3.8.2.1-4
- Fix compiler warning in > 64 CPUs patch (#1048289)

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 3.8.2.1-3
- Mass rebuild 2013-12-27

* Tue Dec 24 2013 David King <dking@redhat.com> - 3.8.2.1-2
- Display more than 64 CPUs correctly (#1040501)

* Tue Dec 17 2013 Soren Sandmann <ssp@redhat.com> - 3.8.2.1-2
- Translation fixes (rhbz 1030351)

* Mon Dec 16 2013 Soren Sandmann <ssp@redhat.com> - 3.8.2.1-2
- Fix for rhbz 1015507

* Tue May 21 2013 Kalev Lember <kalevlember@gmail.com> - 3.8.2.1-1
- Update to 3.8.2.1

* Mon May 13 2013 Richard Hughes <rhughes@redhat.com> - 3.8.2-1
- Update to 3.8.2

* Tue Mar 26 2013 Kalev Lember <kalevlember@gmail.com> - 3.8.0-1
- Update to 3.8.0

* Wed Mar 20 2013 Kalev Lember <kalevlember@gmail.com> - 3.7.92-1
- Update to 3.7.92

* Tue Feb 19 2013 Richard Hughes <rhughes@redhat.com> - 3.7.90-1
- Update to 3.7.90

* Wed Feb 06 2013 Kalev Lember <kalevlember@gmail.com> - 3.7.5-1
- Update to 3.7.5

* Wed Jan 16 2013 Richard Hughes <hughsient@gmail.com> - 3.7.4-1
- Update to 3.7.4

* Sat Nov 24 2012 Matthias Clasen <mclasen@redhat.com> - 3.6.1-3
- Fix display of distro information

* Mon Oct 29 2012 Matthias Clasen <mclasen@redhat.com> - 3.6.1-2
- Display systemd information

* Tue Oct 16 2012 Kalev Lember <kalevlember@gmail.com> - 3.6.1-1
- Update to 3.6.1

* Wed Sep 26 2012 Matthias Clasen <mclasen@redhat.com> - 3.6.0-1
- Update to 3.6.0

* Wed Sep 19 2012 Richard Hughes <hughsient@gmail.com> - 3.5.92-1
- Update to 3.5.92

* Tue Aug 21 2012 Richard Hughes <hughsient@gmail.com> - 3.5.90.1-1
- Update to 3.5.90.1

* Tue Aug 07 2012 Richard Hughes <hughsient@gmail.com> - 3.5.90-1
- Update to 3.5.90

* Fri Jul 27 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.4.1-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sat Jul 21 2012 Ville Skyttä <ville.skytta@iki.fi> - 3.4.1-3
- Own the %%{_datadir}/gnome-system-monitor dir.
- Escape rpm macros in %%changelog.

* Tue Apr 24 2012 Kalev Lember <kalevlember@gmail.com> - 3.4.1-2
- Silence rpm scriptlet output

* Tue Apr 17 2012 Kalev Lember <kalevlember@gmail.com> - 3.4.1-1
- Update to 3.4.1

* Tue Mar 27 2012 Richard Hughes <hughsient@gmail.com> - 3.4.0-1
- Update to 3.4.0

* Wed Mar 21 2012 Kalev Lember <kalevlember@gmail.com> - 3.3.92-1
- Update to 3.3.92

* Tue Mar 6 2012 Matthias Clasen <mclasen@redhat.com> - 3.3.91-1
- Update to 3.3.91

* Sun Feb 26 2012 Matthias Clasen <mclasen@redhat.com> - 3.3.90-1
- Update to 3.3.90

* Tue Jan 17 2012 Matthias Clasen <mclasen@redhat.com> - 3.3.4-1
- Update to 3.3.4

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.3.3-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Dec 20 2011 Matthias Clasen <mclasen@redhat.com> - 3.3.3-1
- Update to 3.3.3

* Thu Nov 24 2011 Matthias Clasen <mclasen@redhat.com> - 3.3.2.1-1
- Update to 3.3.2.1

* Mon Nov 21 2011 Matthias Clasen <mclasen@redhat.com> - 3.3.2-1
- Update to 3.3.2

* Wed Oct 26 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.2.1-2
- Rebuilt for glibc bug#747377

* Wed Oct 19 2011 Matthias Clasen <mclasen@redhat.com> - 3.2.1-1
- Update to 3.2.1

* Tue Sep 27 2011 Ray <rstrode@redhat.com> - 3.2.0-1
- Update to 3.2.0

* Wed Aug 31 2011 Matthias Clasen <mclasen@redhat.com> 3.1.90-1
- Update to 3.1.90

* Tue Jul 05 2011 Bastien Nocera <bnocera@redhat.com> 3.1.3-1
- Update to 3.1.3

* Wed Jun 15 2011 Tomas Bzatek <tbzatek@redhat.com> - 3.1.2-1
- Update to 3.1.2

* Thu May 12 2011 Christopher Aillon <caillon@redhat.com> - 3.1.1.1-1
- Update to 3.1.1.1

* Wed May 11 2011 Tomas Bzatek <tbzatek@redhat.com> - 3.1.1-1
- Update to 3.1.1

* Mon Apr  4 2011 Matthias Clasen <mclasen@redhat.com> - 3.0.0-1
- Update to 3.0.0

* Sat Mar 26 2011 Matthias Clasen <mclasen@redhat.com> - 2.99.3-2
- Fix runtime error with > 4 cpus

* Fri Mar 25 2011 Matthias Clasen <mclasen@redhat.com> - 2.99.3-1
- Update to 2.99.3

* Mon Mar 21 2011 Matthias Clasen <mclasen@redhat.com> - 2.99.2-1
- Update to 2.99.2

* Mon Mar  7 2011 Matthias Clasen <mclasen@redhat.com> - 2.99.1-1
- Update to 2.99.1
- Drop 'About this computer', we no longer show it anyway

* Tue Feb 22 2011 Matthias Clasen <mclasen@redhat.com> - 2.99.0-1
- Update to 2.99.0
- Drop accumulated patches

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.28.2-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Wed Sep 29 2010 Matthias Clasen <mclasen@redhat.com> - 2.28.2-1
- Update to 2.28.2

* Tue Mar 30 2010 Matthias Clasen <mclasen@redhat.com> - 2.28.1-1
- Update to 2.28.1
