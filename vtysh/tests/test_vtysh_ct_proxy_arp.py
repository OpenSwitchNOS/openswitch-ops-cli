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


def validationOfProxyArpToEnableTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp enabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False
    cmdOut = dut01.cmdVtysh(command="show interface 1")
    assert 'Proxy ARP is enabled' in cmdOut, "Test to set ip proxy-arp"
    "config failed"
    return True


def validationOfProxyArpToDisableTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="no ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate no ip proxy-arp"
    "disabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config interface 1")
    assert 'ip proxy-arp' not in cmdOut, "Test to set no ip proxy-arp"
    "config failed"
    return True


def ProxyArpOnNoRoutingTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="no routing")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "disabling failed "

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "disabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'ip proxy-arp' not in cmdOut, "Test to set no ip proxy-arp"
    "config failed"
    return True


def ProxyArpOnL3VlanInterfaceEnableTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface vlan 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "enabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False
    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'ip proxy-arp' in cmdOut, "Test to set ip proxy-arp"
    "config failed"

    return True


def ProxyArpOnL3VlanInterfaceDisableTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface vlan 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="no ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate no ip proxy-arp"
    "enabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False
    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'ip proxy-arp' not in cmdOut, "Test to set ip proxy-arp"
    "config failed"

    return True


def ProxyArpToEnableOnSplitChildInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="split \n y")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface"

    devIntReturn = dut01.DeviceInteract(command="interface 50-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface 50-1"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "enabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command=" show interface 50-1")
    assert 'Proxy ARP is enabled' in cmdOut, "Test to set ip proxy-arp"
    "config failed"
    return True


def ProxyArpToDisableOnSplitChildInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="split \n y")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface"

    devIntReturn = dut01.DeviceInteract(command="interface 50-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface 50-1"

    devIntReturn = dut01.DeviceInteract(command="no ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate no ip proxy-arp"
    "enabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command=" show running-config")
    assert 'ip proxy-arp' not in cmdOut, "Test to set no ip proxy-arp"
    "config failed"
    return True


def ProxyArpNoSplitParentInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="split \n y")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface"

    devIntReturn = dut01.DeviceInteract(command="interface 50-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface 50-1"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "enabling failed "

    devIntReturn = dut01.DeviceInteract(command="interface 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="no split \n y")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to do no split on  interface"

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command=" show running-config")
    assert 'ip proxy-arp' not in cmdOut, "Test to set no ip proxy-arp"
    "config failed"
    return True


def ProxyArpNonSplitChildInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False
    devIntReturn = dut01.DeviceInteract(command="interface 51-1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter split interface 51-1"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "enabling failed "

    cmdOut = devIntReturn.get('buffer')
    assert 'This is a QSFP child interface whose'
    'parent interface has not been split.' in cmdOut, "check for non-split\
    parent interface failed with error Message"
    return True

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False


def ProxyArpEnableOnParentInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 51")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp"
    "disabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show interface 51")
    assert 'Proxy ARP is enabled' in cmdOut, "Test to set ip proxy-arp"
    " config failed"
    return True


def ProxyArpDisableOnParentInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 51")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="no ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate no ip proxy-arp"
    " disabling failed "

    devIntReturn = dut01.DeviceInteract(command="exit")

    if (exitContext(dut01) is False):
        return False
    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'ip proxy-arp' not in cmdOut, "Test to set ip proxy-arp"
    " config failed"

    return True


def ProxyArpOnSplitParentInterfaceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="interface 51")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    devIntReturn = dut01.DeviceInteract(command="split \n yes")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate split "
    "interface is configured"

    devIntReturn = dut01.DeviceInteract(command="ip proxy-arp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate ip proxy-arp "
    "enabling failed "

    cmdOut = devIntReturn.get('buffer')
    assert 'This interface has been split'
    'Operation not allowed.' in cmdOut, "check for non-split\
    parent interface failed with error Message"
    return True

    if (enterConfigShell(dut01) is False):
        return False
    return True


class Test_proxy_arp_configuration:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_proxy_arp_configuration.testObj = \
            testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_proxy_arp_configuration.topoObj = \
            Test_proxy_arp_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_proxy_arp_configuration.topoObj.terminate_nodes()

    def test_validationOfProxyArpToEnableTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = validationOfProxyArpToEnableTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP Enable - passed")
        else:
            LogOutput('error', "Validation of proxy ARP Enable- failed")

    def test_validationOfProxyArpToDisableTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = validationOfProxyArpToDisableTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP Disable- passed")
        else:
            LogOutput('error', "Validation of proxy ARP Disable- failed")

    def test_ProxyArpOnNoRoutingTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpOnNoRoutingTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Proxy ARP is not config on L2 "
                              " interface- passed")
        else:
            LogOutput('error', "Proxy ARP is not config on L2"
                               " interface - failed")

    def test_ProxyArpOnL3VlanInterfaceEnableTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpOnL3VlanInterfaceEnableTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP enable on VLAN"
                              " interface- passed")
        else:
            LogOutput('error', "Validation of proxy ARP enable on VLAN"
                               " interface- failed")

    def test_ProxyArpOnL3VlanInterfaceDisableTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpOnL3VlanInterfaceDisableTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP disable on VLAN"
                              " interface- passed")
        else:
            LogOutput('error', "Validation of proxy ARP disable on VLAN"
                               " interface- failed")

    def test_ProxyArpEnableOnParentInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpEnableOnParentInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP enable on parent"
                              " interface- passed")
        else:
            LogOutput('error', "Validation of proxy ARP enable on parent"
                               "interface- failed")

    def test_ProxyArpDisableOnParentInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpDisableOnParentInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP disable on "
                              " parent interface- passed")
        else:
            LogOutput('error', "Validation of proxy ARP disable on "
                               " parent interface- failed")

    def test_ProxyArpOnSplitParentInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpOnSplitParentInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP on split parent"
                              " interface- passed")
        else:
            LogOutput('error', "Validation of proxy ARP on split parent"
                               "interface- failed")

    def test_ProxyArpToEnableOnSplitChildInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpToEnableOnSplitChildInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP enable on split child "
                              " Interface - passed")
        else:
            LogOutput('error', "Validation of proxy ARP enable on split child"
                               " Interface- failed")

    def test_ProxyArpToDisableOnSplitChildInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpToDisableOnSplitChildInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP disable on split child "
                              " Interface - passed")
        else:
            LogOutput('error', "Validation of proxy ARP disable on split child"
                               " Interface- failed")

    def test_ProxyArpNoSplitParentInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpNoSplitParentInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP no split on parent "
                              " Interface - passed")
        else:
            LogOutput('error', "Validation of proxy ARP no split on parent "
                               " Interface- failed")

    def test_ProxyArpNonSplitChildInterfaceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = ProxyArpNonSplitChildInterfaceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of proxy ARP non split child "
                              " Interface - passed")
        else:
            LogOutput('error', "Validation of proxy ARP non split child"
                               " Interface- failed")
