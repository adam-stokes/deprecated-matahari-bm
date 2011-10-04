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

err = sys.stderr
testFile = "/sysconfig-test"
testFileWithPath = "/var/www/html" + testFile
testFileUrl = "http://127.0.0.1" + testFile
targetFilePerms = '440'
targetFileGroup = 'root'
targetFileOwner = 'root'
origFilePerms = '777'
origFileGroup = 'qpidd'
origFileOwner = 'qpidd'
fileContents = "file { \""+testFileWithPath+"\":\n    owner => "+targetFileOwner+", group => "+targetFileGroup+", mode => "+targetFilePerms+"\n}"
connection = None
sysconfig = None

def resetTestFile(file, perms, owner, group, contents):
    cmd.getoutput("rm -rf " + file)
    cmd.getoutput("touch " + file)
    cmd.getoutput("chmod "+ perms +" "+ file)
    cmd.getoutput("chown "+ owner +":"+ group +" "+ file)
    testUtil.setFileContents(file, contents)
    time.sleep(3)
    #print "++checking test file pre-reqs++"
    if checkFile(file, perms, owner, group) != 0:
        sys.exit("problem setting up test file")
    #print "++DONE...checking test file pre-reqs++"

def wrapper(method,value,flag,schema,key):
    resetTestFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup, fileContents)
    results = None
    if method == 'uri':
        results = sysconfig.run_uri(value, flag, schema, key)
    elif method == 'string':
       results = sysconfig.run_string(value, flag, schema, key)
    time.sleep(5)
    return results

def checkFile(file,perms,owner,grp):
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

# Initialization
# =====================================================
def setUp(self):
    # get httpd pre-req
    result = cmd.getstatusoutput("yum -y install httpd")
    if result[0] != 0:
        sys.exit("Unable to install httpd server (required for sysconfig url tests)")
    cmd.getoutput("service httpd start")
    # get puppet pre-req
    if platform.dist()[0] == 'redhat':
        cmd.getoutput("wget -O /etc/yum.repos.d/rhel-aeolus.repo http://repos.fedorapeople.org/repos/aeolus/conductor/0.3.0/rhel-aeolus.repo")
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

class SysconfigSetup(object):
    def __init__(self):
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-sysconfig start")
        time.sleep(3)
        self.expectedMethods = [ 'run_uri(uri, flags, scheme, key)', 'run_string(text, flags, scheme, key)', 'query(text, flags, scheme)', 'is_configured(key)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[1]
        self.reQuery()
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
    def test_run_uri_good_url_puppet_manifest(self):
        results = wrapper('uri',testFileUrl, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")
        self.assertTrue( 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    def test_run_uri_http_url_not_found(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testFileUrl+"_bad", 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_good_file_puppet_manifest(self):
        results = wrapper('uri', 'file://'+testFileWithPath, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")
        self.assertTrue( 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    def test_run_uri_file_url_not_found(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', 'file://'+testFile, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_bad_puppet_manifest(self):
        resetTestFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup, 'bad puppet script')
        results = sysconfig.run_uri(testFileUrl, 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'FAILED\n1', "result: " + str(results.get('status')) + " != FAILED\n1")
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_non_schema(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testFileUrl, 0, 'schema', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    # TODO: need to handle upstream vs rhel difference
    def test_run_uri_augeas_schema_not_implemented(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testFileUrl, 0, 'augeas', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_empty_key(self):
        self.assertRaises(QmfAgentException, wrapper, 'uri', testFileUrl, 0, 'puppet', '')
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_uri_special_chars_in_key(self):
        results = wrapper('uri', testFileUrl, 0, 'puppet', testUtil.getRandomKey(5) + "'s $$$")
        self.assertTrue( results.get('status') == 'unknown', "return code (" + str(results.get('status')) + ") not expected")
        self.assertTrue( 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    # TEST - run_string()
    # ================================================================
    def test_run_string_good_puppet_manifest(self):
        results = wrapper('string', testUtil.getFileContents(testFileWithPath), 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")
        self.assertTrue( 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup), "file properties not expected")

    def test_run_string_bad_puppet_manifest(self):
        results = wrapper('string', 'bad puppet manifest', 0, 'puppet', testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'FAILED\n1', "result: " + str(results.get('status')) + " != FAILED\n1")
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_string_non_schema(self):
        self.assertRaises(QmfAgentException, wrapper, 'string', testUtil.getFileContents(testFileWithPath), 0, 'schema', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_string_augeas_schema_not_implemented(self):
        self.assertRaises(QmfAgentException, wrapper, 'string', testUtil.getFileContents(testFileWithPath), 0, 'augeas', testUtil.getRandomKey(5))
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    def test_run_string_empty_key(self):
        self.assertRaises(QmfAgentException, wrapper, 'string', testUtil.getFileContents(testFileWithPath), 0, 'puppet', '')
        self.assertTrue( 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup), "file properties not expected")

    # TEST - query()
    # ================================================================
    #testUtil.printHeader('query()')
    #print "\033[93m[WARN]\033[0m  NO VERIFICATION"

    # TEST - is_configured()
    # ================================================================
    def	test_is_configured_known_key(self):
        key = testUtil.getRandomKey(5)
        wrapper('uri',testFileUrl, 0, 'puppet', key)
        results = sysconfig.is_configured(key)
        self.assertTrue( results.get('status') == 'OK', "result: " + str(results.get('status')) + " != OK")

    def	test_is_configured_unknown_key(self):
        results = sysconfig.is_configured(testUtil.getRandomKey(5))
        self.assertTrue( results.get('status') == 'unknown', "result: " + str(results.get('status')) + " != unknown")

