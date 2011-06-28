%global specversion 88
%global upstream_version b7b1733

# Keep around for when/if required
%global alphatag %{upstream_version}.git
%global mh_release %{?alphatag:0.}%{specversion}%{?alphatag:.%{alphatag}}%{?dist}

%bcond_without dbus
%bcond_without qmf

Name:		matahari
Version:	0.4.1
Release:	%{mh_release}
Summary:	Matahari QMF Agents for Linux guests

Group:		Applications/System
License:	GPLv2
URL:		http://fedorahosted.org/matahari

# wget --no-check-certificate -O matahari-matahari-{upstream_version}.tgz https://github.com/matahari/matahari/tarball/{upstream_version}
Source0:	matahari-matahari-%{upstream_version}.tgz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# NOTE: The host API uses dbus for the machine uuid
Requires:	dbus
Requires:	qmf > 0.7
Requires:	pcre

BuildRequires:	cmake
BuildRequires:	libudev-devel
BuildRequires:	gcc-c++
BuildRequires:	pcre-devel
BuildRequires:	glib2-devel
BuildRequires:	sigar-devel

%if %{with qmf}
BuildRequires:	qpid-cpp-client-devel > 0.7
BuildRequires:	qmf-devel > 0.7
%endif

%if %{with dbus}
BuildRequires:	dbus-devel dbus-glib-devel polkit-devel libxslt
%endif

%description

Matahari provides QMF Agents that can be used to control and manage
various pieces of functionality, using the AMQP protocol.

The Advanced Message Queuing Protocol (AMQP) is an open standard application
layer protocol providing reliable transport of messages.

QMF provides a modeling framework layer on top of qpid (which implements
AMQP).  This interface allows you to manage a host and its various components
as a set of objects with properties and methods.


%if %{with qmf}
%package broker
License:	GPLv2+
Summary:	Optional AMQP Broker for Matahari
Group:		Applications/System
Requires:	%{name} = %{version}-%{release}
Requires:	qpid-cpp-server > 0.7
Requires:	qpid-cpp-server-ssl > 0.7
Requires:	qmf > 0.7

%description broker
Optional AMQP Broker for Matahari
%endif

%package lib
License:	GPLv2+
Summary:	C libraries used by Matahari agents
Group:		Applications/System

%description lib
C libraries used by Matahari agents

%package agent-lib
License:	GPLv2+
Summary:	C++ library used by Matahari agents
Group:		Applications/System
Requires:	%{name}-lib = %{version}-%{release}
Requires:	qpid-cpp-client-ssl > 0.7

%description agent-lib
C++ library containing the base class for Matahari agents

%if %{with dbus}
%package dbus
License:	GPLv2+
Summary:	DBus policies for Matahari services
Group:		Applications/System

%description dbus
DBus policies for allowing Matahari to be used on the local system
%endif

%package host
License:	GPLv2+
Summary:	QMF agent for remote hosts
Group:		Applications/System
Requires:	%{name}-lib = %{version}-%{release}
Requires:	%{name}-agent-lib = %{version}-%{release}

%description host
QMF agent for viewing and controlling remote hosts

%package network
License:	GPLv2+
Summary:	QMF agent for network devices  
Group:		Applications/System
Requires:	%{name}-lib = %{version}-%{release}
Requires:	%{name}-agent-lib = %{version}-%{release}
Obsoletes:	matahari-net < %{version}-%{release}

%description network
QMF agent for viewing and controlling network devices  

%package service
License:	GPLv2+
Summary:	QMF agent for system services
Group:		Applications/System
Requires:	%{name}-lib = %{version}-%{release}
Requires:	%{name}-agent-lib = %{version}-%{release}

%description service
QMF agent for viewing and controlling system services

%package postboot
License:	GPLv2+
Summary:	QMF agent for post boot configuration services.
Group:		Applications/System
Requires:	%{name}-lib = %{version}-%{release}
Requires:	%{name}-agent-lib = %{version}-%{release}

%description postboot
QMF agent/console for providing post boot capabilities.

%package devel
License:	GPLv2+
Summary:	Matahari development package
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}
Requires:	%{name}-lib = %{version}-%{release}
Requires:	%{name}-agent-lib = %{version}-%{release}
Requires:	qpid-cpp-client-devel > 0.7
Requires:	qmf-devel > 0.7
Requires:	glib2-devel

