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


def show_sftpserver(dut01):
    dut01.VtyshShell(enter=False)

    #Enable the SFTP server and then verify the show command.
    dut01.DeviceInteract(command="configure terminal")
    devIntReturn = dut01.DeviceInteract(command="sftp server enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enable SFTP server failed"
    dut01.DeviceInteract(command="exit")

    cmdOut = dut01.cmdVtysh(command="show sftp server")
    assert 'Enabled' in cmdOut, "Test to show SFTP server failed"

    return True


def runningConfigTest(dut01):
    dut01.VtyshShell(enter=False)

    #Enable the SFTP server and then verify the show running command.
    dut01.DeviceInteract(command="configure terminal")
    devIntReturn = dut01.DeviceInteract(command="sftp server enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Enable of SFTP server failed"
    dut01.DeviceInteract(command="exit")

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'sftp server enable' in cmdOut, "Test to show SFTP server " \
                                           "in running config failed"

    return True


def noRunningConfigTest(dut01):
    dut01.VtyshShell(enter=False)

    #Enable the SFTP server and check show running command.
    dut01.DeviceInteract(command="configure terminal")
    devIntReturn = dut01.DeviceInteract(command="sftp server enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Enable of SFTP server failed"
    dut01.DeviceInteract(command="exit")

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'sftp server enable' in cmdOut, "Test to show SFTP server " \
                                           "in running config failed"

    #Disable the SFTP server and verify the config is removed
    #in the show running command.
    dut01.DeviceInteract(command="configure terminal")
    devIntReturn = dut01.DeviceInteract(command="no sftp server enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Disable of SFTP server failed"
    dut01.DeviceInteract(command="exit")

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'sftp server enable' not in cmdOut, "Test to show SFTP server " \
                                               "in running config failed"

    return True


def sftpserver_enable(dut01):
    dut01.VtyshShell(enter=False)

    #Enable the SFTP server.
    dut01.DeviceInteract(command="configure terminal")
    devIntReturn = dut01.DeviceInteract(command="sftp server enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enable SFTP server failed"
    dut01.DeviceInteract(command="exit")

    cmdOut = dut01.cmdVtysh(command="show sftp server")
    assert 'SFTP server : Enabled' in cmdOut, "Test to enable SFTP " \
                                              "server failed"

    return True


def sftpserver_disable(dut01):
    dut01.VtyshShell(enter=False)

    #Disable the SFTP server.
    dut01.DeviceInteract(command="configure terminal")
    devIntReturn = dut01.DeviceInteract(command="no sftp server enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to disable SFTP server failed"
    dut01.DeviceInteract(command="exit")

    cmdOut = dut01.cmdVtysh(command="show sftp server")
    assert 'SFTP server : Disabled' in cmdOut, "Test to disable SFTP " \
                                               "server failed"

    return True


class Test_sftpserver_configuration:
    def setup_class(cls):
        # Test object will parse command line and formulate the env.
        Test_sftpserver_configuration.testObj =\
            testEnviron(topoDict=topoDict, defSwitchContext="vtyShell")
        #    Get topology object.
        Test_sftpserver_configuration.topoObj = \
            Test_sftpserver_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_sftpserver_configuration.topoObj.terminate_nodes()

    def test_show_sftp_server(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = show_sftpserver(dut01Obj)
        if(retValue):
            LogOutput('info', "Show SFTP server - passed")
        else:
            LogOutput('error', "Show SFTP server - failed")

    def test_sftpserver_enable(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = sftpserver_enable(dut01Obj)
        if(retValue):
            LogOutput('info', "Enable SFTP server - passed")
        else:
            LogOutput('error', "Enable SFTP server - failed")

    def test_sftpserver_disable(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = sftpserver_disable(dut01Obj)
        if(retValue):
            LogOutput('info', "Disable SFTP server - passed")
        else:
            LogOutput('error', "Disable SFTP server - failed")

    def test_runningConfigTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = runningConfigTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Running config show - passed")
        else:
            LogOutput('error', "Running config show - failed")

    def test_noRunningConfigTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = noRunningConfigTest(dut01Obj)
        if(retValue):
            LogOutput('info', "no form running config show - passed")
        else:
            LogOutput('error', "no form running config show - failed")
