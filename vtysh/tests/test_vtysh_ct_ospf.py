#!/usr/bin/python

# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
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

    devIntReturn = dut01.DeviceInteract(command="no network 10.0.0.0/24 "
                                                "area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network area id failed"

    return True


def setMaxMetricAdminTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="max-metric router-lsa")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set max-metric admin failed"

    return True


def unsetMaxMetricAdminTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no max-metric router-lsa")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset max-metric admin failed"

    return True


def setMaxMetricStartupTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="max-metric router-lsa "
                                                "on-startup 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set max-metric startup failed"

    return True


def unsetMaxMetricStartupTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no max-metric router-lsa "
                                                "on-startup")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset max-metric startup failed"

    return True


def setHelloIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf hello-interval 25")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set hello interval failed"

    return True


def unsetHelloIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf hello-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset hello interval failed"

    return True


def setDeadIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf dead-interval 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set dead interval failed"

    return True


def unsetDeadIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf dead-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset dead interval failed"

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

    devIntReturn = dut01.DeviceInteract(command="max-metric router-lsa "
                                                "on-startup 200")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set max-metric startup failed"

    if (exitContext(dut01) is False):
        return False

    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf hello-interval 25")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set hello interval failed"

    devIntReturn = dut01.DeviceInteract(command="ip ospf dead-interval 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set dead interval failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'router-id 1.2.3.4' in cmdOut, "Test to show router id in " \
                                          "running config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' in cmdOut, \
        "Test to show network in running config failed"
    assert 'max-metric router-lsa on-startup 200' in cmdOut, \
        "Test to show max-metric in running config failed"
    assert 'ip ospf hello-interval 25' in cmdOut, \
        "Test to show hello-interval in running config failed"
    assert 'ip ospf dead-interval 50' in cmdOut, \
        "Test to show dead-interval in running config failed"

    return True


def noRunningConfigTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router-id")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset router id failed"

    devIntReturn = dut01.DeviceInteract(command="no network 10.0.0.0/24 "
                                                "area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network area id failed"

    devIntReturn = dut01.DeviceInteract(command="no max-metric router-lsa "
                                                "on-startup")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset max-metric startup failed"

    if (exitContext(dut01) is False):
        return False

    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf hello-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset hello interval failed"

    devIntReturn = dut01.DeviceInteract(command="no ip ospf dead-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset dead interval failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'router-id 1.2.3.4' not in cmdOut, "Test to show router id " \
                                              "in running config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' not in cmdOut, \
        "Test to show network in running config failed"
    assert 'max-metric router-lsa on-startup 200' not in cmdOut, \
        "Test to show max-metric in running config failed"
    assert 'ip ospf hello-interval 25' not in cmdOut, \
        "Test to show hello-interval in running config failed"
    assert 'ip ospf dead-interval 50' not in cmdOut, \
        "Test to show dead-interval in running config failed"

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

    def test_setMaxMetricAdminTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setMaxMetricAdminTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set max metric admin - passed")
        else:
            LogOutput('error', "Set max metric admin - failed")

    def test_unsetMaxMetricAdminTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetMaxMetricAdminTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset max metric admin - passed")
        else:
            LogOutput('error', "Unset max metric admin - failed")

    def test_setMaxMetricStartupTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setMaxMetricStartupTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set max metric startup - passed")
        else:
            LogOutput('error', "Set max metric startup - failed")

    def test_unsetMaxMetricStartupTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetMaxMetricStartupTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset max metric startup - passed")
        else:
            LogOutput('error', "Unset max metric startup - failed")

    def test_setHelloIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setHelloIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set hello interval - passed")
        else:
            LogOutput('error', "Set hello interval - failed")

    def test_unsetHelloIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetHelloIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset hello interval - passed")
        else:
            LogOutput('error', "Unset hello interval - failed")

    def test_setDeadIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setDeadIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set Dead interval - passed")
        else:
            LogOutput('error', "Set Dead interval - failed")

    def test_unsetDeadIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetDeadIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Dead interval - passed")
        else:
            LogOutput('error', "Unset Dead interval - failed")

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
