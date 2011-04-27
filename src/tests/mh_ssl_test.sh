#!/bin/bash
# mh_ssl_test.sh - Copyright (c) 2011 Red Hat, Inc.
# Written by Adam Stokes <astokes@fedoraproject.org>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

source ./mh_common.sh

CERT_DIR=`pwd`/test_cert_db
CERT_PW_FILE=`pwd`/cert.password
TEST_CLIENT_CERT=agent
COUNT=10

trap cleanup EXIT

error() { echo $*; exit 1; }

create_certs() {
    #create certificate and key databases with single, simple, self-signed certificate in it
    mkdir ${CERT_DIR}
    certutil -N -d ${CERT_DIR} -f ${CERT_PW_FILE}
    certutil -S -d ${CERT_DIR} -n ${TEST_HOSTNAME} -s "CN=${TEST_HOSTNAME}" -t "CT,," -x -f ${CERT_PW_FILE} -z /usr/bin/certutil
    certutil -S -d ${CERT_DIR} -n ${TEST_CLIENT_CERT} -s "CN=${TEST_CLIENT_CERT}" -t "CT,," -x -f ${CERT_PW_FILE} -z /usr/bin/certutil
}

delete_certs() {
    if [[ -e ${CERT_DIR} ]] ;  then
        rm -rf ${CERT_DIR}
    fi
}

cleanup() {
    delete_certs
}

CERTUTIL=$(type -p certutil)
if [[ !(-x $CERTUTIL) ]] ; then
    echo "No certutil, skipping ssl test";
    exit 0;
fi

if [[ !(-e ${CERT_PW_FILE}) ]] ;  then
    echo password > ${CERT_PW_FILE}
fi

create_certs
