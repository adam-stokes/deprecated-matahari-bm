#!/bin/bash
#
# Copyright (C) 2008 Andrew Beekhof
# Copyrigh (C) 2011, Russell Bryant <rbryant@redhat.com>
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
VERSION=`cat .version`

FEDORA=`rpm --eval %{fedora}`

MOCK=/usr/bin/mock

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

    make ${VARIANT}matahari.spec VARIANT=${VARIANT}
    
    rm -f ${TARFILE}
    git archive --prefix=${TARPREFIX}/ ${TAG} | gzip > ${TARFILE}
    echo `date`: Rebuilt ${TARFILE} from ${TAG}
    
    rm -f *.src.rpm
    rpmbuild -bs --define "_sourcedir ${PWD}" \
		 --define "_specdir   ${PWD}" \
		 --define "_srcrpmdir ${PWD}" ${VARIANT}matahari.spec
}

check_result() {
    RC=$1
    RESULTS=$2
    PLATFORM=$3
    STEP=$4

    if [ ${RC} != 0 ]; then
        cat $results/build.log
        echo "=::=::=::= ${PLATFORM} Build Failed (${STEP}) =::=::=::= "
        echo "=::=::=::= `date` =::=::=::= "
        exit ${RC}
    fi
}

env

build_target=`rpm --eval fedora-%{fedora}-%{_arch}`
#build_target=`rpm --eval fedora-rawhide-%{_arch}`

echo "=::=::=::= `date` =::=::=::= "
echo "=::=::=::= Beginning Linux Build =::=::=::= "

make_srpm 
results=$AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/`rpm --eval %{_arch}`

rm -f $results/build.log $results/*.rpm

${MOCK} -v --root=$build_target --resultdir=$results --init

${MOCK} -v --root=$build_target --resultdir=$results \
    --define="run_unit_tests 1" --no-clean --no-cleanup-after \
    --rebuild ${PWD}/*.src.rpm
check_result $? ${results} "Linux" "build"

if [ "${FEDORA}" = "16" ] ; then
    ${MOCK} -v --root=$build_target --resultdir=$results \
        --install $(ls $results/*.rpm | grep -v vios-proxy)
    check_result $? ${results} "Linux" "install"
else
    ${MOCK} -v --root=$build_target --resultdir=$results \
        --install $results/*.rpm
    check_result $? ${results} "Linux" "install"
fi

. src/tests/deps.sh
${MOCK} -v --root=$build_target --resultdir=$results \
    --install ${MH_TESTS_DEPS}
check_result $? ${results} "Linux" "install test deps"

${MOCK} -v --root=$build_target --resultdir=$results \
    --copyin src/tests /matahari-tests
check_result $? ${results} "Linux" "copy in tests"

${MOCK} -v --root=$build_target --resultdir=$results \
    --shell "nosetests -v /matahari-tests/test_host_api.py"
check_result $? ${results} "Linux" "Host API tests"

${MOCK} -v --root=$build_target --resultdir=$results \
    --shell "nosetests -v /matahari-tests/test_sysconfig_api.py"
check_result $? ${results} "Linux" "Sysconfig API tests"

${MOCK} -v --root=$build_target --resultdir=$results \
    --shell "nosetests -v /matahari-tests/test_resource_api.py"
check_result $? ${results} "Linux" "Resources API tests"

${MOCK} -v --root=$build_target --resultdir=$results \
    --shell "nosetests -v /matahari-tests/test_service_api_minimal.py"
check_result $? ${results} "Linux" "Services API tests"

${MOCK} -v --root=$build_target --resultdir=$results \
    --shell "nosetests -v /matahari-tests/test_network_api_minimal.py"
check_result $? ${results} "Linux" "Network API tests"

# Packages get copied to:
#   /home/builder/matahari/public_html/dist/rpm/ 

ls $results

# Disabled windows builds for now.  The mingw toolchain is busted.
exit 0

echo "=::=::=::= `date` =::=::=::= "
echo "=::=::=::= Beginning Windows Build =::=::=::= "

build_target=`rpm --eval fedora-%{fedora}-%{_arch}`
#build_target=`rpm --eval fedora-rawhide-%{_arch}`

make_srpm mingw32-
results=$AUTOBUILD_PACKAGE_ROOT/rpm/RPMS/noarch

rm -f $results/build.log $results/*.rpm
/usr/bin/mock --root=$build_target --resultdir=$results --rebuild ${PWD}/*.src.rpm
check_result $? $results "Windows" "build"
