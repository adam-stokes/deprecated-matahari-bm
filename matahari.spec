Summary: Qpid QMF Agent for Hosts
Name: matahari
Version: 0.0.1
Release: 1%{?dist}
Source: matahari-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
License: GPL
Group: Applications/System

Requires: dbus >= 1.2.12
Requires: hal >= 0.5.12
Requires: boost >= 1.37.0
Requires: qpidc >= 0.5.787286
Requires: qmf >= 0.5.787286
Requires: libvirt >= 0.6.2

BuildRequires: gcc-c++ >= 4.4.0
BuildRequires: dbus-devel >= 1.2.12
BuildRequires: hal-devel >= 0.5.12
BuildRequires: boost-devel >= 1.37.0
BuildRequires: qpidc-devel >= 0.5.787286
BuildRequires: qmf-devel >= 0.5.787286
BuildRequires: libvirt-devel >= 0.6.2

%description

matahari provides a QMF Agent that can be used to control and manage
various pieces of functionality for a host system, using the AMQP protocol.

The Advanced Message Queuing Protocol (AMQP) is an open standard application 
layer protocol providing reliable transport of messages.

QMF provides a modeling framework layer on top of qpid (which implements 
AMQP).  This interface allows you to manage a host and its various components as
a set of objects with properties and methods.

%prep
%setup

%build
./configure --with-boost-regex=mt --prefix=$RPM_BUILD_ROOT/usr
make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

%post

%preun

%postun

%clean
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT

%files
%defattr(644, root, root, 644)
%dir %{_datadir}/matahari/
%{_datadir}/matahari/schema.xml

%attr(755, root, root) %{_sbindir}/matahari

%doc AUTHORS COPYING

%changelog

* Tue Jun 23 2009 Arjun Roy <arroy@redhat.com> - 0.0.1-1
- Initial rpmspec packaging

