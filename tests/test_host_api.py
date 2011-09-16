#!/usr/bin/env python

import commands as cmd
from qmf.console import Session
import matahariTest as testUtil
import unittest
import time
import sys
import os

connection = None
host = None
err = sys.stderr

# Initialization
# =====================================================
def setUp(self):
    global host
    global connection
    connection = HostTestsSetup()
    host = connection.host

def tearDown():
    connection.disconnect()
    time.sleep(5)


class HostTestsSetup(object):
    def __init__(self):
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-host start")
        time.sleep(3)
        self.expectedMethods = [ 'identify()', 'shutdown()', 'reboot()', 'get_uuid(lifetime)', 'set_uuid(lifetime, uuid)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[0]
        self.reQuery()
    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)
    def reQuery(self):
        self.hosts = self.sess.getObjects(_class='Host',_package="org.matahariproject")
        self.host = self.hosts[0]
        self.props = self.host.getProperties()
    def getPropValueByKey(self,key):
        for item in self.props:
            if str(item[0]) == key:
                return item[1]

class HostApiTests(unittest.TestCase):
 
    # TEST - getMethods() 
    # ================================================================
    def test_method_count(self):
        meths = host.getMethods()
        self.assertEquals(len(meths),len(connection.expectedMethods), "method count not matching")

    def test_method_list(self): 
        meths = host.getMethods()
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
     
    def test_os_property(self):
        value = connection.getPropValueByKey('os')
        self.assertEquals(value, cmd.getoutput("uname -a | awk '{print $1,\"(\"$3\")\"}'"), "os not matching")

    def test_arch_property(self):
        value = connection.getPropValueByKey('arch')
        self.assertEquals(value, cmd.getoutput("uname -a | awk '{print $12}'"), "os not matching")

    def test_wordsize_property(self):
         value = connection.getPropValueByKey('wordsize')
         word_size = 0
         if cmd.getoutput("uname -a | awk '{print $12}'") == "i686":
             word_size = 32
         elif cmd.getoutput("uname -a | awk '{print $12}'") == "i386":
             word_size = 32
         elif cmd.getoutput("uname -a | awk '{print $12}'") == "x86_64":
             word_size = 64
         self.assertEquals(value, word_size, "wordsize not matching") 

    def test_memory_property(self):
         value = connection.getPropValueByKey('memory')
         self.assertEquals(value, int(cmd.getoutput("free | grep Mem | awk '{ print $2 }'")), "memory not matching")

    def test_swap_property(self):
         value = connection.getPropValueByKey('swap')
         self.assertEquals(value, int(cmd.getoutput("free | grep Swap | awk '{ print $2 }'")), "swap not matching")

    def test_cpu_count_property(self):
         value = connection.getPropValueByKey('cpu_count')
         self.assertEquals(value, int(cmd.getoutput("cat /proc/cpuinfo | grep processor | wc -l")), "cpu count not matching")

    def test_cpu_cores_property(self):
         value = connection.getPropValueByKey('cpu_cores')
         cpu_count = cmd.getoutput("cat /proc/cpuinfo | grep processor | wc -l")
         cores_per_cpu = cmd.getoutput("cat /proc/cpuinfo | grep cores | head -1 | awk -F: '{ print $2 }'").strip()
         if cores_per_cpu == "": cores_per_cpu = 1
         total_core_count = int(cpu_count) * int(cores_per_cpu)
         self.assertEquals(value, total_core_count, "cpu core count not matching")

    def test_cpu_model_property(self):
         value = connection.getPropValueByKey('cpu_model')
         cmdline = cmd.getoutput("cat /proc/cpuinfo | grep 'model name' | head -1 | awk -F: {'print $2'}")
         self.assertTrue(value in cmdline, "cpu model not matching")
         
    def test_cpu_flags_property(self):
         value = connection.getPropValueByKey('cpu_flags')
         self.assertEquals(value, cmd.getoutput("cat /proc/cpuinfo | grep 'flags' | head -1 | awk -F: {'print $2'}").strip(), "cpu flags not matching")

    def test_update_interval_property(self):
         value = connection.getPropValueByKey('update_interval')
         self.assertEquals(value, 5, "update interval not matching")

    def test_last_updated_property(self):
         value = connection.getPropValueByKey('last_updated')
         value = value / 1000000000
         now = int("%.0f" % time.time())
         delta = testUtil.getDelta(value, now)
         self.assertFalse(delta > 5, "last updated off gt 5 seconds")

    #def test_sequence_property(self):
    #     self.fail("no verification")
    
    def test_free_mem_property(self):
         value = connection.getPropValueByKey('free_mem')
         top_value = int(cmd.getoutput("free | grep Mem | awk '{ print $4 }'"))
         self.assertTrue(testUtil.checkTwoValuesInMargin(value, top_value, 0.05, "free memory"), "free memory outside margin")

    def test_free_swap_property(self):
         value = connection.getPropValueByKey('free_swap')
         top_value = int(cmd.getoutput("free | grep Swap | awk '{ print $4 }'"))
         self.assertTrue(testUtil.checkTwoValuesInMargin(value, top_value, 0.05, "free swap"), "free swap outside margin")

    # TEST - get_uuid()
    # =====================================================
    def test_get_uuid_Hardware_lifetime(self):
        result = host.get_uuid('Hardware')
        self.assertNotEqual(result.outArgs['uuid'],'not-available', "not-available text not found on parm 'lifetime'")

    def test_get_uuid_Reboot_lifetime(self):
        result = host.get_uuid('Reboot')
        self.assertNotEqual(result.outArgs['uuid'],'not-available', "not-available text not found on parm 'lifetime'")

    def test_get_uuid_unset_Custom_lifetime(self):
        cmd.getoutput("rm -rf /etc/custom-machine-id")
        testUtil.restartService("matahari-host")
        global connection
        global host
        connection = HostTestsSetup()
        host = connection.host
        result = host.get_uuid('Custom')
        self.assertEqual(result.outArgs['uuid'],'not-available', "not-available text not found on parm 'lifetime'")

    def test_get_uuid_unknown_lifetime(self):
        result = host.get_uuid('lifetime')
        self.assertEqual(result.outArgs['uuid'],'invalid-lifetime', "invalid-lifetime text not found on parm 'lifetime'")

    def test_get_uuid_empty_string(self):
        result = host.get_uuid('')
        self.assertEqual(result.outArgs['uuid'],'invalid-lifetime', "invalid-lifetime text not found on empty string")

    def test_get_uuid_zero_parameters(self):
        try:
            result = host.get_uuid()
            self.fail("no exception on zero parms")
        except Exception as e:
            pass 
    
    # TEST - set_uuid()
    # =====================================================
    def test_set_uuid_Custom_lifetime(self):
        test_uuid = testUtil.getRandomKey(20) 
        host.set_uuid('Custom', test_uuid) 
        result = host.get_uuid('Custom') 
        self.assertEqual(result.outArgs['uuid'], test_uuid, "uuid value not matching expected("+result.outArgs['uuid']+")")
        connection.reQuery()
        self.assertEqual(connection.getPropValueByKey('custom_uuid'), test_uuid, "property not matching set value")
        
    def test_set_uuid_new_Custom_lifetime(self):
        test_uuid = testUtil.getRandomKey(20) 
        host.set_uuid('Custom', test_uuid) 
        result = host.get_uuid('Custom') 
        self.assertEqual(result.outArgs['uuid'], test_uuid, "uuid value not matching expected("+result.outArgs['uuid']+")")
        connection.reQuery()
        self.assertEqual(connection.getPropValueByKey('custom_uuid'), test_uuid, "property not matching set value")

    def test_set_uuid_Hardware_lifetime_fails(self):
        result = host.set_uuid('Hardware', testUtil.getRandomKey(20) ) 
        self.assertEqual(result.outArgs['rc'], 23, "Unexpected return code ("+str(result.outArgs['rc'])+"), expected 23")

    def test_set_uuid_Reboot_lifetime_fails(self):
        result = host.set_uuid('Reboot', testUtil.getRandomKey(20) ) 
        self.assertEqual(result.outArgs['rc'], 23, "Unexpected return code ("+str(result.outArgs['rc'])+"), expected 23")

    # TEST - misc
    # =====================================================
    #def test_identify(self):
    #    self.fail("no verification")

    #def test_reboot(self):
    #    self.fail("no verification")

    #def test_shutdown(self):
    #    self.fail("no verification")