%description devel
Headers and shared libraries for developing Matahari agents.

%prep
%setup -q -n matahari-matahari-%{upstream_version}

%build
%{cmake} -DCMAKE_BUILD_TYPE=RelWithDebInfo %{!?with_qmf: -DWITH-QMF:BOOL=OFF} %{!?with_dbus: -DWITH-DBUS:BOOL=OFF} -Dinitdir=%{_initddir} -Dsysconfdir=%{_sysconfdir} .
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%{__install} -d $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/
%{__install} matahari.sysconf $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/matahari

%if %{with qmf}
%{__install} -d $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d
%{__install} matahari-broker $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/matahari-broker

%{__install} matahari-broker.sysconf $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/matahari-broker
%{__ln_s} qpidd $RPM_BUILD_ROOT/%{_sbindir}/matahari-brokerd

%{__install} -d -m0755 %{buildroot}%{_localstatedir}/lib/%{name}
%{__install} -d -m0755 %{buildroot}%{_localstatedir}/run/%{name}
%endif

%post -n matahari-lib -p /sbin/ldconfig
%postun -n matahari-lib -p /sbin/ldconfig

%post -n matahari-agent-lib -p /sbin/ldconfig
%postun -n matahari-agent-lib
# Can't use -p, gives: '/sbin/ldconfig: relative path `0' used to build cache' error
/sbin/ldconfig

%if %{with qmf}
#== Host

%post host
/sbin/service matahari-host condrestart

%preun host
if [ $1 = 0 ]; then
   /sbin/service matahari-host stop >/dev/null 2>&1 || :
   chkconfig --del matahari-host
fi

%postun host
if [ "$1" -ge "1" ]; then
    /sbin/service matahari-host condrestart >/dev/null 2>&1 || :
fi

#== Network

%post network
/sbin/service matahari-network condrestart

%preun network
if [ $1 = 0 ]; then
   /sbin/service matahari-network stop >/dev/null 2>&1 || :
   chkconfig --del matahari-network
fi

%postun network
if [ "$1" -ge "1" ]; then
    /sbin/service matahari-network condrestart >/dev/null 2>&1 || :
fi

#== Services

%post service
/sbin/service matahari-service condrestart

%preun service
if [ $1 = 0 ]; then
   /sbin/service matahari-service stop >/dev/null 2>&1 || :
   chkconfig --del matahari-service
fi

%postun service
if [ "$1" -ge "1" ]; then
    /sbin/service matahari-service condrestart >/dev/null 2>&1 || :
fi

#== Postboot

%post postboot
/sbin/service matahari-postboot condrestart
/sbin/service matahari-postboot-console condrestart

%preun postboot
if [ $1 = 0 ]; then
   /sbin/service matahari-postboot-console stop >/dev/null 2>&1 || :
   /sbin/service matahari-postboot stop >/dev/null 2>&1 || :
   chkconfig --del matahari-postboot-console
   chkconfig --del matahari-postboot
fi

%postun postboot
if [ "$1" -ge "1" ]; then
    /sbin/service matahari-postboot-console condrestart >/dev/null 2>&1 || :
    /sbin/service matahari-postboot condrestart >/dev/null 2>&1 || :
fi

#== Broker

%post broker
/sbin/service matahari-broker condrestart

%preun broker
if [ $1 = 0 ]; then
    /sbin/service matahari-broker stop >/dev/null 2>&1 || :
    chkconfig --del matahari-broker
fi

%postun broker
if [ "$1" -ge "1" ]; then
    /sbin/service matahari-broker condrestart >/dev/null 2>&1 || :
fi

%endif

%clean
test "x%{buildroot}" != "x" && rm -rf %{buildroot}

%files
%defattr(644, root, root, 755)
%doc AUTHORS COPYING

%files agent-lib
%defattr(644, root, root, 755)
%dir %{_datadir}/matahari/
%config(noreplace) %{_sysconfdir}/sysconfig/matahari
%doc AUTHORS COPYING

%if %{with qmf}
%{_libdir}/libmcommon_qmf.so.*
%endif

%if %{with dbus}
%{_libdir}/libmcommon_dbus.so.*
%endif

