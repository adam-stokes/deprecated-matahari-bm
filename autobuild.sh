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

: ${AUTO_BUILD_COUNTER:="custom"}
: ${AUTOBUILD_SOURCE_ROOT:=`pwd`}
: ${AUTOBUILD_INSTALL_ROOT:=`pwd`}

function make_srpm() {
    VARIANT=$1
    TARFILE=matahari-${VERSION}.tbz2
    TAG=`git show --pretty="format:%h" --abbrev-commit | head -n 1`

    sed -i.sed s/global\ specversion.*/global\ specversion\ ${AUTO_BUILD_COUNTER}/ ${VARIANT}matahari.spec
    sed -i.sed s/global\ upstream_version.*/global\ upstream_version\ ${TAG}/ ${VARIANT}matahari.spec
    
    rm -f ${TARFILE}
    git archive --prefix=matahari-${VERSION}/ ${TAG} | bzip2 > ${TARFILE}
    echo `date`: Rebuilt ${TARFILE} from ${TAG}
    
    rm -f *.src.rpm
    rpmbuild -bs --define "_sourcedir ${AUTOBUILD_SOURCE_ROOT}" \
		 --define "_specdir  ${AUTOBUILD_SOURCE_ROOT}"  \
		 --define "_srcrpmdir ${AUTOBUILD_SOURCE_ROOT}" ${VARIANT}matahari.spec
}

env

make_srpm 
mock --root=`rpm --eval fedora-%{fedora}-%{_arch}` --resultdir=$AUTOBUILD_INSTALL_ROOT --rebuild ${AUTOBUILD_SOURCE_ROOT}/*.src.rpm

rc=$?
cat $AUTOBUILD_INSTALL_ROOT/build.log

if [ $rc != 0 ]; then
    exit $rc
fi

make_srpm mingw32-
mock --root=`rpm --eval fedora-%{fedora}-%{_arch}` --resultdir=$AUTOBUILD_INSTALL_ROOT --rebuild ${AUTOBUILD_SOURCE_ROOT}/*.src.rpm

rc=$?
cat $AUTOBUILD_INSTALL_ROOT/build.log

if [ $rc != 0 ]; then
    exit $rc
fi
