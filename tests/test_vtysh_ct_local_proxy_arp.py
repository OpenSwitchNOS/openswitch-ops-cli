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
topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01",
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}


def enterConfigShell(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    return True


def exitContext(dut01):
    retStruct = dut01.ConfigVtyShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit config terminal"

    retStruct = dut01.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"

    return True


def LocalproxyArpOnL3InterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enable local proxy ARP on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="do show running-config")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show running config"

    cmdOut = devIntReturn.get('buffer')
    assert 'ip local-proxy-arp' in cmdOut, "Failed to validate the "\
        "presence of 'ip local-proxy-arp' in show running-config"

    devIntReturn = dut01.DeviceInteract(command="no ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to disable local proxy ARP  on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config interface 1")
    assert 'ip local-proxy-arp' not in cmdOut, "Failed to validate the "\
        "absence of 'ip local-proxy-arp' in the show running config"\
        "interface output"

    cmdOut = dut01.cmdVtysh(command="show interface 1")
    assert 'Local Proxy ARP is enabled' not in cmdOut, "Failed to validate "\
        "the absence of string 'Local proxy ARP is enabled' in show "\
        "interface output"

    return True


def LocalproxyArpOnNoRoutingTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="no routing")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to disable routing on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Command execution failure"

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'ip local-proxy-arp' not in cmdOut, "Failed to validate the "\
        "absence of 'ip local-proxy-arp' in the show running-config output"

    return True


def LocalproxyArpOnL3VlanInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface vlan 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface vlan 1 context"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enable local proxy ARP"\
        "on a L3 vlan interface"

    devIntReturn = dut01.DeviceInteract(command="do show running-config")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show running config"

    cmdOut = devIntReturn.get('buffer')
    assert 'ip local-proxy-arp' in cmdOut, "Failed to validate the"\
        "presence of 'ip local-proxy-arp' in show running-config output"

    devIntReturn = dut01.DeviceInteract(command="no ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to disable local proxy ARP"\
        "on a L3 vlan interface"

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False
    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'ip local-proxy-arp' not in cmdOut, "Failed to validate the"\
        "absence of 'ip local-proxy-arp' in the show running-config output"

    return True


def LocalproxyArpOnSplitChildInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="split \n y")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to split the parent interface"

    devIntReturn = dut01.DeviceInteract(command="interface 50-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enable local proxy ARP on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="do show interface 50-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show interface"

    cmdOut = devIntReturn.get('buffer')
    assert 'Local Proxy ARP is enabled' in cmdOut, "Failed to validate the"\
        "presence of string 'Local proxy ARP is enabled' in show"\
        " interface output"

    devIntReturn = dut01.DeviceInteract(command="no ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to disable local proxy ARP"
    "on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="do show running-config")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show running config"

    cmdOut = devIntReturn.get('buffer')
    assert 'ip local-proxy-arp' not in cmdOut, "Failed to validate the "\
        "absence of 'ip local-proxy-arp' in the show running-config output"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enable local proxy ARP on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="interface 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="no split \n y")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to validate no split on interface"

    devIntReturn = dut01.DeviceInteract(command="do show running-config")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show running config"

    cmdOut = devIntReturn.get('buffer')
    assert 'ip local-proxy-arp' not in cmdOut, "Failed to validate the "\
        "absence of 'ip local-proxy-arp' in the show running-config output"

    devIntReturn = dut01.DeviceInteract(command="interface 51-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Command execution failure"

    cmdOut = devIntReturn.get('buffer')
    assert 'This is a QSFP child interface whose'
    'parent interface has not been split.' in cmdOut, "Failed to validate"\
        "that local proxy ARP configuration fails on a child interface of"\
        "a non-split parent interface"
    return True

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False


def LocalproxyArpOnParentInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 51")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter interface context"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enable local proxy ARP on "
    "a L3 port"

    devIntReturn = dut01.DeviceInteract(command="do show interface 51")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show interface"

    cmdOut = devIntReturn.get('buffer')
    assert 'Local Proxy ARP is enabled' in cmdOut, "Failed to validate the "\
        "presence of string 'Local proxy ARP is enabled' in show "\
        "interface output "

    devIntReturn = dut01.DeviceInteract(command="no ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to disable local proxy ARP"\
        "on a L3 port"

    devIntReturn = dut01.DeviceInteract(command="do show running-config")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to execute do show running config"

    cmdOut = devIntReturn.get('buffer')
    assert 'ip local-proxy-arp' not in cmdOut, "Failed to validate the"\
        "absence of'ip local-proxy-arp' in the show running-config output"

    devIntReturn = dut01.DeviceInteract(command="split \n yes")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to split the parent interface"

    devIntReturn = dut01.DeviceInteract(command="ip local-proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Command execution failure"

    cmdOut = devIntReturn.get('buffer')
    assert 'This interface has been split'
    'Operation not allowed.' in cmdOut, "Failed to validate that "\
        "local proxy ARP configuration fails on split parent interface"
    return True

    if (enterConfigShell(dut01) is False):
        return False
    return True


@pytest.mark.skipif(True, reason="Disabling this testcase "
                    "since local proxy ARP feature is globally disabled")
class Test_Local_proxy_arp_configuration:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_Local_proxy_arp_configuration.testObj = \
            testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_Local_proxy_arp_configuration.topoObj = \
            Test_Local_proxy_arp_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_Local_proxy_arp_configuration.topoObj.terminate_nodes()

    def test_LocalproxyArpOnL3InterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = LocalproxyArpOnL3InterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of local proxy ARP on L3 interface"
                              "- passed")
        else:
            LogOutput('error', "Validation of local proxy ARP on L3 interface"
                               "- failed")

    def test_LocalproxyArpOnNoRoutingTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = LocalproxyArpOnNoRoutingTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of local proxy ARP on L2 "
                              " interface- passed")
        else:
            LogOutput('error', "Validation of local proxy ARP on L2"
                               " interface - failed")

    def test_LocalproxyArpOnL3VlanInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = LocalproxyArpOnL3VlanInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of local proxy ARP on VLAN"
                              " interface- passed")
        else:
            LogOutput('error', "Validation of local proxy ARP on VLAN"
                               " interface- failed")

    def test_LocalproxyArpOnParentInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = LocalproxyArpOnParentInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of local proxy ARP on split parent"
                              " interface- passed")
        else:
            LogOutput('error', "Validation of local proxy ARP on split parent"
                               "interface- failed")

    def test_LocalproxyArpOnSplitChildInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = LocalproxyArpOnSplitChildInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of local proxy ARP on split child "
                              " Interface - passed")
        else:
            LogOutput('error', "Validation of local proxy ARP on split child"
                               " Interface- failed")
