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

TEST="/matahari/Sanity/mh_simple_connect"
PACKAGE="matahari"

TEST_DEFAULT_AGENTS=( matahari-qmf-hostd \
                         matahari-qmf-networkd \
                         matahari-qmf-serviced \
                         matahari-qmf-configd  )
                             
MATAHARI_BROKER_PORT=`grep -P 'MATAHARI_PORT=\d+' /etc/sysconfig/matahari-broker|cut -f2 -d=`

rlJournalStart
    rlPhaseStartSetup
        rlAssertRpm $PACKAGE
    rlPhaseEnd

    rlPhaseStartTest
    rlRun "lsof -i :$MATAHARI_BROKER_PORT | grep -q ':$MATAHARI_BROKER_PORT.*LISTEN'" "0" "Verify broker is listening on $MATAHARI_BROKER_PORT."
    for i in "${TEST_DEFAULT_AGENTS[@]}"
    do
        rlRun "ps -ef|grep -v grep|grep -q $i" "0" "Verifying $i is running."
        TEST_PID=`ps -ef|grep $i|grep -v grep|cut -f4 -d' '`
        if [ $TEST_PID ]; then
            rlRun "lsof -p $TEST_PID |grep -q '$MATAHARI_BROKER_PORT.*ESTABLISHED'" "0" "Verify $i is connected to broker."
        fi
    done
    rlPhaseEnd
    
    rlPhaseStartCleanup
        unset $TEST_PID
    rlPhaseEnd
rlJournalEnd
rlJournalPrintText
