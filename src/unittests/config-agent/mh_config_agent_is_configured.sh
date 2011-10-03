#!/bin/bash
# mh_config_agent_is_configured.sh - Copyright (c) 2011 Red Hat, Inc.
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

TEST="/matahari/Sanity/mh_config_agent_is_configured"
PACKAGE="matahari-config"

TEST_STORAGE_DIR=/var/lib/matahari/
TEST_CONFIGURED_FILE=.mh_configured
TEST_FULL_CONFIG_PATH="${TEST_STORAGE_DIR}${TEST_CONFIGURED_FILE}"
TEST_QMF_CONSOLE="matahari-qmf-config-consoled"
TEST_QMF_AGENT="matahari-qmf-configd"

rlJournalStart
    rlPhaseStartSetup
        rlAssertRpm $PACKAGE
        rlAssertExists "$TEST_STORAGE_DIR"
        TEST_PID=$(get_pid_of "$TEST_QMF_CONSOLE")
        if [ $TEST_PID ]; then
            rlRun "$(is_established $TEST_PID $MATAHARI_BROKER_PORT)" "0" "Verify $TEST_QMF_CONSOLE ($TEST_PID) is connected to broker."
        else
            rlFail "Failed to find pid of $TEST_QMF_CONSOLE."
        fi
        TEST_PID=$(get_pid_of "$TEST_QMF_AGENT")
        if [ $TEST_PID ]; then
            rlRun "$(is_established $TEST_PID $MATAHARI_BROKER_PORT)" "0" "Verify $TEST_QMF_AGENT ($TEST_PID) is connected to broker."
        else
            rlFail "Failed to find pid of $TEST_QMF_AGENT"
        fi
    rlPhaseEnd

    rlPhaseStartTest
        rlAssertExists "$TEST_FULL_CONFIG_PATH"
    rlPhaseEnd
    
    rlPhaseStartCleanup
    rlPhaseEnd
rlJournalEnd
rlJournalPrintText
