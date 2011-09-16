#!/usr/bin/env python

import commands as cmd
from qmf.console import Session
import matahariTest as testUtil
import unittest
import time
import sys
import os

connection = None
resource = None
err = sys.stderr

# Initialization
# =====================================================
def setUp(self):
    global resource
    global connection
    connection = ResourceTestsSetup()
    resource = connection.resource

def tearDown():
    connection.disconnect()
    time.sleep(5)

class ResourceTestsSetup(object):
    def __init__(self):
        cmd.getoutput("yum -y install resource-agent")
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-service start")
        self.expectedMethods = [ 'list_standards()', 'list_providers(standard)', 'list(standard, provider)', 'describe(standard, provider, agent)', 
                                 'invoke(name, standard, provider, agent, action, interval, parameters, timeout, expected-rc, userdata)', 
                                 'cancel(name, action, interval, timeout)', 'fail(name, rc)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[0]
        self.reQuery()
    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)
    def reQuery(self):
        self.resource_objects = self.sess.getObjects(_class='Resources',_package="org.matahariproject")
        self.resource= self.resource_objects[0]
        self.props = self.resource.getProperties()
    def getPropValueByKey(self,key):
        for item in self.props:
            if str(item[0]) == key:
                return item[1]

class TestResourceApi(unittest.TestCase):

    # TEST - getMethods() 
    # ================================================================
    def test_getMethods(self):
        meths = resource.getMethods()
        self.assertEquals(len(meths),len(connection.expectedMethods), "method count not matching")
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

    # TEST - fail()
    # =====================================================
    def test_fail_not_implemented(self):
        result = resource.fail("crond", 0)
        self.assertTrue("Not implemented" in result.text, "text not found, implemented?")

    # TEST - describe()
    # =====================================================
    def test_describe_not_implemented(self):
        result = resource.describe('ocf','heartbeat','IPaddr')
        self.assertTrue("Not implemented" in result.text, "text not found, implemented?")

    # TEST - describe()
    # =====================================================
    #def test_list_standards_empty(self):
        




#def compareList(results, expected):
#    if len(results) != len(expected):
#        m.error("result count not matching expected")
#    for item in (results):
#        try:
#            expected.index(str(item))
#        except:
#            m.error(str(item)+"not expected, new?")

# TEST - list_standards() 
# ================================================================
#m.printHeader('list_standards')
#results = resource.list_standards().outArgs['standards']
#expected = ['ocf', 'lsb', 'windows']
#compareList(results, expected)

# TEST - list_providers() 
# ================================================================
#m.printHeader('list_providers')
#results = resource.list_providers('ocf').outArgs['providers']
#expected = ['heartbeat', 'redhat']
#compareList(results, expected)

# TEST - list() 
# ================================================================
#m.printHeader('list()')
#expected = ['AoEtarget', 'AudibleAlarm', 'CTDB', 'ClusterMon', 'Delay', 'Dummy', 'EvmsSCC', 'Evmsd', 'Filesystem', 'ICP', 'IPaddr', 'IPaddr2', 'IPsrcaddr', 'IPv6addr', 'LVM', 'LinuxSCSI', 'MailTo', 'ManageRAID', 'ManageVE', 'Pure-FTPd', 'Raid1', 'Route', 'SAPDatabase', 'SAPInstance', 'SendArp', 'ServeRAID', 'SphinxSearchDaemon', 'Squid', 'Stateful', 'SysInfo', 'VIPArip', 'VirtualDomain', 'WAS', 'WAS6', 'WinPopup', 'Xen', 'Xinetd', 'anything', 'apache', 'conntrackd', 'db2', 'drbd', 'eDir88', 'ethmonitor', 'exportfs', 'fio', 'iSCSILogicalUnit', 'iSCSITarget', 'ids', 'iscsi', 'jboss', 'lxc', 'mysql', 'mysql-proxy', 'nfsserver', 'nginx', 'oracle', 'oralsnr', 'pgsql', 'pingd', 'portblock', 'postfix', 'proftpd', 'rsyncd', 'scsi2reservation', 'sfex', 'symlink', 'syslog-ng', 'tomcat', 'vmware']
#results = resource.list('ocf', 'heartbeat').outArgs['agents']
#compareList(results, expected)

# TEST - invoke() 
# ================================================================
#m.printHeader('invoke()')
#print "\033[93m[WARN]\033[0m  NO VERIFICATION"

# TEST - cancel() 
# ================================================================
#m.printHeader('cancel()')
#print "\033[93m[WARN]\033[0m  NO VERIFICATION"


