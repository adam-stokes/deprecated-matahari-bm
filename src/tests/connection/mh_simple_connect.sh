#!/bin/bash
# mh_ssl_simple_connect.sh - Copyright (c) 2011 Red Hat, Inc.
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
. ../commonlib.sh

TEST="/matahari/Sanity/mh_simple_connect"
PACKAGE="matahari"
MATAHARI_BROKER_PORT=`grep -P 'MATAHARI_PORT=\d+' /etc/sysconfig/matahari-broker|cut -f2 -d=`

rlJournalStart
    rlPhaseStartSetup
        rlAssertRpm $PACKAGE
        for i in "${TEST_DEFAULT_SERVICE_NAMES[@]}"
        do
            rlServiceStart $i
        done
    rlPhaseEnd

    rlPhaseStartTest
    rlRun "lsof -i :$MATAHARI_BROKER_PORT | grep -q ':$MATAHARI_BROKER_PORT.*LISTEN'" "0" "Verify broker is listening on $MATAHARI_BROKER_PORT."
    for i in "${TEST_DEFAULT_AGENTS[@]}"
    do
        TEST_PID=$(get_pid_of $i)
        if [ $TEST_PID ]; then
            rlRun "$(is_established $TEST_PID $MATAHARI_BROKER_PORT)" "0" "Verify $i ($TEST_PID) is connected to broker."
        else
            rlFail "Failed to find pid of $i"
        fi
    done
    rlPhaseEnd
    
    rlPhaseStartCleanup
    for i in "${TEST_DEFAULT_SERVICE_NAMES[@]}"
    do
        rlServiceStop $i
    done
    rlPhaseEnd
rlJournalEnd
rlJournalPrintText
