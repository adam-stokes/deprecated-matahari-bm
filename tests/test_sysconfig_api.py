#!/usr/bin/env python

import commands as cmd
import sys
from qmf.console import Session
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
    cmd.getoutput("yum -y install httpd")
    cmd.getoutput("service httpd start")
    global sysconfig
    global connection
    connection = SysconfigSetup()
    sysconfig = connection.sysconfig

def tearDown():
    connection.disconnect()
    time.sleep(5)
        
class SysconfigSetup(object):
    def __init__(self):
        cmd.getoutput("service matahari-broker start")
        cmd.getoutput("service matahari-sysconfig start")
        time.sleep(3)
        self.expectedMethods = [ 'run_uri(uri, flags, scheme, key)', 'run_string(text, flags, scheme, key)', 'query(text, flags, scheme)', 'is_configured(key)' ]
        self.connect_info = testUtil.connectToBroker('localhost','49000')
        self.sess = self.connect_info[0]
        self.reQuery()
    def disconnect(self):
        testUtil.disconnectFromBroker(self.connect_info)
    def reQuery(self):
        self.sysconfigs = self.sess.getObjects(_class='Sysconfig',_package="org.matahariproject")
        self.sysconfig = self.sysconfigs[0]
        self.props = self.sysconfig.getProperties()
    def getPropValueByKey(self,key):
        for item in self.props:
            if str(item[0]) == key:
                return item[1]

class TestSysconfigApi(unittest.TestCase):

    # TEST - getMethods() 
    # ================================================================
    def test_available_methods(self):
        meths = sysconfig.getMethods()
        assert len(meths) == len(connection.expectedMethods)
        for meth in meths:
            try:
                connection.expectedMethods.index(str(meth))
            except:
                self.fail(str(meth)+" not expected")
                

    # TEST - getProperties() 
    # =====================================================
    def test_hostname_property(self):
        value = connection.getPropValueByKey('hostname')
        assert value == cmd.getoutput("hostname")

    # TODO:
    #	no puppet
    #	duplicate keys
    #	flags	
    #	

    # TEST - run_uri() 
    # ================================================================
    def test_run_uri_good_url_puppet_manifest(self):
        results = wrapper('uri',testFileUrl, 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup)

    def test_run_uri_http_url_not_found(self):
        results = wrapper('uri',testFileUrl+"_bad", 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    def test_run_uri_good_file_puppet_manifest(self):
        results = wrapper('uri', 'file:///'+testFileWithPath, 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup)

    def test_run_uri_file_url_not_found(self):
        results = wrapper('uri', 'file:///'+testFile, 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    def test_run_uri_bad_puppet_manifest(self):
        resetTestFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup, 'bad puppet script')
        results = sysconfig.run_uri('file:///'+testFileWithPath, 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    def test_run_uri_non_schema(self):
        results = wrapper('uri', 'file:///'+testFileWithPath, 0, 'schema', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    def test_run_uri_augeas_schema_not_implemented(self):
        results = wrapper('uri', 'file:///'+testFileWithPath, 0, 'augeas', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    #def test_run_uri_empty_key(self):
    #    results = wrapper('uri', 'file:///'+testFileWithPath, 0, 'puppet', '')
    #    assert results.status == 0
    #    assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    #def test_run_uri_special_chars_in_key(self):
    #    results = wrapper('uri', 'file:///'+testFileWithPath, 0, 'puppet', "dave's $$$")
    #    assert results.status == 0
    #    assert 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup)

    # TEST - run_string() 
    # ================================================================
    def test_run_string_good_puppet_manifest(self):
        results = wrapper('string', testUtil.getFileContents(testFileWithPath), 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, targetFilePerms, targetFileOwner, targetFileGroup)

    def test_run_string_bad_puppet_manifest(self):
        results = wrapper('string', 'bad puppet manifest', 0, 'puppet', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    def test_run_string_non_schema(self):
        results = wrapper('string', testUtil.getFileContents(testFileWithPath), 0, 'schema', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    def test_run_string_augeas_schema_not_implemented(self):
        results = wrapper('string', testUtil.getFileContents(testFileWithPath), 0, 'augeas', testUtil.getRandomKey(5))
        assert results.status == 0
        assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

    #def test_run_string_empty_key(self):
    #    resetTestFile()
    #    results = wrapper('string', testUtil.getFileContents(testFileWithPath), 0, 'puppet', '')
    #    assert results.status == 0
    #    assert 0 == checkFile(testFileWithPath, origFilePerms, origFileOwner, origFileGroup)

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
        assert results.outArgs['status'] == 'OK'

    def	test_is_configured_unknown_key(self):
        results = sysconfig.is_configured(testUtil.getRandomKey(5))
        assert results.outArgs['status'] == 'unknown'

