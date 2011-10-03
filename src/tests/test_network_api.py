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
    testUtil.disconnectFromBroker(connection.connect_info)

class NetworkTestsSetup(object):
    def __init__(self):
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-network start")
        time.sleep(3)
        self.expectedMethods = [ 'list()', 'start(iface)', 'stop(iface)', 'status(iface)', 'get_ip_address(iface)', 'get_mac_address(iface)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[1]
        self.reQuery()
        self.nic_ut = self.select_nic_for_test()
    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)
    def reQuery(self):
        self.network = testUtil.findAgent(self.sess,'Network', 'Network', cmd.getoutput('hostname'))
        self.props = self.network.getProperties()
    def ifconfig_nic_list(self):
        list = cmd.getoutput("ifconfig | grep 'Link encap:' | awk '{print $1}' | tr '\n' '|'").rsplit('|')
        list.pop()
        return list
    def select_nic_for_test(self):
        nic_list = self.ifconfig_nic_list()
        known_list = ["eth2", "eth1", "eth0", "em2", "em1", "em0"]
        for i in known_list:
            if i in nic_list:
                return i
        print "Error determining NIC for testing."
        sys.exit(1)
            

class TestNetworkApi(unittest.TestCase):

    # TEARDOWN
    # ================================================================
    def tearDown(self):
        list = connection.ifconfig_nic_list() 
        if connection.nic_ut not in list:
            cmd.getoutput("ifup " + connection.nic_ut)
            time.sleep(3)
        
    # TEST - getProperties() 
    # =====================================================
    def test_hostname_property(self):
        value = connection.props.get('hostname')
        self.assertEquals(value, cmd.getoutput("hostname"), "hostname not matching") 
     
    # TEST - list()
    # =====================================================
    def test_nic_list(self):
        result = network.list()
        found_list = result.get("iface_map")
        output = cmd.getoutput("cat /proc/net/dev | awk '{print $1}' | grep : | sed 's/^\(.*\):\{1\}.*$/\\1/' | tr '\n' '|'").rsplit('|')
        output.pop()
        self.assertTrue(len(output) == len(found_list), "nic count not matching")

        for nic in found_list:
            try:
                output.index(nic)
            except:
                self.fail("nic ("+nic+") not found)")
    
    # TEST - start()
    # ================================================================
    def test_nic_start(self):
        nic_ut = connection.nic_ut
        # first, make sure it is stopped
        cmd.getoutput("ifdown " + nic_ut)
        list = connection.ifconfig_nic_list() 
        if nic_ut in list:
            self.fail("pre-req error: " + nic_ut + " not stopping for start test")
        else:
            # do test
            results = network.start(nic_ut)
            #print "OUTPUT:",strt
        # verify up
        time.sleep(3)
        list = connection.ifconfig_nic_list() 
        if nic_ut not in list:
            self.fail(nic_ut + " not stopping for start test")

    def test_nic_start_bad_value(self):
        results = network.start("bad")
        self.assertTrue(results.get('status') == 1, "")

    # TEST - stop()
    # ================================================================
    def test_nic_stop(self):
        nic_ut = connection.nic_ut
        cmd.getoutput("ifup " + nic_ut)
        list = connection.ifconfig_nic_list() 
        if nic_ut not in list:
            self.fail("pre-req error: " + nic_ut + " not started for stop test")
        else:
            # do test
            result = network.stop(nic_ut)
        # verify down 
        time.sleep(3)
        list = connection.ifconfig_nic_list() 
        if nic_ut in list:
            self.fail(nic_ut + " did not stop")

    def test_nic_stop_bad_value(self):
        results = network.stop("bad")
        self.assertTrue(results.get('status') == 1, "")

    # TEST - status()
    # ================================================================
    def test_nic_stop_status(self):
        nut = connection.nic_ut
        cmd.getoutput("ifdown " + nut)
        time.sleep(3)
        list = connection.ifconfig_nic_list()
        if nut in list:
            self.fail("pre-req error: " + nut + " not started for stop test")
        result = network.status(nut)
        stop_value = result.get("status")
        self.assertTrue(stop_value == 1, "")

    def test_nic_start_status(self):
        nic_ut = connection.nic_ut
        cmd.getoutput("ifup " + nic_ut)
        list = connection.ifconfig_nic_list()
        if nic_ut not in list:
            self.fail("pre-req error: " + nic_ut + " not started for stop test")
        result = network.status(nic_ut)
        start_value = result.get("status")
        self.assertTrue(start_value == 0, "")

    def test_nic_status_bad_value(self):
        results = network.status("bad")
        self.assertTrue(results.get('status') == 1, "")

    # TEST - get_ip_address()
    # ================================================================
    def test_get_ip_address(self):
        nic_ut = connection.nic_ut
        output = cmd.getoutput("ifconfig "+nic_ut+" | grep 'inet addr' | gawk -F: '{print $2}' | gawk '{print $1}'")
        result = network.get_ip_address(nic_ut)
        ip_value = result.get("ip")
        if output == "":
            output = "0.0.0.0"
        self.assertTrue(output == ip_value, str(output) + " != " + str(ip_value))

    def test_get_ip_addr_bad_value(self):
        results = network.get_ip_address("bad")
        self.assertTrue(results.get('ip') == '', "Bad IP TEST, expecting empty string")

    # TEST - get_mac_address()
    # ================================================================
    def test_get_mac_address(self):
        nic_ut = connection.nic_ut
        result = network.get_mac_address(nic_ut)
        mac_value = result.get("mac")
        output = cmd.getoutput("ifconfig "+nic_ut+" | grep 'HWaddr' | sed 's/^.*HWaddr \(.*\)\{1\}.*$/\\1/'").strip()
        self.assertTrue(output == mac_value, str(output) + " != " + str(mac_value))

    def test_get_mac_addr_bad_value(self):
        results = network.get_mac_address("bad")
        self.assertTrue(results.get('mac') == '', "Expected empty string")

