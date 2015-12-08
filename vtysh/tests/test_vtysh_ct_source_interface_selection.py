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


def setSourceIpAddressToAllServersTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all address 1.1.1.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip address to all servers failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface")
    assert '1.1.1.1' in cmdOut, "Test to set source-ip \
address to all servers failed"

    return True


def setSourceIpAddressToTftpServerTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp address 1.1.1.3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip address to tftp server failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface tftp")
    assert '1.1.1.3' in cmdOut, "Test to set source-ip \
address to tftp server failed"

    return True


def setSourceIpInterfaceToAllServersTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all interface 2")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip \
interface to all servers failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface")
    assert '2' in cmdOut, "Test to set source-ip \
interface to all servers failed"

    return True


def setSourceIpInterfaceToTftpServerTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp interface 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip \
interface to tftp server failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface tftp")
    assert '3' in cmdOut, "Test to set source-ip \
interface to tftp server failed"

    return True


def unsetSourceIpToAllServersTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip source-interface all")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset source-ip to all servers failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface")
    assert '(null)' in cmdOut, "Test to unset source-ip to all servers failed"

    return True


def unsetSourceIpToTftpServerTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip source-interface tftp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset source-ip to tftp server failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface tftp")
    assert '(null)' in cmdOut, "Test to unset source-ip to tftp server failed"

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

    def test_setSourceIpAddressToAllServersTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpAddressToAllServersTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP address to all servers - passed")
        else:
            LogOutput('error', "Set source IP address to all servers - failed")

    def test_setSourceIpAddressToTftpServerTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpAddressToTftpServerTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP address to tftp server - passed")
        else:
            LogOutput('error', "Set source IP address to tftp server - failed")

    def test_setSourceIpInterfaceToAllServersTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpInterfaceToAllServersTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP \
interface to all servers - passed")
        else:
            LogOutput('error', "Set source IP \
interface to all servers - failed")

    def test_setSourceIpInterfaceToTftpServerTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpInterfaceToTftpServerTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP \
interface to tftp server - passed")
        else:
            LogOutput('error', "Set source IP \
interface to tftp server - failed")

    def test_unsetSourceIpToAllServersTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetSourceIpToAllServersTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset source IP to all servers - passed")
        else:
            LogOutput('error', "Unset source IP to all servers - failed")

    def test_unsetSourceIpToTftpServerTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetSourceIpToTftpServerTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset source IP to tftp servers - passed")
        else:
            LogOutput('error', "Unset source IP to tftp servers - failed")
