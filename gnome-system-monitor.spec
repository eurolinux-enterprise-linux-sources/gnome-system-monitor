# Note that this is NOT a relocatable package

%define libgtop2_version 2.28.2
%define libwnck_version 2.91.0
%define desktop_file_utils_version 0.2.90
%define libselinux_version 1.23.2
%define polkit_version 0.92

Summary: Process and resource monitor
Name: gnome-system-monitor
Version: 3.14.1
Release: 4%{?dist}
License: GPLv2+
Group: Applications/System
URL: http://www.gnome.org/
#VCS: git:git://git.gnome.org/gnome-system-monitor
Source: http://download.gnome.org/sources/gnome-system-monitor/3.14/%{name}-%{version}.tar.xz

Patch0: 0001-Fix-Ukrainian-translation-for-desktop-file-keywords.patch
Patch1: 0001-Fix-desktop-file-keywords-translation-syntax-error.patch
Patch2: gnome-system-monitor-3.14.1-high-n-cpus.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1254332
Patch3: gnome-system-monitor-3.14.1-improve-ncpu-detection.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=1272383
Patch4: gnome-system-monitor-3.14.1-translations.patch

BuildRequires: libgtop2-devel >= %{libgtop2_version}
BuildRequires: libwnck3-devel >= %{libwnck_version}
BuildRequires: gtk3-devel
BuildRequires: gtkmm30-devel
BuildRequires: desktop-file-utils
BuildRequires: startup-notification-devel
BuildRequires: intltool gettext
BuildRequires: libselinux-devel >= %{libselinux_version}
BuildRequires: pcre-devel
BuildRequires: systemd-devel
BuildRequires: librsvg2-devel
BuildRequires: libxml2-devel
BuildRequires: itstool

%description
gnome-system-monitor allows to graphically view and manipulate the running
processes on your system. It also provides an overview of available resources
such as CPU and memory.

%prep
%setup -q
%patch0 -p1
%patch1 -p1
%patch2 -p1 -b .CPUs
%patch3 -p1 -b .nCPUs
%patch4 -p1 -b .translations

%build
%configure --enable-systemd
make %{?_smp_mflags}

%install
make install DESTDIR=$RPM_BUILD_ROOT

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
%doc AUTHORS NEWS COPYING README
%{_bindir}/gnome-system-monitor
%{_datadir}/appdata/gnome-system-monitor.appdata.xml
%{_datadir}/applications/gnome-system-monitor.desktop
%{_datadir}/applications/gnome-system-monitor-kde.desktop
%{_datadir}/glib-2.0/schemas/org.gnome.gnome-system-monitor.enums.xml
%{_datadir}/glib-2.0/schemas/org.gnome.gnome-system-monitor.gschema.xml
%{_datadir}/polkit-1/actions/org.gnome.gnome-system-monitor.policy
%{_libexecdir}/gnome-system-monitor/gsm-*

%changelog
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
