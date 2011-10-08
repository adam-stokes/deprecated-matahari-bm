#!/usr/bin/env python
"""
  test_service_api.py - Copyright (c) 2011 Red Hat, Inc.
  Written by Dave Johnson <dajo@redhat.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
"""

import commands as cmd
import matahariTest as testUtil
import unittest
import time
import sys
from qmf2 import QmfAgentException

connection = None
service = None
err = sys.stderr

# Initialization
# =====================================================
def setUp(self):
    global service
    global connection
    connection = ServicesTestsSetup()
    service = connection.service

def tearDown():
    connection.teardown()

class ServicesTestsSetup(object):
    def __init__(self):
        self.broker = testUtil.MatahariBroker()
        self.broker.start()
        time.sleep(3)
        self.service_agent = testUtil.MatahariAgent("matahari-qmf-serviced")
        self.service_agent.start()
        time.sleep(3)
        self.expectedMethods = [ 'list()', 'enable(name)', 'disable(name)', 'start(name, timeout)', 'stop(name, timeout)', 'status(name, timeout)', 'describe(name)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49001')
        self.sess = self.connect_info[1]
        self.reQuery()

    def teardown(self):
        testUtil.disconnectFromBroker(self.connect_info)
        self.service_agent.stop()
        self.broker.stop()

    def reQuery(self):
        self.service = testUtil.findAgent(self.sess,'service', 'Services', cmd.getoutput('hostname'))
        self.props = self.service.getProperties()


class TestServicesApi(unittest.TestCase):

    # TEST - getProperties()
    # =====================================================
    def test_hostname_property(self):
        value = connection.props.get('hostname')
        self.assertEquals(value, cmd.getoutput("hostname"), "hostname not matching")
