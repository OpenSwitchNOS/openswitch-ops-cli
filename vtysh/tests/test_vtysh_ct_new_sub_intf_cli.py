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
            "topoDevices": "dut01 wrkston01",
            "topoLinks": "lnk01:dut01:wrkston01",
            "topoFilters": "dut01:system-category:switch,\
                            wrkston01:system-category:workstation"}


def subintf_cli(**kwargs):

    device1 = kwargs.get('device1', None)
    device2 = kwargs.get('device2', None)

# creating a sub interface and verifying in port and interface table
    retStruct = InterfaceEnable(deviceObj=device1, enable=True,
                                interface="4.2")

    if retStruct.returnCode() != 0:
        LogOutput('error', "Failed to enable interface")
        assert(False)

    LogOutput('info', "### Verify the interface is created with'\
              same name for L3 port ###")
    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get interface 4.2 name")
    retCode = devIntReturn.get('buffer')
    assert "4.2" in retCode, "Failed to retrieve ovs-vsctl command"
    LogOutput('info', "### interface 4.2 created successfully ###")

    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get port 4.2 name")
    retCode = devIntReturn.get('buffer')
    assert "4.2" in retCode, "Failed to retrieve ovs-vsctl command"
    LogOutput('info', "### interface 4.2 created successfully ###")

# configuring the ip address and verifying ip address in port table
    retStruct = InterfaceIpConfig(deviceObj=device1,
                                  interface="4.3",
                                  addr="192.168.1.2", mask=24, config=True)

    if retStruct.returnCode() != 0:
        LogOutput('error',
                  "### Failed to configure interface IPV4 address ###")
        assert(False)

    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get port 4.3 ip4_address")
    retCode = devIntReturn.get('buffer')
    assert '192.168.1.2/24' in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

# verifying in assigning Invalid Ip address
    retStruct = InterfaceIpConfig(deviceObj=device1,
                                  interface="4.5",
                                  addr="0.0.0.0", mask=24, config=True)
    retCode = retStruct.buffer()
    assert 'Invalid IP address' in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

# verifying in assigning Invalid Ip address
    retStruct = InterfaceIpConfig(deviceObj=device1,
                                  interface="4.4",
                                  addr="255.255.255.255", mask=24, config=True)
    retCode = retStruct.buffer()
    assert 'Invalid IP address' in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

# configuring the ipv6 address and verifying in port table
    retStruct = InterfaceIpConfig(deviceObj=device1,
                                  interface="4.4",
                                  addr="10:10::10:10", ipv6flag=True,
                                  mask=24, config=True)
    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get port 4.4 ip6_address")
    retCode = devIntReturn.get('buffer')
    assert "10:10::10:10/24" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

# verifying same vlan for different subinterface
    retStruct = Dot1qEncapsulation(deviceObj=device1, subInterface="4.3",
                                   dot1q=True, vlan=100)
    if retStruct.returnCode() != 0:
        LogOutput('error', "failed to configure vlan")
        assert(False)

    retStruct = Dot1qEncapsulation(deviceObj=device1, subInterface="4.8",
                                   dot1q=True, vlan=100)
    retCode = retStruct.buffer()
    assert "Encapsulation VLAN is already configured on interface 4.3." \
        in retCode, 'Test to verify sub-interface configuration clis - FAILED!'

    retStruct = InterfaceIpConfig(deviceObj=device1,
                                  interface="4.8",
                                  addr="192.168.1.2", mask=24, config=True)

    retCode = retStruct.buffer()
    assert "Duplicate IP Address." in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    retStruct = Dot1qEncapsulation(deviceObj=device1, subInterface="4.8",
                                   dot1q=False, vlan=100)
    devIntReturn = device1.DeviceInteract(command="do show running-config")
    retCode = retStruct.buffer()
    assert "encapsulation dot1Q 100" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    retStruct = InterfaceEnable(deviceObj=device1, enable=True,
                                interface="4")
    retStruct = InterfaceIpConfig(deviceObj=device1,
                                  interface="4",
                                  routing=True, config=False)

    devIntReturn = device1.DeviceInteract(command="vtysh")
    devIntReturn = device1.DeviceInteract(command="conf t")

    devIntReturn = device1.DeviceInteract(command="int 12.24545435612431562")
    retCode = devIntReturn.get('buffer')
    assert "Invalid input" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    devIntReturn = device1.DeviceInteract(command="no int 12.2")
    retCode = devIntReturn.get('buffer')
    assert "Interface does not exist" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    devIntReturn = device1.DeviceInteract(command="int 4.9")
    retCode = devIntReturn.get('buffer')
    assert "Parent interface is not L3" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    devIntReturn = device1.DeviceInteract(command="no int 4.2")
    devIntReturn = device1.DeviceInteract(command="no int 4.2")
    retCode = devIntReturn.get('buffer')
    assert "Interface does not exist" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    devIntReturn = device1.DeviceInteract(command="int 100.1025")
    retCode = devIntReturn.get('buffer')
    assert "Parent interface does not exist" in retCode, \
           'Test to verify sub-interface configuration clis - FAILED!'

    devIntReturn = device1.DeviceInteract(command="exit")
    devIntReturn = device1.DeviceInteract(command="exit")

    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get interface 4.2 name")
    lines_list= devIntReturn.get('buffer').splitlines()
    list = filter(None, lines_list)
    print list
    if len(list) >= 2:
        list = list[1]
    assert "no row \"4.2\" in table Interface" in list, \
           'Test to verify sub-interface configuration clis - FAILED!'
    LogOutput('info', "4.2  deleted successfully")


class Test_subintf_cli:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_subintf_cli.testObj = testEnviron(topoDict=topoDict)
        # Get topology object
        Test_subintf_cli.topoObj = Test_subintf_cli.testObj.topoObjGet()

    def teardown_class(cls):
        Test_subintf_cli.topoObj.terminate_nodes()

    def test_subintf_cli(self):
        LogOutput('info', "**configuring**")
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        wrkston1Obj = self.topoObj.deviceObjGet(device="wrkston01")
        retValue = subintf_cli(device1=dut01Obj, device2=wrkston1Obj)
