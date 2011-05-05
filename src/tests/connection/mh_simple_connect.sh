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

TEST_HOSTNAME=127.0.0.1
TEST_PORT=49000
TEST_AGENT=matahari-hostd

rlJournalStart
    rlPhaseStartSetup
        rlAssertRpm $PACKAGE
    rlPhaseEnd

    rlPhaseStartTest
        rlRun "$TEST_AGENT --broker $TEST_HOSTNAME --port $TEST_PORT"
    rlPhaseEnd
    
    rlPhaseStartCleanup
    rlPhaseEnd
rlJournalEnd
rlJournalPrintText
