Summary: Qpid QMF Agent for Ovirt Nodes
Name: matahari
Version: 0.0.4
Release: 4%{?dist}
Source: http://arjunroy.fedorapeople.org/matahari/matahari-0.0.4.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
License: GPLv2
Group: Applications/System
URL: http://arjunroy.fedorapeople.org/matahari/index.html

Requires: dbus >= 1.2.12
Requires: hal >= 0.5.12
Requires: qpidc >= 0.5.790661
Requires: qmf >= 0.5.790661
Requires: libvirt >= 0.6.2
Requires: pcre >= 7.8

BuildRequires: gcc-c++ >= 4.4.0
BuildRequires: dbus-devel >= 1.2.12
BuildRequires: hal-devel >= 0.5.12
BuildRequires: qpidc-devel >= 0.5.790661
BuildRequires: qmf-devel >= 0.5.790661
BuildRequires: libvirt-devel >= 0.6.2
BuildRequires: pcre-devel >= 7.8

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
%configure
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%post
/sbin/chkconfig --level 2345 matahari on
/sbin/service matahari condrestart

%preun
if [ $1 = 0 ]; then
    /sbin/service matahari stop >/dev/null 2>&1 || :
    chkconfig --del matahari
fi

%postun
if [ "$1" -ge "1" ]; then
    /sbin/service matahari condrestart >/dev/null 2>&1 || :
fi

%clean
test "x%{buildroot}" != "x" && rm -rf %{buildroot}

%files
%defattr(644, root, root, 755)
%dir %{_datadir}/matahari/
%{_datadir}/matahari/schema.xml

%attr(755, root, root) %{_sbindir}/matahari
%attr(755, root, root) %{_sysconfdir}/rc.d/init.d/matahari
%config(noreplace) %{_sysconfdir}/sysconfig/matahari

%doc AUTHORS COPYING

%changelog

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

