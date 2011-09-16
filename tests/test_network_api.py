#!/usr/bin/env python

import commands as cmd
from qmf.console import Session
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
    connection.disconnect()
    time.sleep(5)

class NetworkTestsSetup(object):
    def __init__(self):
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-network start")
        time.sleep(3)
        self.expectedMethods = [ 'list()', 'start(iface)', 'stop(iface)', 'status(iface)', 'get_ip_address(iface)', 'get_mac_address(iface)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[0]
        self.reQuery()
        self.nic_ut = self.select_nic_for_test()
    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)
    def reQuery(self):
        self.net_objects = self.sess.getObjects(_class='Network',_package="org.matahariproject")
        self.network = self.net_objects[0]
        self.props = self.network.getProperties()
    def getPropValueByKey(self,key):
        for item in self.props:
            if str(item[0]) == key:
                return item[1]
    def ifconfig_nic_list(self):
        list = cmd.getoutput("ifconfig | grep 'Link encap:' | awk '{print $1}' | tr '\n' '|'").rsplit('|')
        list.pop()
        return list
    def select_nic_for_test(self):
        list = self.ifconfig_nic_list()
        if "eth2" in list:
            return "eth2"
        elif "eth1" in list:
            return "eth1"
        elif "eth0" in list:
            return "eth0"
        else:
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
        
    # TEST - getMethods() 
    # ================================================================
    def test_method_count(self):
        meths = network.getMethods()
        self.assertEquals(len(meths),len(connection.expectedMethods), "method count not matching")

    def test_method_list(self): 
        meths = network.getMethods()
        for meth in meths:
            try:
                connection.expectedMethods.index(str(meth))
            except:
                self.fail(str(meth)+" not expected")

    # TEST - getProperties() 
    # =====================================================
    def test_hostname_property(self):
        value = connection.getPropValueByKey('hostname')
        self.assertEquals(value, cmd.getoutput("hostname"), "hostname not matching") 
     
    # TEST - list()
    # =====================================================
    def test_nic_list_count(self):
        li = network.list()
        found_dict = dict(li.outArgs)
        found_list = found_dict["iface_map"]
        output = cmd.getoutput("cat /proc/net/dev | awk '{print $1}' | grep : | sed 's/^\(.*\):\{1\}.*$/\\1/' | tr '\n' '|'").rsplit('|')
        output.pop()
        self.assertTrue(len(output) == len(found_list), "nic count not matching")

    def test_nic_list(self):
        li = network.list()
        found_dict = dict(li.outArgs)
        found_list = found_dict["iface_map"]
        output = cmd.getoutput("cat /proc/net/dev | awk '{print $1}' | grep : | sed 's/^\(.*\):\{1\}.*$/\\1/' | tr '\n' '|'").rsplit('|')
        output.pop()

        for nic in output:
            try:
                found_list.index(nic)
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
        self.assertTrue(results.outArgs['status'] == 1, "")

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
        self.assertTrue(results.outArgs['status'] == 1, "")

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
        stop_dict = dict(result.outArgs)
        stop_value = stop_dict["status"]
        self.assertTrue(stop_value == 1, "")

    def test_nic_start_status(self):
        nic_ut = connection.nic_ut
        cmd.getoutput("ifup " + nic_ut)
        list = connection.ifconfig_nic_list()
        if nic_ut not in list:
            self.fail("pre-req error: " + nic_ut + " not started for stop test")
        result = network.status(nic_ut)
        start_dict = dict(result.outArgs)
        start_value = start_dict["status"]
        self.assertTrue(start_value == 0, "")

    def test_nic_status_bad_value(self):
        results = network.status("bad")
        self.assertTrue(results.outArgs['status'] == 1, "")

    # TEST - get_ip_address()
    # ================================================================
    def test_get_ip_address(self):
        nic_ut = connection.nic_ut
        result = cmd.getoutput("ifconfig "+nic_ut+" | grep 'inet addr' | gawk -F: '{print $2}' | gawk '{print $1}'")
        gt_ipadd = network.get_ip_address(nic_ut)
        ip_dict = dict(gt_ipadd.outArgs)
        ip_value = ip_dict["ip"]
        if result == "":
            result = "0.0.0.0"
        self.assertTrue(result == ip_value, "")

    def test_get_ip_addr_bad_value(self):
        results = network.get_ip_address("bad")
        self.assertTrue(results.outArgs['ip'] == '', "Bad IP TEST, expecting empty string")

    # TEST - get_mac_address()
    # ================================================================
    def test_get_mac_address(self):
        nic_ut = connection.nic_ut
        mac_add = network.get_mac_address(nic_ut)
        mac_dict = dict(mac_add.outArgs)
        mac_value = mac_dict["mac"]
        output = cmd.getoutput("ifconfig "+nic_ut+" | grep 'HWaddr' | sed 's/^.*HWaddr \(.*\)\{1\}.*$/\\1/'").strip()
        self.assertTrue(output == mac_value, "")

    def test_get_mac_addr_bad_value(self):
        results = network.get_mac_address("bad")
        self.assertTrue(results.outArgs['mac'] == '', "")

