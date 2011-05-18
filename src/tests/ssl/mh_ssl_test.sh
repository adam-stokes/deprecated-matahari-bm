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

. /usr/share/beakerlib/beakerlib.sh

TEST="/matahari/Sanity/mh_ssl_test"
PACKAGE="matahari"

QPIDC_CONF_FILE=/etc/qpid/qpidc.conf
CERT_DIR=test_cert_db
CERT_PW_FILE=cert.password
TEST_HOSTNAME=127.0.0.1
TEST_CLIENT_CERT=agent

export QPID_SSL_CERT_DB=${CERT_DIR}
export QPID_SSL_CERT_PASSWORD_FILE=${CERT_PW_FILE}

rlJournalStart
    rlPhaseStartSetup
        rlAssertRpm $PACKAGE
        rlAssertRpm nss-tools
        rlAssertGrep "ssl\-cert\-db=" "$QPIDC_CONF_FILE"
        rlAssertGrep "ssl\-cert\-password\-file=" "$QPIDC_CONF_FILE"
        rlAssertGrep "ssl\-cert\-name=" "$QPIDC_CONF_FILE"
        rlRun "TmpDir=\`mktemp -d\` " 0 "Creating tmp directory"
        rlRun "pushd $TmpDir"
        rlRun "mkdir $CERT_DIR"
        rlAssertExists "$QPIDC_CONF_FILE"
        rlRun "echo password > $CERT_PW_FILE" 0 "Creating password file"
        rlAssertExists "$CERT_PW_FILE"
        rlRun "certutil -N -d $CERT_DIR -f $CERT_PW_FILE" 0 "Creating certificate database"
        rlAssertExists "$CERT_DIR/cert8.db"
        rlRun "certutil -S -d $CERT_DIR -n $TEST_HOSTNAME -s 'CN=$TEST_HOSTNAME' -t 'CT,,' -x -f $CERT_PW_FILE -z /usr/bin/certutil" 0 "Creating broker certificate"
        rlRun "certutil -L -d $CERT_DIR -n $TEST_HOSTNAME"
        rlRun "certutil -S -d $CERT_DIR -n $TEST_CLIENT_CERT -s 'CN=$TEST_CLIENT_CERT' -t 'CT,,' -x -f $CERT_PW_FILE -z /usr/bin/certutil" 0 "Creating agent certficiate"
        rlRun "certutil -L -d $CERT_DIR -n $TEST_CLIENT_CERT"
    rlPhaseEnd

    rlPhaseStartTest
        rlServiceStart matahari-broker matahari-config matahari-config-console
    rlPhaseEnd
    
    rlPhaseStartCleanup
        rlRun "popd"
        rlRun "rm -r $TmpDir" 0 "Removing tmp directory"
    rlPhaseEnd
rlJournalEnd
rlJournalPrintText
