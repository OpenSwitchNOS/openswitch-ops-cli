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


def validationOfSourceIpAddressToAllprotocolsTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all address 255.255.255.255")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate source-ip address \
to all the defined protocols failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'Broadcast, multicast and loopback addresses are not \
allowed.' in cmdOut, "Test to validate source-ip address \
to all the defined protocols failed"

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all address 224.0.0.0")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate source-ip address \
to all the defined protocols failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'Broadcast, multicast and loopback addresses are not \
allowed.' in cmdOut, "Test to validate source-ip address \
to all the defined protocols failed"

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all address 127.0.0.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate source-ip address \
to all the defined protocols failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'Broadcast, multicast and loopback addresses are not \
allowed.' in cmdOut, "Test to validate source-ip address \
to all the defined protocols failed"

    if (exitContext(dut01) is False):
        return False

    return True


def validationOfSourceIpAddressToTftpprotocolTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp address 255.255.255.255")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate source-ip address \
to TFTP protocol failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'Broadcast, multicast and loopback addresses are not \
allowed.' in cmdOut, "Test to validate source-ip address \
to TFTP protocol failed"

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp address 224.0.0.0")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate source-ip address \
to TFTP protocol failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'Broadcast, multicast and loopback addresses are not \
allowed.' in cmdOut, "Test to validate source-ip address \
to TFTP protocol failed"

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp address 127.0.0.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to validate source-ip address \
to TFTP protocol failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'Broadcast, multicast and loopback addresses are not \
allowed.' in cmdOut, "Test to validate source-ip address \
to TFTP protocol failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setSourceIpAddressToAllprotocolsTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all address 1.1.1.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip address to \
all the defined protocols failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface")
    assert '1.1.1.1' in cmdOut, "Test to set source-ip \
address to all the defined protocols failed"

    return True


def setSourceIpAddressToTftpprotocolTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp address 1.1.1.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip address \
to TFTP protocol failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface tftp")
    assert '1.1.1.1' in cmdOut, "Test to set source-ip \
address to TFTP protocol failed"

    return True


def setSourceIpInterfaceToAllprotocolsTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
all interface 2")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip \
interface to all the defined protocols failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface")
    assert '2' in cmdOut, "Test to set source-ip \
interface to all the defined protocols failed"

    return True


def setSourceIpInterfaceToTftpprotocolTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip source-interface \
tftp interface 2")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set source-ip \
interface to TFTP protocol failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface tftp")
    assert '2' in cmdOut, "Test to set source-ip \
interface to TFTP protocol failed"

    return True


def unsetSourceIpToAllprotocolsTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip source-interface all")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset source-ip to \
all the defined protocols failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface")
    assert '(null)' in cmdOut, "Test to unset source-ip to \
all the defined protocols failed"

    return True


def unsetSourceIpToTftpprotocolTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip source-interface tftp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset source-ip to tftp protocol failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip source-interface tftp")
    assert '(null)' in cmdOut, "Test to unset source-ip to \
tftp protocol failed"

    return True


@pytest.mark.skipif(True, reason="Disabling old tests")
class Test_source_interface_configuration:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_source_interface_configuration.testObj = \
            testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_source_interface_configuration.topoObj = \
            Test_source_interface_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_source_interface_configuration.topoObj.terminate_nodes()

    def test_validationOfSourceIpAddressToAllprotocolsTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = validationOfSourceIpAddressToAllprotocolsTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of source IP address \
to all the defined protocols - passed")
        else:
            LogOutput('error', "Validation of source IP address \
to all the defined protocols - failed")

    def test_validationOfSourceIpAddressToTftpprotocolTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = validationOfSourceIpAddressToTftpprotocolTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Validation of source IP address \
to tftp protocol - passed")
        else:
            LogOutput('error', "Validation of source IP address \
to tftp protocol - failed")

    def test_setSourceIpAddressToAllprotocolsTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpAddressToAllprotocolsTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP address \
to all the defined protocols - passed")
        else:
            LogOutput('error', "Set source IP address \
to all the defined protocols - failed")

    def test_setSourceIpAddressToTftpprotocolTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpAddressToTftpprotocolTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP address \
to tftp protocol - passed")
        else:
            LogOutput('error', "Set source IP address \
to tftp protocol - failed")

    def test_setSourceIpInterfaceToAllprotocolsTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpInterfaceToAllprotocolsTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP \
interface to all the defined protocols - passed")
        else:
            LogOutput('error', "Set source IP \
interface to all the defined protocols - failed")

    def test_setSourceIpInterfaceToTftpprotocolTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setSourceIpInterfaceToTftpprotocolTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set source IP \
interface to tftp protocol - passed")
        else:
            LogOutput('error', "Set source IP \
interface to tftp protocol - failed")

    def test_unsetSourceIpToAllprotocolsTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetSourceIpToAllprotocolsTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset source IP \
to all the defined protocols - passed")
        else:
            LogOutput('error', "Unset source IP \
to all the defined protocols - failed")

    def test_unsetSourceIpToTftpprotocolTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetSourceIpToTftpprotocolTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset source IP to tftp protocols - passed")
        else:
            LogOutput('error', "Unset source IP to tftp protocols - failed")
