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
VERSION=0.4.4

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

    cp ${VARIANT}matahari.spec.in ${VARIANT}matahari.spec
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

# When qpid is consistently usable 
# build_target=`rpm --eval fedora-%{fedora}-%{_arch}`
build_target=`rpm --eval fedora-rawhide-%{_arch}`

echo "=::=::=::= `date` =::=::=::= "
echo "=::=::=::= Beginning Linux Build =::=::=::= "

make_srpm 
results=$AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/`rpm --eval %{_arch}`

rm -f $results/build.log $results/*.rpm
/usr/bin/mock --root=$build_target --resultdir=$results --rebuild ${PWD}/*.src.rpm
rc=$?

if [ $rc != 0 ]; then
    cat $results/build.log
    echo "=::=::=::= Linux Build Failed =::=::=::= "
    echo "=::=::=::= `date` =::=::=::= "
    exit $rc
fi

# Packages get copied to:
#   /home/builder/matahari/public_html/dist/rpm/ 

$results

echo "=::=::=::= `date` =::=::=::= "
echo "=::=::=::= Beginning Windows Build =::=::=::= "

# When qpid is consistently usable 
# build_target=`rpm --eval fedora-%{fedora}-%{_arch}`
build_target=`rpm --eval fedora-rawhide-%{_arch}`

make_srpm mingw32-
results=$AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/noarch

rm -f $results/build.log $results/*.rpm
/usr/bin/mock --root=$build_target --resultdir=$results --rebuild ${PWD}/*.src.rpm
rc=$?

if [ $rc != 0 ]; then
    cat $results/build.log
    echo "=::=::=::= Windows Build Failed =::=::=::= "
    echo "=::=::=::= `date` =::=::=::= "
    exit $rc
fi