%files lib
%defattr(644, root, root, 755)
%{_libdir}/libmcommon.so.*
%{_libdir}/libmhost.so.*
%{_libdir}/libmnetwork.so.*
%{_libdir}/libmservice.so.*
%{_libdir}/libmpostboot.so.*
%doc AUTHORS COPYING

%files network
%defattr(644, root, root, 755)
%doc AUTHORS COPYING

%if %{with qmf}
%attr(755, root, root) %{_initddir}/matahari-network
%attr(755, root, root) %{_sbindir}/matahari-qmf-networkd
%endif

%if %{with dbus}
%attr(755, root, root) %{_sbindir}/matahari-dbus-networkd
%endif

%files host
%defattr(644, root, root, 755)
%doc AUTHORS COPYING

%if %{with qmf}
%attr(755, root, root) %{_initddir}/matahari-host
%attr(755, root, root) %{_sbindir}/matahari-qmf-hostd
%endif

%if %{with dbus}
%attr(755, root, root) %{_sbindir}/matahari-dbus-hostd
%endif

%files service
%defattr(644, root, root, 755)
%doc AUTHORS COPYING

%if %{with qmf}
%attr(755, root, root) %{_initddir}/matahari-service
%attr(755, root, root) %{_sbindir}/matahari-qmf-serviced
%attr(755, root, root) %{_sbindir}/matahari-qmf-service-cli
%endif

%if %{with dbus}
%attr(755, root, root) %{_sbindir}/matahari-dbus-serviced
%endif

%files postboot
%defattr(644, root, root, 755)
%doc AUTHORS COPYING

%if %{with qmf}
%attr(755, root, root) %{_initddir}/matahari-postboot
%attr(755, root, root) %{_initddir}/matahari-postboot-console
%attr(755, root, root) %{_sbindir}/matahari-qmf-postbootd
%attr(755, root, root) %{_sbindir}/matahari-qmf-postboot-consoled
%endif

%if %{with qmf}
%files broker
%defattr(644, root, root, 755)
%attr(755, root, root) %{_initddir}/matahari-broker
%config(noreplace) %{_sysconfdir}/sysconfig/matahari-broker
%config(noreplace) %{_sysconfdir}/matahari-broker.conf
%{_sbindir}/matahari-brokerd

%ghost %attr(755, qpidd, qpidd) %{_localstatedir}/lib/%{name}
%ghost %attr(755, qpidd, qpidd) %{_localstatedir}/run/%{name}
%doc AUTHORS COPYING

%else
%exclude %{_sysconfdir}/matahari-broker.conf
%endif

%if %{with dbus}
%files dbus
%{_sysconfdir}/dbus-1/system.d/org.matahariproject.Host.conf
%{_sysconfdir}/dbus-1/system.d/org.matahariproject.Network.conf
%{_sysconfdir}/dbus-1/system.d/org.matahariproject.Services.conf
%{_datadir}/dbus-1/interfaces/org.matahariproject.Host.xml
%{_datadir}/dbus-1/interfaces/org.matahariproject.Network.xml
%{_datadir}/dbus-1/interfaces/org.matahariproject.Services.xml
%{_datadir}/dbus-1/system-services/org.matahariproject.Host.service
%{_datadir}/dbus-1/system-services/org.matahariproject.Network.service
%{_datadir}/dbus-1/system-services/org.matahariproject.Services.service
%{_datadir}/polkit-1/actions/org.matahariproject.Host.policy
%{_datadir}/polkit-1/actions/org.matahariproject.Network.policy
%{_datadir}/polkit-1/actions/org.matahariproject.Services.policy
%endif

%files devel
%defattr(644, root, root, 755)
%doc AUTHORS COPYING

%{_libdir}/libm*.so
%{_includedir}/matahari.h
%{_datadir}/matahari/schema.xml
%{_datadir}/cmake/Modules/FindMatahari.cmake

%if %{with qmf}
%{_includedir}/matahari/mh_agent.h
%{_includedir}/matahari/mainloop.h
%{_datadir}/cmake/Modules/FindQPID.cmake
%endif

%if %{with dbus}
%{_includedir}/matahari/mh_dbus_common.h
%{_includedir}/matahari/mh_gobject_class.h
%{_datadir}/cmake/Modules/MatahariMacros.cmake
%{_datadir}/matahari/schema-to-dbus.xsl
%{_datadir}/matahari/dbus-to-c.xsl
%endif

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
