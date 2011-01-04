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
TARFILE=${PACKAGE}-${VERSION}.tbz2

RPM_OPTS='--define "_sourcedir $(AUTOBUILD_SOURCE_ROOT)" 	\
	  --define "_specdir   $(AUTOBUILD_SOURCE_ROOT)" 	\
	  --define "_srcrpmdir $(AUTOBUILD_SOURCE_ROOT)"	\
'

function linux_build() {
    echo "Installing dependancies..."
    sudo yum install -y "cmake make libudev-devel gcc-c++ dbus-devel hal-devel qpid-cpp-server-devel qmf-devel pcre-devel glib2-devel sigar-devel"

    echo "Building for Linux..."
    rm -rf build
    mkdir build
    cd build

    CFLAGS="${CFLAGS:--O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic}" ; export CFLAGS ; 
    CXXFLAGS="${CXXFLAGS:--O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic}" ; export CXXFLAGS ; 
    FFLAGS="${FFLAGS:--O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic}" ; export FFLAGS ; 
    /usr/bin/cmake \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_INSTALL_PREFIX:PATH=/usr \
        -DCMAKE_INSTALL_LIBDIR:PATH=/usr/lib64 \
        -DINCLUDE_INSTALL_DIR:PATH=/usr/include \
        -DLIB_INSTALL_DIR:PATH=/usr/lib64 \
        -DSYSCONF_INSTALL_DIR:PATH=/etc \
        -DSHARE_INSTALL_PREFIX:PATH=/usr/share \
        -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/var/lib/builder/matahari/install-root ..

    #eval "`rpm --eval "%{cmake}"`" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=$AUTOBUILD_INSTALL_ROOT ..
    make all install
    cd ..
}

function windows_build() {
    echo "Installing dependancies..."
    sudo yum install -y "redhat-rpm-config cmake make qmf-devel mingw32-filesystem mingw32-gcc-c++ mingw32-nsis genisoimage mingw32-pcre mingw32-qpid-cpp mingw32-glib2 mingw32-sigar mingw32-srvany"

    echo "Building for Windows..."
    rm -rf build-win
    mkdir build-win
    cd build-win

    eval "`rpm --eval "%{_mingw32_cmake}"`" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$AUTOBUILD_INSTALL_ROOT ..
    make all install
    cd ..
}

function mock_build() {
	sed -i.sed 's/global\ specversion.*/global\ specversion\ ${shell expr 1 + ${lastword ${shell grep "global specversion" ${VARIANT}${PACKAGE}.spec}}}/' ${VARIANT}${PACKAGE}.spec
	sed -i.sed 's/global\ upstream_version.*/global\ upstream_version\ ${firstword ${shell git show --pretty="format: %h"}}/' ${VARIANT}${PACKAGE}.spec

	rm -f ${TARFILE}
	git archive --prefix=${PACKAGE}-${VERSION}/ ${TAG} | bzip2 > ${TARFILE}
	echo `date`: Rebuilt ${TARFILE}

	rm -f *.src.rpm
	rm -rf ${AUTOBUILD_SOURCE_ROOT}/mock
	rpmbuild -bs ${RPM_OPTS} ${VARIANT}${PACKAGE}.spec

	mock --root=${PROFILE} --resultdir=${AUTOBUILD_SOURCE_ROOT}/mock --rebuild ${AUTOBUILD_SOURCE_ROOT}/*.src.rpm
}

#	make PROFILE=matahari VARIANT=mingw32- srpm mock-nodeps

exit_rc=0

env

linux_build
rc=$?

if [ $rc != 0 ]; then
    echo "Linux build failed: $rc"
    exit_rc=$rc
fi

windows_build
rc=$?

if [ $rc != 0 ]; then
    echo "Windows build failed: $rc"
    exit_rc=$rc
fi

exit $exit_rc
