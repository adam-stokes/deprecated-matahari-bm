#!/usr/bin/env python

import commands as cmd
from qmf.console import Session
import matahariTest as testUtil
import unittest
import time
import sys
import os
connection = None
service = None
err = sys.stderr

# Initialization
# =====================================================
def setUp(self):
    global service 
    global connection
    connection = ServiceTestsSetup()
    service = connection.service

def tearDown():
    connection.disconnect()
    time.sleep(5)

class ServiceTestsSetup(object):
    def __init__(self):
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-service start")
        self.expectedMethods = [ 'list()', 'enable(name)', 'disable(name)', 'start(name, timeout)', 'stop(name, timeout)', 'status(name, timeout)', 'describe(name)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[0]
        self.test_service = "crond"
        self.reQuery()
        # prepatory cleanup
        if os.path.isfile('/etc/init.d/crond-slow'):
            cmd.getoutput("rm -rf /etc/init.d/crond-slow")
        if not os.path.isfile("/etc/init.d/crond"):
            cmd.getoutput("mv /etc/init.d/crond.orig /etc/init.d/crond")
    def reQuery(self):
        self.service_objects = self.sess.getObjects(_class='Services',_package="org.matahariproject")
        self.service = self.service_objects[0]
        self.props = self.service.getProperties()
    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)
    def getPropValueByKey(self,key):
        for item in self.props:
            if str(item[0]) == key:
                return item[1]

class TestServiceApi(unittest.TestCase):

    # TEST - getMethods() 
    # ================================================================
    def test_getMethods(self):
        meths = service.getMethods()
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

    # TEST - list()
    # =====================================================
    def test_list(self):
        result = service.list()
        dirlist = os.listdir("/etc/init.d")
        dict_list = dict(result.outArgs)
        api_list = dict_list["agents"]
        self.assertTrue(len(api_list) == (len(dirlist)-1), "service count not mataching")
        for svc in dirlist:
            if (svc != 'functions'):
                try: 
                    api_list.index(svc)
                except:
                    self.fail(str(svc) + " service missing")
        
    # TEST - disable()
    # =====================================================
    def test_disable_known_service(self):
        # make sure service enabled
	cmd.getoutput("chkconfig crond on")
        # test
	dis = service.disable("crond")
        # verify
	chk_off = cmd.getoutput("chkconfig --list crond")
	self.assertFalse("2:off" not in chk_off, "run level 2 wrong")
	self.assertFalse("3:off" not in chk_off, "run level 3 wrong")
	self.assertFalse("4:off" not in chk_off, "run level 4 wrong")
	self.assertFalse("5:off" not in chk_off, "run level 5 wrong")

    def test_disable_unknown_service(self):
        t = service.enable("zzzzz")
        self.assertFalse(t.outArgs['rc'] == 0, "Return code not expected (" + str(t.outArgs['rc']) + ")")

    # TEST - enable()
    # =====================================================
    def test_enable_known_service(self):
        # make sure service disabled
        cmd.getoutput("chkconfig crond off")
        # test
        dis = service.enable("crond")
        # verify
        chk_on = cmd.getoutput("chkconfig --list crond")
        self.assertTrue("2:on" in chk_on, "run level 2 wrong")
        self.assertTrue("3:on" in chk_on, "run level 3 wrong")
        self.assertTrue("4:on" in chk_on, "run level 4 wrong")
        self.assertTrue("5:on" in chk_on, "run level 5 wrong")

    def test_enable_unknown_service(self):
        t = service.enable("zzzzz")
        self.assertFalse(t.outArgs['rc'] == 0, "Return code not expected (" + str(t.outArgs['rc']) + ")")

    # TEST - stop()
    # =====================================================
     
    def test_stop_running_known_service(self):
        # pre-req
        cmd.getoutput("service crond start")
        # test
        result = service.stop("crond",10000)
        # verify
        self.assertTrue(result.outArgs['rc'] == 0, "Return code not expected (" + str(result.outArgs['rc']) + ")")
        svc_status = cmd.getoutput("service crond status")
        self.assertTrue("stopped" in svc_status, "text not found, still running?")

    def test_stop_stopped_known_service(self):
        # pre-req
        cmd.getoutput("service crond stop")
        # test
        result = service.stop("crond",10000)
        # verify
        self.assertTrue(result.outArgs['rc'] == 0, "Return code not expected (" + str(result.outArgs['rc']) + ")")
        svc_status = cmd.getoutput("service crond status")
        self.assertTrue("stopped" in svc_status, "text not found, still running?")

    def test_stop_unknown_service(self):
        t = service.stop("zzzzz", 10000)
        self.assertFalse(t.outArgs['rc'] == 0, "Return code not expected (" + str(t.outArgs['rc']) + ")")

    # TEST - start()
    # =====================================================

    def test_start_stopped_known_service(self):
        # pre-req
        cmd.getoutput("service crond stop")
        # test
        result = service.start("crond",10000)
        # verify
        self.assertTrue(result.outArgs['rc'] == 0, "Return code not expected (" + str(result.outArgs['rc']) + ")")
        svc_status = cmd.getoutput("service crond status")
        self.assertTrue("running" in svc_status, "text not found, still running?")

    def test_start_running_known_service(self):
        # pre-req
        cmd.getoutput("service crond start")
        # test
        result = service.start("crond",10000)
        # verify
        self.assertTrue(result.outArgs['rc'] == 0, "Return code not expected (" + str(result.outArgs['rc']) + ")")
        svc_status = cmd.getoutput("service crond status")
        self.assertTrue("running" in svc_status, "text not found, still running?")

    def test_start_unknown_service(self):
        t = service.start("zzzzz", 10000)
        self.assertFalse(t.outArgs['rc'] == 0, "Return code not expected (" + str(t.outArgs['rc']) + ")")

    # TEST - status()
    # =====================================================

    def test_status_stopped_known_service(self):
        # pre-req
        cmd.getoutput("service crond stop")
        # test
        result = service.status("crond",10000)
        # verify
        self.assertTrue(result.outArgs['rc'] == 3, "Return code not expected (" + str(result.outArgs['rc']) + ")")

    def test_status_running_known_service(self):
        # pre-req
        cmd.getoutput("service crond start")
        # test
        result = service.status("crond",10000)
        # verify
        self.assertTrue(result.outArgs['rc'] == 0, "Return code not expected (" + str(result.outArgs['rc']) + ")")

    def test_status_unknown_service(self):
        t = service.status("zzzzz", 10000)
        self.assertFalse(t.outArgs['rc'] == 0, "Return code not expected (" + str(t.outArgs['rc']) + ")")

    # TEST - describe()
    # =====================================================
    def test_describe_not_implemented(self):
        dscrb = service.describe("crond")
        self.assertTrue("Not implemented" in dscrb.text, "text not found, implemented?")

