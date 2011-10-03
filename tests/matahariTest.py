#!/usr/bin/env python

"""
  matahariTest.py - Copyright (c) 2011 Red Hat, Inc.
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

import time
import commands as cmd
import cqpid
from qmf2 import ConsoleSession
import sys
import os
from stat import *
from pwd import getpwuid
from grp import getgrgid
import random
import string

# QMF
# ========================================================
def disconnectFromBroker(connection_info):
    connection_info[1].close()
    connection_info[0].close()

def connectToBroker(hostname, port):
    connection = cqpid.Connection(hostname + ":" + port)
    connection.open()
    session = ConsoleSession(connection)
    session.open()
    return [ connection, session ]

def findAgent(session,agentKeyWord,agentClass,hostname):
    loop_count = 0
    while loop_count < 70:
        agents = session.getAgents()
        for agent in agents:
            if agentKeyWord in str(agent):
                if agent.getAttributes().get('hostname') == hostname:
                    return agent.query("{class:"+agentClass+",package:'org.matahariproject'}")[0]
        time.sleep(1)
        loop_count = loop_count + 1
    sys.exit("specific " + agentKeyWord + " agent for " + hostname + " not found.")

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
