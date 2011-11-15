#!/usr/bin/env python

"""
  test_sysconfig_api.py - Copyright (c) 2011 Red Hat, Inc.
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
import sys
from qmf2 import QmfAgentException
import time
from os import stat
import matahariTest as testUtil
from stat import *
from pwd import getpwuid
from grp import getgrgid
import unittest
import random
import string
import sys
import platform
import os
import threading
import SimpleHTTPServer
import SocketServer
import errno

# The docs for SocketServer show an allow_reuse_address option, but I
# can't seem to make it work, so screw it, randomize the port.
HTTP_PORT = 49002 + random.randint(0, 500)

err = sys.stderr
testPuppetFile = "/sysconfig-puppet-test"
testAugeasFile = "/sysconfig-augeas-test"
testPath = "/tmp/sysconfig-test-http-root"
testPuppetFileWithPath = testPath + testPuppetFile
testAugeasFileWithPath = testPath + testAugeasFile
testPuppetFileUrl = ("http://127.0.0.1:%d" % HTTP_PORT) + testPuppetFile
testAugeasFileUrl = ("http://127.0.0.1:%d" % HTTP_PORT) + testAugeasFile
targetFilePerms = '440'
targetFileGroup = 'root'
targetFileOwner = 'root'
origFilePerms = '777'
origFileGroup = 'qpidd'
origFileOwner = 'qpidd'
puppetFileContents = "file { \""+testPuppetFileWithPath+"\":\n    owner => "+targetFileOwner+", group => "+targetFileGroup+", mode => "+targetFilePerms+"\n}"
augeasQuery = "/files/etc/mtab/1/spec"
augeasFileContents = "get %s\n" % augeasQuery
connection = None
sysconfig = None
httpd_thread = None


def resetTestFile(file, perms, owner, group, contents):
    cmd.getoutput("rm -rf " + file)
    cmd.getoutput("touch " + file)
    cmd.getoutput("chmod "+ perms +" "+ file)
    cmd.getoutput("chown "+ owner +":"+ group +" "+ file)
    testUtil.setFileContents(file, contents)
    #print "++checking test file pre-reqs++"
    if checkFile(file, perms, owner, group) != 0:
        sys.exit("problem setting up test file")
    #print "++DONE...checking test file pre-reqs++"

def wrapper(method, value, flag, schema, key):
    if schema == "augeas":
        resetTestFile(testAugeasFileWithPath, origFilePerms, origFileOwner, origFileGroup, augeasFileContents)
    else:
        resetTestFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup, puppetFileContents)
    results = None
    if method == 'uri':
        results = sysconfig.run_uri(value, flag, schema, key)
    elif method == 'string':
       results = sysconfig.run_string(value, flag, schema, key)
    return results

def checkFile(file, perms, owner, grp):
    count = 0
    if testUtil.getFilePermissionMask(file) != perms:
        err.write('\nfile permissions wrong, '+testUtil.getFilePermissionMask(file)+' != ' +perms+ '\n')
        count = count + 1
    if testUtil.getFileOwner(file) != owner:
        err.write('\nfile owner wrong, '+testUtil.getFileOwner(file)+' != ' +owner+ '\n')
        count = count + 1
    if testUtil.getFileGroup(file) != grp:
        err.write('\nfile group wrong, '+testUtil.getFileGroup(file)+' != ' +grp+ '\n')
        count = count + 1
    return count


class HTTPThread(threading.Thread):
    def run(self):
        try:
            os.makedirs(testPath)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        os.chdir(testPath)
        self.handler = SimpleHTTPServer.SimpleHTTPRequestHandler
        self.httpd = SocketServer.TCPServer(("", HTTP_PORT), self.handler)
        sys.stderr.write("Starting HTTP Server on port: %d ...\n" % HTTP_PORT)
        self.httpd.serve_forever()


# Initialization
# =====================================================
def setUp(self):
    global httpd_thread
    httpd_thread = HTTPThread()
    httpd_thread.start()

    # get puppet pre-req
    if platform.dist()[0] == 'redhat':
        cmd.getoutput("wget -O /etc/yum.repos.d/rhel-aeolus.repo http://repos.fedorapeople.org/repos/aeolus/conductor/rhel-aeolus.repo")
        result = cmd.getstatusoutput("yum -y install puppet")
        if result[0] != 0:
            sys.exit("Unable to install puppet (required for sysconfig tests)")

    # make connection
    global sysconfig
    global connection
    connection = SysconfigSetup()
    sysconfig = connection.sysconfig


def tearDown():
    testUtil.disconnectFromBroker(connection.connect_info)
    global connection
    connection.teardown()
    global httpd_thread
    httpd_thread.httpd.shutdown()
    httpd_thread.httpd.server_close()
    httpd_thread.join()


class SysconfigSetup(object):
    def __init__(self):
        self.broker = testUtil.MatahariBroker()
        self.broker.start()
        time.sleep(3)
        self.sysconfig_agent = testUtil.MatahariAgent("matahari-qmf-sysconfigd")
        self.sysconfig_agent.start()
        time.sleep(3)
        self.expectedMethods = [ 'run_uri(uri, flags, scheme, key)',
                                 'run_string(text, flags, scheme, key)',
                                 'query(text, flags, scheme)',
                                 'is_configured(key)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49001')
        self.sess = self.connect_info[1]
        self.reQuery()

    def teardown(self):
        self.sysconfig_agent.stop()
        self.broker.stop()

    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)

    def reQuery(self):
        self.sysconfig = testUtil.findAgent(self.sess,'Sysconfig','Sysconfig',cmd.getoutput("hostname"))
        self.props = self.sysconfig.getProperties()


class TestSysconfigApi(unittest.TestCase):

    # TEST - getProperties()
    # =====================================================
    def test_hostname_property(self):
        value = connection.props.get('hostname')
        self.assertTrue( value == cmd.getoutput("hostname"), "hostname not expected")

    # TODO:
    #	no puppet
    #	duplicate keys
    #	flags
    #

    # TEST - run_uri()
    # ================================================================
    # TEST - Puppet
    def test_run_uri_good_url_puppet_manifest(self):
        results = wrapper('uri', testPuppetFileUrl, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    def test_run_uri_http_url_not_found_puppet(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testPuppetFileUrl+"_bad", 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_good_file_puppet_manifest(self):
        results = wrapper('uri', 'file://'+testPuppetFileWithPath, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    def test_run_uri_file_url_not_found(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', 'file://'+testPuppetFile, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_bad_puppet_manifest(self):
        resetTestFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup, 'bad puppet script')
        results = sysconfig.run_uri(testPuppetFileUrl, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'FAILED\n1', "result: " + str(results.get('status')) + " != FAILED\n1")
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_non_schema(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testPuppetFileUrl, 0, 'schema', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    # TODO: need to handle upstream vs rhel difference

    # TEST - Augeas
    def test_run_uri_good_url_augeas(self):
        results = wrapper('uri', testAugeasFileUrl, 0, 'augeas', testUtil.getRandomKey(5)).get('status')
        tokens = results.split('\n')
        self.assertEqual(tokens[0], 'OK', "result: %s != OK" % tokens[0])
        self.assertTrue(tokens[1].startswith("%s = " % augeasQuery))

    def test_run_uri_http_url_not_found_augeas(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testAugeasFileUrl + "_bad", 0, 'augeas', testUtil.getRandomKey(5))

    def test_run_uri_good_file_augeas(self):
        results = wrapper('uri', 'file://'+testAugeasFileWithPath, 0, 'augeas', testUtil.getRandomKey(5)).get('status')
        tokens = results.split('\n')
        self.assertEqual(tokens[0], 'OK', "result: %s != OK" % tokens[0])
        self.assertTrue(tokens[1].startswith("%s = " % augeasQuery))

    def test_run_uri_file_url_not_found(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', 'file://'+testAugeasFile, 0, 'augeas', testUtil.getRandomKey(5))


    def test_run_uri_empty_key(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testPuppetFileUrl, 0, 'puppet', '')
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_special_chars_in_key(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testPuppetFileUrl, 0, 'puppet', testUtil.getRandomKey(5) + "'s $$$")
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    # TEST - run_string()
    # ================================================================
    # TEST - Puppet
    def test_run_string_good_puppet_manifest(self):
        results = wrapper('string', testUtil.getFileContents(testPuppetFileWithPath), 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    def test_run_string_bad_puppet_manifest(self):
        results = wrapper('string', 'bad puppet manifest', 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'FAILED\n1', "result: " + str(results.get('status')) + " != FAILED\n1")
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_string_non_schema(self):
        self.assertRaises(QmfAgentException, wrapper, 'string', testUtil.getFileContents(testPuppetFileWithPath), 0, 'schema', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_string_empty_key(self):
        self.assertRaises(QmfAgentException, wrapper, 'string', testUtil.getFileContents(testPuppetFileWithPath), 0, 'puppet', '')
        self.assertTrue( 0 == checkFile(testPuppetFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    # TEST - Augeas
    def test_run_string_good_augeas(self):
        results = wrapper('string', augeasFileContents, 0, 'augeas', testUtil.getRandomKey(5)).get('status')
        tokens = results.split('\n')
        self.assertEqual(tokens[0], 'OK', "result: %s != OK" % tokens[0])
        self.assertTrue(tokens[1].startswith("%s = " % augeasQuery))

    def test_run_string_bad_augeas(self):
        result = wrapper('string', 'bad augeas query', 0, 'augeas', testUtil.getRandomKey(5)).get('status')
        tokens = result.split('\n')
        self.assertEqual(tokens[0], 'FAILED', "result: %s != FAILED" % tokens[0])

    # TEST - query()
    # ================================================================
    def test_query_good_augeas(self):
        result = sysconfig.query(augeasQuery, 0, 'augeas').get('data')
        self.assertNotEqual(result, 'unknown', "result: %s == unknown" % result)

    def test_query_bad_augeas(self):
        result = sysconfig.query('bad augeas query', 0, 'augeas').get('data')
        self.assertEqual(result, 'unknown', "result: %s != unknown" % result)

    # TEST - is_configured()
    # ================================================================
    def test_is_configured_known_key(self):
        key = testUtil.getRandomKey(5)
        wrapper('uri',testPuppetFileUrl, 0, 'puppet', key)
        results = sysconfig.is_configured(key)
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")

    def test_is_configured_unknown_key(self):
        results = sysconfig.is_configured(testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'unknown', "result: " + str(results.get('status')) + " != unknown")

    def test_is_configured_failed_key(self):
        key = testUtil.getRandomKey(5)
        wrapper('string', "bad puppet manifest", 0, 'puppet', key)
        tokens = sysconfig.is_configured(key).get('status').split('\n')
        self.assertTrue(tokens[0] == 'FAILED', "result: %s != FAILED" % tokens[0])
