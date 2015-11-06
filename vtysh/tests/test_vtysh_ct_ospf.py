#!/usr/bin/python

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

import pytest
import re
from opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *

# Topology definition
topoDict = {"topoExecution": 5000,
            "topoTarget": "dut01 dut02",
            "topoDevices": "dut01 dut02",
            "topoLinks": "lnk01:dut01:dut02",
            "topoFilters": "dut01:system-category:switch,\
                            dut02:system-category:switch"}


def enterConfigShell(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    return True


def enterRouterContext(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    devIntReturn = dut01.DeviceInteract(command="router ospf")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter OSPF context"

    return True


def enterInterfaceContext(dut01, interface, enable):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    cmd = "interface " + str(interface)
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    if (enable is True):
        dut01.DeviceInteract(command="no shutdown")
        dut01.DeviceInteract(command="no routing")

    return True


def exitContext(dut01):
    devIntReturn = dut01.DeviceInteract(command="exit")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to exit current context"

    retStruct = dut01.ConfigVtyShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit config terminal"

    retStruct = dut01.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"

    return True


def enableOSPFFeatureTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ospf enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enable OSPF feature failed"

    return True


def disableOSPFFeatureTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ospf enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to disable OSPF feature failed"

    return True


def createRouterInstanceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="router ospf")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enter OSPF context failed"

    return True


def deleteRouterInstanceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router ospf")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to delete OSPF context failed"

    return True


def enableRouterInstanceTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enable OSPF router instance failed"

    return True


def disableRouterInstanceTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to disable OSPF router instance failed"
    return True


def setRouterIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="router-id 1.1.1.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set router id failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip ospf")
    assert '1.1.1.1' in cmdOut, "Test to set router id failed"

    return True


def unsetRouterIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router-id")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset router id failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip ospf")
    assert '1.1.1.1' not in cmdOut, "Test to unset router id failed"
    return True


def setNetworkAreaIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="network 10.0.0.0/24 area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set network area id failed"

    return True


def unsetNetworkAreaIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no network 10.0.0.0/24 area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network area id failed"

    return True


def runningConfigTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="router-id 1.2.3.4")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set router id failed"

    devIntReturn = dut01.DeviceInteract(command="network 10.0.0.0/24 area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set network area id failed"

    devIntReturn = dut01.DeviceInteract(command="no enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to disable OSPF router instance failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'router-id 1.2.3.4' in cmdOut, "Test to show router id in running " \
                                          "config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' in cmdOut, \
        "Test to show network in running config failed"

    assert 'no enable' in cmdOut, \
        "Test to show enable in running config failed"

    return True


def noRunningConfigTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router-id")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset router id failed"

    devIntReturn = dut01.DeviceInteract(command="no network 10.0.0.0/24 area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network area id failed"

    devIntReturn = dut01.DeviceInteract(command="enable")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enable OSPF router instance failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'router-id 1.2.3.4' not in cmdOut, "Test to show router id in running " \
                                          "config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' not in cmdOut, \
        "Test to show network in running config failed"

    assert 'enable' not in cmdOut, \
        "Test to show enable in running config failed"

    return True


class Test_ospf_configuration:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_ospf_configuration.testObj = testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_ospf_configuration.topoObj = \
            Test_ospf_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_ospf_configuration.topoObj.terminate_nodes()

    def test_enableOSPFFeatureTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        #dut02Obj = self.topoObj.deviceObjGet(device="dut02")
        retValue = enableOSPFFeatureTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Enabling OSPF feature - passed")
        else:
            LogOutput('error', "Enabling OSPF feature - failed")

    def test_disableOSPFFeatureTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = disableOSPFFeatureTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Disabling OSPF feature - passed")
        else:
            LogOutput('error', "Disabling OSPF feature - failed")

    def test_createRouterInstanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = createRouterInstanceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Creating OSPF router instance - passed")
        else:
            LogOutput('error', "Creating OSPF router instance - failed")

    def test_deleteRouterInstanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = deleteRouterInstanceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Deleting OSPF router instance - passed")
        else:
            LogOutput('error', "Deleting OSPF router instance - failed")

    def test_enableRouterInstanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = enableRouterInstanceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Enabling OSPF router instance - passed")
        else:
            LogOutput('error', "Enabling OSPF router instance - failed")

    def test_disableRouterInstanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = disableRouterInstanceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Disabling OSPF router instance - passed")
        else:
            LogOutput('error', "Disabling OSPF router instance - failed")

    def test_setRouterIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set router id - passed")
        else:
            LogOutput('error', "Set router id - failed")

    def test_unsetRouterIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset router id - passed")
        else:
            LogOutput('error', "Unset router id - failed")

    def test_setNetworkAreaIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setNetworkAreaIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set network area id - passed")
        else:
            LogOutput('error', "Set network area id - failed")

    def test_unsetNetworkAreaIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetNetworkAreaIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset network area id - passed")
        else:
            LogOutput('error', "Unset network area id - failed")

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
