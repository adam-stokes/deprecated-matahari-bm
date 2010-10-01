%global specversion 1
%global upstream_version 5e26232

# Keep around for when/if required
%global alphatag %{upstream_version}.git

%global mh_release %{?alphatag:0.}%{specversion}%{?alphatag:.%{alphatag}}%{?dist}

Name:		matahari
Version:	0.4.0
Release:	%{mh_release}
Summary:	Matahari QMF Agents for Linux guests

Group:		Applications/System
License:	GPLv2
URL:		http://fedorahosted.org/matahari
Source0:	matahari-%{version}.tbz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:	dbus >= 1.2.12
Requires:	hal >= 0.5.12
Requires:	qpidc >= 0.5.819819
Requires:	qmf >= 0.5.819819
Requires:	libvirt >= 0.6.2
Requires:	pcre >= 7.8

BuildRequires:	cmake
BuildRequires:	libudev-devel
BuildRequires:	gcc-c++ >= 4.4.0
BuildRequires:	dbus-devel >= 1.2.12
BuildRequires:	hal-devel >= 0.5.12
BuildRequires:	qpidc-devel >= 0.5.819819
BuildRequires:	qmf-devel >= 0.5.819819
BuildRequires:	libvirt-devel >= 0.6.2
BuildRequires:	pcre-devel >= 7.8

%description

matahari provides a QMF Agent that can be used to control and manage
various pieces of functionality for an ovirt node, using the AMQP protocol.

The Advanced Message Queuing Protocol (AMQP) is an open standard application
layer protocol providing reliable transport of messages.

QMF provides a modeling framework layer on top of qpid (which implements
AMQP).  This interface allows you to manage a host and its various components
as a set of objects with properties and methods.

%prep
%setup -q

%build
%{cmake} .
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%{__install} -d $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d
%{__install} matahari.init $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/matahari-host
%{__install} matahari-broker.init $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/matahari-broker

%{__install} -d $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/
%{__install} matahari.sysconf $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/matahari-host
%{__install} matahari-broker.sysconf $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/matahari-broker

%post
/sbin/chkconfig --level 2345 matahari-host on
/sbin/service matahari-host condrestart
/sbin/chkconfig --level 2345 matahari-broker on
/sbin/service matahari-broker condrestart

%preun
if [ $1 = 0 ]; then
    /sbin/service matahari-host stop >/dev/null 2>&1 || :
    chkconfig --del matahari-host
    /sbin/service matahari-broker stop >/dev/null 2>&1 || :
    chkconfig --del matahari-broker
fi

%postun
if [ "$1" -ge "1" ]; then
    /sbin/service matahari-host condrestart >/dev/null 2>&1 || :
    /sbin/service matahari-broker condrestart >/dev/null 2>&1 || :
fi

%clean
test "x%{buildroot}" != "x" && rm -rf %{buildroot}

%files
%defattr(644, root, root, 755)
%dir %{_datadir}/matahari/
%{_datadir}/matahari/schema-host.xml
%{_datadir}/matahari/schema-net.xml

%attr(755, root, root) %{_sbindir}/matahari-hostd
%attr(755, root, root) %{_initddir}/matahari-host
%attr(755, root, root) %{_initddir}/matahari-broker
%config(noreplace) %{_sysconfdir}/sysconfig/matahari-host
%config(noreplace) %{_sysconfdir}/sysconfig/matahari-broker
%config(noreplace) %{_sysconfdir}/matahari-broker.conf

%doc AUTHORS COPYING

%changelog
* Fri Oct 01 2010 Adam Stokes <astokes@fedoraproject.org> - 0.4.0-0.1.5e26232.git
- Add schema-net for network api

* Tue Sep 21 2010 Andrew Beekhof <andrew@beekhof.net> - 0.4.0-0.1.9fc30e4.git
- Pre-release of the new cross platform version of Matahari
- Add matahari broker scripts

* Thu Oct 08 2009 Arjun Roy <arroy@redhat.com> - 0.0.4-7
- Refactored for new version of qpidc.

* Fri Oct 02 2009 Kevin Kofler <Kevin@tigcc.ticalc.org> - 0.0.4-6
- Rebuild for new qpidc.

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.0.4-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Jul 16 2009 Arjun Roy <arroy@redhat.com> - 0.0.4-4
- Changed buildroot value to meet fedora packaging guidelines
- Updated project website

* Mon Jul 13 2009 Arjun Roy <arroy@redhat.com> - 0.0.4-3
- Quietened rpmlint errors and warnings.
- Fixed most gcc warnings.
- Changed init script so it doesn't run by default
- Now rpm specfile makes it so service runs by default instead

* Thu Jul 9 2009 Arjun Roy <arroy@redhat.com> - 0.0.4-2
- Bumped qpidc and qmf version requirements to 0.5.790661.

* Thu Jul 9 2009 Arjun Roy <arroy@redhat.com> - 0.0.4-1
- Removed dependency on boost. Added dependency on pcre.

* Thu Jul 2 2009 Arjun Roy <arroy@redhat.com> - 0.0.3-2
- Fixed bug with not publishing host hypervisor and arch to broker
- Updated aclocal.m4 to match new version of automake

* Tue Jun 30 2009 Arjun Roy <arroy@redhat.com> - 0.0.3-1
- Added getopt and daemonize support
- Added sysV init script support

* Mon Jun 29 2009 Arjun Roy <arroy@redhat.com> - 0.0.2-1
- Now tracks hypervisor and arch using libvirt

* Tue Jun 23 2009 Arjun Roy <arroy@redhat.com> - 0.0.1-1
- Initial rpmspec packaging
