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

# QMF
# ========================================================
def connectToBroker(hostname, port):
    sess = Session() # defaults to synchronous-only operation. It also defaults to user-management of connections.
    # attempt to connect to a broker
    try:
        broker = sess.addBroker('amqp://'+hostname+':'+port)
        #print "Connection Success"
    except:
        sys.exit(1) 
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

def getRandomKey(length):
    return ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(length))
