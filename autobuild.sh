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
: ${AUTOBUILD_PACKAGE_ROOT:=`pwd`}

function make_srpm() {
    VARIANT=$1

    PACKAGE=matahari
    TAG=`git show --pretty="format:%h" --abbrev-commit | head -n 1`
    TARPREFIX=${PACKAGE}-${PACKAGE}-${TAG}
    TARFILE=${TARPREFIX}.tgz

    sed -i.sed s/global\ specversion.*/global\ specversion\ ${AUTO_BUILD_COUNTER}/ ${VARIANT}matahari.spec
    sed -i.sed s/global\ upstream_version.*/global\ upstream_version\ ${TAG}/ ${VARIANT}matahari.spec
    
    rm -f ${TARFILE}
    git archive --prefix=${TARPREFIX}/ ${TAG} | gzip > ${TARFILE}
    echo `date`: Rebuilt ${TARFILE} from ${TAG}
    
    rm -f *.src.rpm
    rpmbuild -bs --define "_sourcedir ${PWD}" \
		 --define "_specdir   ${PWD}" \
		 --define "_srcrpmdir ${PWD}" ${VARIANT}matahari.spec
}

env

# Until qpid is usable somewhere
#build_target=`rpm --eval fedora-%{fedora}-%{_arch}`
build_target=`rpm --eval fedora-rawhide-%{_arch}`

make_srpm 
/usr/bin/mock --root=$build_target --resultdir=$AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/`rpm --eval %{_arch}` --rebuild ${PWD}/*.src.rpm

rc=$?
cat $AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/x86_64/build.log

if [ $rc != 0 ]; then
    exit $rc
fi

make_srpm mingw32-
/usr/bin/mock --root=$build_target --resultdir=$AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/noarch --rebuild ${PWD}/*.src.rpm

rc=$?
cat $AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/noarch/build.log

if [ $rc != 0 ]; then
    exit $rc
fi
