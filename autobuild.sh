#!/bin/bash
#
# Copyright (C) 2008 Andrew Beekhof
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
set -x
PACKAGE=matahari
VERSION=0.4.0

RPM_OPTS='--define "_sourcedir $(AUTOBUILD_SOURCE_ROOT)" 	\
	  --define "_specdir   $(AUTOBUILD_SOURCE_ROOT)" 	\
	  --define "_srcrpmdir $(AUTOBUILD_SOURCE_ROOT)"	\
'

function make_srpm() {
    VARIANT=$1
    TARFILE=matahari-${VERSION}.tbz2
    TAG=`git show --pretty="format:%h" --abbrev-commit | head -n 1`

    sed -i.sed 's/global\ specversion.*/global\ specversion\ ${AUTO_BUILD_COUNTER}/' ${VARIANT}matahari.spec
    sed -i.sed 's/global\ upstream_version.*/global\ upstream_version\ ${shell git show --pretty="format:%h" | head -n 1}/' ${VARIANT}matahari.spec
    
    rm -f ${TARFILE}
    git archive --prefix=matahari-${VERSION}/ ${TAG} | bzip2 > ${TARFILE}
    echo `date`: Rebuilt ${TARFILE} from ${TAG}
    
    rm -f *.src.rpm
    rpmbuild -bs ${RPM_OPTS} ${VARIANT}matahari.spec
}

env

# Local builds
echo "Installing Linux dependancies..."
sudo yum install -y cmake make libudev-devel gcc-c++ dbus-devel hal-devel qpid-cpp-server-devel qmf-devel pcre-devel glib2-devel sigar-devel

echo "Installing Windows dependancies..."
sudo yum install -y redhat-rpm-config cmake make qmf-devel mingw32-filesystem mingw32-gcc-c++ mingw32-nsis genisoimage mingw32-pcre mingw32-qpid-cpp mingw32-glib2 mingw32-sigar mingw32-srvany

#DESTDIR=$AUTOBUILD_INSTALL_ROOT/linux; export DESTDIR
#mkdir -p $DESTDIR

make check
rc=$?

if [ $rc != 0 ]; then
    exit $rc
fi

make_srpm 
mock --root=`rpm --eval fedora-%{fedora}-%{_arch}` --resultdir=$AUTOBUILD_INSTALL_ROOT --rebuild ${AUTOBUILD_SOURCE_ROOT}/*.src.rpm

rc=$?

if [ $rc != 0 ]; then
    exit $rc
fi


# Need to wait until mingw32-qpid-cpp is in F-14 updates
#make_srpm mingw32-
#mock --root=`rpm --eval fedora-%{fedora}-%{_arch}` --resultdir=$AUTOBUILD_INSTALL_ROOT --rebuild ${AUTOBUILD_SOURCE_ROOT}/*.src.rpm
