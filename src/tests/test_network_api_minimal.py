#!/usr/bin/env python
"""
  test_network_api.py - Copyright (c) 2011 Red Hat, Inc.
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
network = None
err = sys.stderr

# Initialization
# =====================================================
def setUp(self):
    global network
    global connection
    connection = NetworkTestsSetup()
    network = connection.network

def tearDown():
    connection.teardown()

class NetworkTestsSetup(object):
    def __init__(self):
        self.broker = testUtil.MatahariBroker()
        self.broker.start()
        time.sleep(3)
        self.network_agent = testUtil.MatahariAgent("matahari-qmf-networkd")
        self.network_agent.start()
        time.sleep(3)
        self.expectedMethods = [ 'list()', 'start(iface)', 'stop(iface)', 'status(iface)', 'get_ip_address(iface)', 'get_mac_address(iface)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49001')
        self.sess = self.connect_info[1]
        self.reQuery()

    def teardown(self):
        testUtil.disconnectFromBroker(self.connect_info)
        self.network_agent.stop()
        self.broker.stop()

    def reQuery(self):
        self.network = testUtil.findAgent(self.sess,'Network', 'Network', cmd.getoutput('hostname'))
        self.props = self.network.getProperties()


class TestNetworkApi(unittest.TestCase):

    # TEST - getProperties()
    # =====================================================
    def test_hostname_property(self):
        value = connection.props.get('hostname')
        self.assertEquals(value, cmd.getoutput("hostname"), "hostname not matching")