class TestMatahariServiceApiTimeouts(unittest.TestCase):
    def setUp(self):
        cmd.getoutput("mv /etc/init.d/crond /etc/init.d/crond.orig")
        cmd.getoutput("touch /etc/init.d/crond-slow")
        cmd.getoutput("echo 'sleep 10' >> /etc/init.d/crond-slow")
        cmd.getoutput("echo '/etc/init.d/crond.orig $1' >> /etc/init.d/crond-slow")
        cmd.getoutput("chmod 777 /etc/init.d/crond-slow")
        cmd.getoutput("ln -s /etc/init.d/crond-slow /etc/init.d/crond")
    def tearDown(self):
        cmd.getoutput("rm -rf /etc/init.d/crond /etc/init.d/crond-slow")
        cmd.getoutput("mv /etc/init.d/crond.orig /etc/init.d/crond")
    def test_stop_timeout(self):
        result = service.stop("crond",5000)
        self.assertTrue(result.outArgs['rc'] == 198, "not expected rc198, recieved " + str(result.outArgs['rc']))
    def test_start_timeout(self):
        result = service.start("crond",5000)
        self.assertTrue(result.outArgs['rc'] == 198, "not expected rc198, recieved " + str(result.outArgs['rc']))
    def test_status_timeout(self):
        result = service.status("crond",5000)
        self.assertTrue(result.outArgs['rc'] == 198, "not expected rc198, recieved " + str(result.outArgs['rc']))

