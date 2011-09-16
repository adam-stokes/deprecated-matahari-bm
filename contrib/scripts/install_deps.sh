#!/bin/bash

FEDORA_PACKAGES="cmake pcre-devel glib2-devel qpid-qmf qpid-qmf-devel qpid-cpp-client qpid-cpp-client-devel qpid-cpp-server qpid-cpp-server-devel sigar sigar-devel libcurl libcurl-devel puppet cxxtest help2man dbus-glib dbus-glib-devel polkit polkit-devel"

if [ ! -f /etc/fedora-release ] ; then
    echo "This script only supports Fedora so far."
    exit 1
fi

yum groupinstall "Development Tools"
yum install ${FEDORA_PACKAGES}
