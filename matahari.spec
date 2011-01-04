%global specversion 21
%global upstream_version b6f91b3

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

Requires:	dbus
Requires:	hal
Requires:	qmf > 0.7
Requires:	pcre

BuildRequires:	cmake
BuildRequires:	libudev-devel
BuildRequires:	gcc-c++
BuildRequires:	dbus-devel
BuildRequires:	hal-devel
BuildRequires:	qpid-cpp-server-devel > 0.7
BuildRequires:	qmf-devel > 0.7
BuildRequires:	pcre-devel
BuildRequires:	glib2-devel
BuildRequires:	sigar-devel

%description

matahari provides a QMF Agent that can be used to control and manage
various pieces of functionality for an ovirt node, using the AMQP protocol.

The Advanced Message Queuing Protocol (AMQP) is an open standard application
layer protocol providing reliable transport of messages.

QMF provides a modeling framework layer on top of qpid (which implements
AMQP).  This interface allows you to manage a host and its various components
as a set of objects with properties and methods.

%package broker
License:	GPLv2+
Summary:	Optional AMQP Broker for Matahari
Group:		Applications/System
Requires:	%{name} = %{version}-%{release}
Requires:	qpid-cpp-server > 0.7
Requires:	qmf > 0.7

%description broker
Optional AMQP Broker for Matahari

%package devel 
License:	GPLv2+
Summary:	Matahari development package
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}
Requires:	qpid-cpp-server-devel > 0.7
Requires:	qmf-devel > 0.7
Requires:	glib2-devel

%description devel
Headers and shared libraries for developing Matahari agents.

%prep
%setup -q

%build
%{cmake} -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%{__install} -d $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d
%{__install} matahari.init   $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/matahari-net
%{__install} matahari.init   $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/matahari-host
%{__install} matahari-broker $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/matahari-broker

%{__install} -d $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/
%{__install} matahari.sysconf $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/matahari
%{__install} matahari-broker.sysconf $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/matahari-broker

%{__install} -d -m0755 %{buildroot}%{_localstatedir}/lib/%{name}
%{__install} -d -m0755 %{buildroot}%{_localstatedir}/run/%{name}

%post
for svc in net host broker; do
    /sbin/chkconfig --level 2345 matahari-$svc on
    /sbin/service matahari-$svc condrestart
done

%preun
if [ $1 = 0 ]; then
    for svc in net host broker; do
       /sbin/service matahari-$svc stop >/dev/null 2>&1 || :
       chkconfig --del matahari-$svc
    done
fi

%postun
if [ "$1" -ge "1" ]; then
    for svc in net host broker; do
        /sbin/service matahari-$svc condrestart >/dev/null 2>&1 || :
    done
fi

%clean
test "x%{buildroot}" != "x" && rm -rf %{buildroot}

%files
%defattr(644, root, root, 755)
%dir %{_datadir}/matahari/
%{_libdir}/libm*.so.*

%config(noreplace) %{_sysconfdir}/sysconfig/matahari

%attr(755, root, root) %{_initddir}/matahari-net
%attr(755, root, root) %{_sbindir}/matahari-netd

%attr(755, root, root) %{_initddir}/matahari-host
%attr(755, root, root) %{_sbindir}/matahari-hostd

%doc AUTHORS COPYING

%files broker
%attr(755, root, root) %{_initddir}/matahari-broker
%config(noreplace) %{_sysconfdir}/sysconfig/matahari-broker
%config(noreplace) %{_sysconfdir}/matahari-broker.conf

%attr(755, qpidd, qpidd) %{_localstatedir}/lib/%{name}
%attr(755, qpidd, qpidd) %{_localstatedir}/run/%{name}

%files devel
%defattr(644, root, root, 755)
%{_datadir}/matahari/schema.xml
%{_includedir}/matahari.h
%{_libdir}/libm*.so

%changelog
* Wed Oct 12 2010 Andrew Beekhof <andrew@beekhof.net> - 0.4.0-0.8.ad8b81b.git
- Added the Network agent
- Removed unnecessary OO-ness from existing Host agent/schema

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
