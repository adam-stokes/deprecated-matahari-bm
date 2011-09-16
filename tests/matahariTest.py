#!/usr/bin/env python

import time
import commands as cmd
from qmf.console import Session
import sys
import os
from stat import *
from pwd import getpwuid
from grp import getgrgid
import random
import string

errorCount = 0
testCount = 1

# QMF
# ========================================================
def connectToBroker(hostname, port):
    sess = Session() # defaults to synchronous-only operation. It also defaults to user-management of connections.
    # attempt to connect to a broker
    try:
        broker = sess.addBroker('amqp://'+hostname+':'+port)
        #print "Connection Success"
    except:
        error("Connection Failed")
        sys.exit(errorCount) 
    return [ sess, broker ]

def disconnectFromBroker(list):
    list[0].delBroker(list[1])

def restartService(serviceName):
    cmd.getoutput("service " + serviceName + " restart")

# Files
# ========================================================

def getFilePermissionMask(file):
    return oct(os.stat(file)[ST_MODE])[-3:]

def getFileOwner(file):
    return getpwuid(os.stat(file).st_uid).pw_name

def getFileGroup(file):
    return getgrgid(os.stat(file).st_gid).gr_name

def getFileContents(file):
    f = open(file, 'r')
    return f.read()

def setFileContents(file, content):
    file = open(file, 'w')
    file.write(content)
    file.close

# Other
# =======================================================

def error(msg):
     print "\t\033[91m[ERROR]\033[0m "+ msg +"\n"
     global errorCount
     errorCount += 1

def getDelta(int1, int2):
     delta = 0
     if int1 > int2:
          delta = int1 - int2

     elif int2 > int1:
          delta = int2 - int1
     # else it is even which is a test success
     else:
          delta = 0

     return delta

def checkTwoValuesInMargin(val1, val2, fudgeFactor, description):
    delta = getDelta(val1,val2)
    margin = (val1+100) * fudgeFactor 
    #percent_str = str(fudgeFactor * 100) + "%"
    #delta > margin: error(description + " value gt "+ percent_str + " off (va1:"+str(val1)+" val2:" + str(val2) + ")")
    return delta < margin

def printHeader(method_text):
    #time.sleep(5)
    global testCount
    print '\n\n***************TESTCASE ' + str(testCount) + '*************************'
    print 'Testing method: ' + method_text
    print '***************************************************'
    testCount = testCount + 1

def finish():
    print "\nerrorCount = " + str(errorCount)
    sys.exit(errorCount)

#def notImplementedTest(obj, method_name, parmList):
#    TODO: figure out how to pass the parmList
#    getattr(obj, method_name)('name', 1, 1) 
#    results = obj.
#    print "OUTPUT:",results
#    if "Not implemented" not in results.text:
#        error(method_name + "() result not expected")

def getRandomKey(length):
    return ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(length))

