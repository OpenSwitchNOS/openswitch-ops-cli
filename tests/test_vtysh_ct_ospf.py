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
from opsvsiutils.vtyshutils import *

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


def setDistanceTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="distance 5")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set distance cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'distance 5' in cmdOut, "Test to set distance failed"

    return True


def unsetDistanceTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no distance 5")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset distance failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'distance 5' not in cmdOut, "Test to unset distance failed"
    return True


def configure(**kwargs):
    '''
     - Configures the IP address in SW1 and SW2
     - Creates router ospf instances
     - Configures the router id
     - Configures the network range and area
    '''

    switch1 = kwargs.get('switch1', None)
    switch2 = kwargs.get('switch2', None)

    '''
    - Enable the link.
    - Set IP for the switches.
    '''
    # Enabling interface 1 SW1.
    LogOutput('info', "Enabling interface1 on SW1")
    retStruct = InterfaceEnable(deviceObj=switch1, enable=True,
                                interface=switch1.linkPortMapping['lnk01'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Unable to enable interface1 on SW1"

    # Assigning an IPv4 address on interface 1 of SW1
    LogOutput('info', "Configuring IPv4 address on link 1 SW1")
    retStruct = InterfaceIpConfig(deviceObj=switch1,
                                  interface=switch1.linkPortMapping['lnk01'],
                                  addr="10.10.10.1", mask="24",
                                  config=True)
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to configure an IPv4 address on interface 1 of SW1"

    # Enabling interface 1 SW2
    LogOutput('info', "Enabling interface1 on SW2")
    retStruct = InterfaceEnable(deviceObj=switch2, enable=True,
                                interface=switch2.linkPortMapping['lnk01'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Unable to enable interface 1 on SW2"

    # Assigning an IPv4 address on interface 1 for link 1 SW2
    LogOutput('info', "Configuring IPv4 address on link 1 SW2")
    retStruct = InterfaceIpConfig(deviceObj=switch2,
                                  interface=switch2.linkPortMapping['lnk01'],
                                  addr="10.10.10.2", mask="24",
                                  config=True)
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to configure an IPv4 address on interface 1 of SW2"

    '''
    For all the switches
    - Create the instance.
    - Configure network range and area id.
    - Configure the router Id.
    '''
    result = setRouterIdTest(switch1)
    assert result is True, "OSPF router id set failed for SW1"
    LogOutput('info', "Configuring OSPF network for SW1")
    result = setNetworkAreaIdTest(switch1)
    assert result is True, "OSPF network creation failed for SW1"
    exitContext(switch1)  # In config context

    result = setRouterIdTest(switch2)
    assert result is True, "OSPF router id set failed for SW2"
    LogOutput('info', "Configuring OSPF network for SW2")
    result = setNetworkAreaIdTest(switch2)
    assert result is True, "OSPF network creation failed for SW1"
    exitContext(switch2)  # In config context


# Function to get the dead interval and hello timer value
def getTimers(dut02, timer_type):
    ospf_interface = SwitchVtyshUtils.vtysh_cmd(dut02, "show\
                                                        ip ospf interface")

    if "Dead" in timer_type:
        matchObj = re.search(r'Dead\s\d+', ospf_interface)
    elif "Hello" in timer_type:
        matchObj = re.search(r'Hello\s\d+', ospf_interface)

    if matchObj:
        timers = re.split(r'\s+', matchObj.group())
        timer = timers[1]
    else:
        timer = 0

    return timer


def setHelloIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf hello-interval 25")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set hello interval command failed"

    if (exitContext(dut01) is False):
        return False

    return True


def unsetHelloIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf hello-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset hello interval command failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setDeadIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf dead-interval 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set dead interval command failed"

    if (exitContext(dut01) is False):
        return False

    return True


def unsetDeadIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf dead-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset dead interval failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setAreaAuthType(dut01, authType, area):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " authentication"
    auth_output = "simple password authentication"

    if authType == "md5":
        cmd = cmd + " message-digest"
        auth_output = "message digest authentication"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set auth type %s with area %s cmd failed" % \
                         (authType, area)

    if (exitContext(dut01) is False):
        return False

    ospf_auth = SwitchVtyshUtils.vtysh_cmd(dut01, "show ip ospf")
    assert auth_output in ospf_auth, "Test to set auth type %s with " \
                                     "area %s failed" % (authType, area)

    return True


def unsetAreaAuthType(dut01, area):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " authentication"
    auth_output = "no authentication"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset auth with area %s cmd failed"\
                         % (area)

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfaceAuthType(dut01, authType):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf authentication"

    if authType == "md5":
        cmd += " message-digest"

    if authType == "null":
        cmd += " null"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set auth type %s in interface 1 failed" % \
                         (authType)

    return True


def setInterfaceAuthKey(dut01, authKey):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf authentication-key " + authKey

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set auth key %s in interface 1 failed" % \
                         (authKey)

    return True


def unsetInterfaceAuthKey(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf authentication-key"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset auth key in interface 1 failed"

    return True


def setInterfaceMd5Key(dut01, md5KeyId, md5Key):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf message-digest-key " + md5KeyId + " md5 " + md5Key

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set md5 key %s with keyId %s in " \
                         "interface 1 failed" % (md5Key, md5KeyId)
    return True


def unsetInterfaceMd5Key(dut01, md5KeyId):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf message-digest-key " + md5KeyId

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset md5 key id %s in interface 1 failed" \
                         % (md5KeyId)
    return True


def unsetInterfaceAuthType(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf authentication"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset auth in interface 1 failed"

    return True


def setAreaNssa(dut01, area, nssaType, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " nssa " + nssaType

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set nssa type %s with area %s cmd failed" % \
                         (nssaType, area)

    output = devIntReturn.get('buffer')

    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure NSSA to backbone" in output, \
            "Should not configure backbone area as NSSA"

    return True


def unsetAreaNssa(dut01, area, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " nssa"

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset area %s from nssa failed" % area

    output = devIntReturn.get('buffer')
    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure NSSA to backbone" in output, \
            "Should not configure backbone area as NSSA"

    return True


def setAreaStub(dut01, area, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " stub"

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set area %s as stub failed" % area

    output = devIntReturn.get('buffer')
    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure" and "to backbone" in output, \
            "Should not configure backbone area as Stub"

    return True


def unsetAreaStub(dut01, area, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " stub"

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset area %s from stub failed" % area

    output = devIntReturn.get('buffer')
    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure" and "to backbone" in output, \
            "Should not configure backbone area as Stub"

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

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'router-id 1.2.3.4' in cmdOut, "Test to show router id in " \
                                          "running config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' in cmdOut, \
        "Test to show network in running config failed"
    assert 'max-metric router-lsa on-startup 200' in cmdOut, \
        "Test to show max-metric in running config failed"

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

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'router-id 1.2.3.4' not in cmdOut, "Test to show router id " \
                                              "in running config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' not in cmdOut, \
        "Test to show network in running config failed"
    assert 'max-metric router-lsa on-startup 200' not in cmdOut, \
        "Test to show max-metric in running config failed"

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

    def test_setDistanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set distance - passed")
        else:
            LogOutput('error', "Set distance - failed")

    def test_unsetDistanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset distance - passed")
        else:
            LogOutput('error', "Unset distance - failed")

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

    def test_configure(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        dut02Obj = self.topoObj.deviceObjGet(device="dut02")
        configure(switch1=dut01Obj, switch2=dut02Obj)

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

    # Enable text auth for backbone
    def test_setAreaTextAuthForBackbone(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "0.0.0.0")
        if(retValue):
            LogOutput('info', "Set Backbone area text auth  - passed")
        else:
            LogOutput('error', "Set Backbone area text auth - failed")

    # Enable md5 auth for backbone
    def test_setAreaMd5AuthForBackbone(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "0")
        if(retValue):
            LogOutput('info', "Set Backbone area md5 auth  - passed")
        else:
            LogOutput('error', "Set Backbone area md5 auth - failed")

    # Enable text auth
    def test_setAreaTextAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "1.1.1.1")
        if(retValue):
            LogOutput('info', "Set area text auth  - passed")
        else:
            LogOutput('error', "Set area text auth - failed")

    # Enable md5 auth
    def test_setAreaMd5Auth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "1")
        if(retValue):
            LogOutput('info', "Set area md5 auth  - passed")
        else:
            LogOutput('error', "Set area md5 auth - failed")

    # Disable auth for backbone
    def test_unsetAreaAuthForBackbone(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaAuthType(dut01Obj, "0")
        if(retValue):
            LogOutput('info', "Unset Backbone area auth - passed")
        else:
            LogOutput('error', "Unset Backbone area auth - failed")

    def test_unsetAreaAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaAuthType(dut01Obj, "1.1.1.1")
        if(retValue):
            LogOutput('info', "Unset area auth - passed")
        else:
            LogOutput('error', "Unset area auth - failed")

    # Enable text auth in interface 1
    def test_setInterfaceTextAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthType(dut01Obj, "text")
        if(retValue):
            LogOutput('info', "Set Backbone interface text auth  - passed")
        else:
            LogOutput('error', "Set Backbone interface text auth - failed")

    # Enable md5 auth in interface 1
    def test_setInterfaceMd5Auth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthType(dut01Obj, "md5")
        if(retValue):
            LogOutput('info', "Set interface md5 auth  - passed")
        else:
            LogOutput('error', "Set interface md5 auth - failed")

    # Enable null auth in interface 1
    def test_setInterfaceNullAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthType(dut01Obj, "null")
        if(retValue):
            LogOutput('info', "Set interface null auth  - passed")
        else:
            LogOutput('error', "Set interface null auth - failed")

    def test_unsetInterfaceAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceAuthType(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset interface auth - passed")
        else:
            LogOutput('error', "Unset interface auth - failed")

    def test_setAreaNssaCandidate(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaNssa(dut01Obj, "0.0.0.0", "translate-candidate",
                               False)
        if(retValue):
            LogOutput('info', "Set area NSSA candidate - passed")
        else:
            LogOutput('error', "Set area NSSA candidate - failed")

    def test_setAreaNssaNever(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaNssa(dut01Obj, "0", "translate-never", False)
        if(retValue):
            LogOutput('info', "Set area NSSA never - passed")
        else:
            LogOutput('error', "Set area NSSA never - failed")

    def test_setAreaNssaAlways(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaNssa(dut01Obj, "100", "translate-always", True)
        if(retValue):
            LogOutput('info', "Set area NSSA always - passed")
        else:
            LogOutput('error', "Set area NSSA always - failed")

    def test_unsetAreaNssa(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaNssa(dut01Obj, "0", False)
        if(retValue):
            LogOutput('info', "unset area NSSA  - passed")
        else:
            LogOutput('error', "unset area NSSA - failed")

    def test_unsetAreaNssaNoSummary(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaNssa(dut01Obj, "100", True)
        if(retValue):
            LogOutput('info', "unset area NSSA no summary - passed")
        else:
            LogOutput('error', "unset area NSSA no summary - failed")

    def test_setAreaStub(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaStub(dut01Obj, "0", False)
        if(retValue):
            LogOutput('info', "Set area stub - passed")
        else:
            LogOutput('error', "Set area stub - failed")

    def test_setAreaStubNoSummary(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaStub(dut01Obj, "0", True)
        if(retValue):
            LogOutput('info', "Set area stub no summary - passed")
        else:
            LogOutput('error', "Set area stub no summary - failed")

    def test_unsetAreaStub(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaStub(dut01Obj, "0", False)
        if(retValue):
            LogOutput('info', "unset area stub - passed")
        else:
            LogOutput('error', "unset area stub - failed")

    def test_unsetAreaStubNoSummary(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaStub(dut01Obj, "0", True)
        if(retValue):
            LogOutput('info', "Unset area stub no summary - passed")
        else:
            LogOutput('error', "Unset area stub no summary - failed")

    def test_setInterfaceAuthKey(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthKey(dut01Obj, "victory")
        if(retValue):
            LogOutput('info', "Set Interface auth key - passed")
        else:
            LogOutput('error', "Set Interface auth key - failed")

    def test_unsetInterfaceAuthKey(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceAuthKey(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface auth key - passed")
        else:
            LogOutput('error', "Unset Interface auth key - failed")

    def test_setInterfaceMd5Key(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceMd5Key(dut01Obj, "1", "md5victory")
        if(retValue):
            LogOutput('info', "Set Interface md5 key - passed")
        else:
            LogOutput('error', "Set Interface md5 key - failed")

    def test_unsetInterfaceMd5Key(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceMd5Key(dut01Obj, "1")
        if(retValue):
            LogOutput('info', "Unset Interface md5 key - passed")
        else:
            LogOutput('error', "Unset Interface md5 key - failed")
