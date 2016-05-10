#!/usr/bin/env python

# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import pytest
import re
from opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *

# Topology definition
topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01",
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}


def enterConfigShell(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    return True


def sftpclient_user_limit(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    LogOutput('info', "Test SFTP client username limit")
    MAX_USERNAME_LEN = 256
    username = 'u'
    srcpath = ' /etc/ssh/sshd_config'
    dstpath = ' /home/admin/'

    for x in range(1, MAX_USERNAME_LEN+1):
        username = username+'u'

    copy = "copy sftp "
    hostip = ' 127.0.0.1'
    cmd = copy+username+hostip+srcpath+dstpath

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to check the username limit succeded"

    cmdOut = dut01.cmdVtysh(command=cmd)
    assert 'Username should be less than 256 characters' \
           in cmdOut, "Test to check username failed"

    return True


def sftpclient_host_limit(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    LogOutput('info', "Test SFTP client hostname limit")
    MAX_HOSTNAME_LEN = 256
    hostname = ' h'
    username = "root"
    srcpath = ' /etc/ssh/sshd_config'
    dstpath = ' /home/admin/'
    copy = "copy sftp "

    for y in range(1, MAX_HOSTNAME_LEN+1):
        hostname = hostname+'h'

    cmd = copy+username+hostname+srcpath+dstpath

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to check the hostname limit succeded"

    cmdOut = dut01.cmdVtysh(command=cmd)
    assert 'Hostname should be less than 256 characters' \
           in cmdOut, "Test to check hostname failed"

    return True


def sftpclient_non_admin_user(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    LogOutput('info', "Test SFTP client for a netop user")
    hostname = ' 127.0.0.1'
    username = "root"
    srcpath = ' /etc/ssh/sshd_config'
    dstpath = ' /home/'
    copy = "copy sftp "

    # Login to the device as a netop user
    cmd = "start-shell"
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter the bash shell"

    cmd = "su - netop"
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter vtysh as a netop user"

    # non-interactive sftp client
    cmd = copy+username+hostname+srcpath+dstpath
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to check the non-admin user failed"

    cmdOut = dut01.cmdVtysh(command=cmd)
    assert 'ERROR :This user has no authorisation to ' \
           'execute this command' \
           in cmdOut, "Test to verify sftp client with non-admin user failed"

    # interactive sftp client
    cmd = copy+username+hostname
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to check the non-admin user failed"

    cmdOut = dut01.cmdVtysh(command=cmd)
    assert 'ERROR :This user has no authorisation to ' \
           'execute this command' \
           in cmdOut, "Test to verify sftp client with non-admin user failed"

    dut01.cmdVtysh(command="exit")
    dut01.cmdVtysh(command="exit")

    return True


class Test_sftpclient_configuration:
    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_sftpclient_configuration.testObj =\
            testEnviron(topoDict=topoDict, defSwitchContext="vtyShell")
        #    Get topology object
        Test_sftpclient_configuration.topoObj = \
            Test_sftpclient_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_sftpclient_configuration.topoObj.terminate_nodes()

    def test_sftpclient_user_limit(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = sftpclient_user_limit(dut01Obj)
        if(retValue):
            LogOutput('info', "Test SFTP client username max limit - passed")
        else:
            LogOutput('error', "Test SFTP client username max limit - failed")

    def test_sftpclient_host_limit(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = sftpclient_host_limit(dut01Obj)
        if(retValue):
            LogOutput('info', "Test SFTP client hostname max limit - passed")
        else:
            LogOutput('error', "Test SFTP client hostname max limit - failed")

    def test_sftpclient_non_admin_user(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = sftpclient_non_admin_user(dut01Obj)
        if(retValue):
            LogOutput('info',
                      "Test SFTP client for a non-admin user - passed")
        else:
            LogOutput('error',
                      "Test SFTP client for a non-admin user - failed")
