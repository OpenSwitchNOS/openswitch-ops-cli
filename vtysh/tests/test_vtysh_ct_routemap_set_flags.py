#!/usr/bin/python

# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
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
import math
from opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *
from opstestfw.switch.CLI.InterfaceIpConfig import InterfaceIpConfig
from opsvsiutils.vtyshutils import *

'''
This code is CT for route-map commands:
set local-preference
set weight
set comm-list delete
set aggregator as
set atomic-aggregate

TOPOLOGY
+---------------+             +---------------+
|               |             |               |
| Switch 1     1+-------------+1  Switch 2    |
|               |             |               |
|               |             |               |
+---------------+             +---------------+

switch 1 configuration
----------------------

#  router bgp 1
#  bgp router-id 8.0.0.1
#  network 9.0.0.0/8
#  neighbor 8.0.0.2 remote-as 2
#  neighbor 8.0.0.2 route-map 1 in

#  interface 1
#  no shutdown
#  ip address 8.0.0.1/8
switch 2 configuration
----------------------

# router bgp 2
#  bgp router-id 8.0.0.2
#  network 10.0.0.0/8
#  network 11.0.0.0/8
#  neighbor 8.0.0.1 remote-as 1

interface 1
    no shutdown
    ip address 8.0.0.2/8

'''

IP_ADDR1 = "8.0.0.1"
IP_ADDR2 = "8.0.0.2"

DEFAULT_PL = "8"

SW1_ROUTER_ID = "8.0.0.1"
SW2_ROUTER_ID = "8.0.0.2"

AS_NUM1 = "1"
AS_NUM2 = "2"

VTYSH_CR = '\r\n'
MAX_WAIT_TIME = 100
# Topology definition
topoDict = {"topoExecution": 5000,
            "topoTarget": "dut01 dut02",
            "topoDevices": "dut01 dut02",
            "topoLinks": "lnk01:dut01:dut02",
            "topoFilters": "dut01:system-category:switch,\
                            dut02:system-category:switch"}


def enterConfigShell(dut):
    retStruct = dut.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"
    return True

# If the context is not present already then it will be created
def enterRouterContext(dut,as_num):
    if (enterConfigShell(dut) is False):
        return False

    devIntReturn = dut.DeviceInteract(command="router bgp "+ as_num)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter BGP context"
    return True

def enterNoRouterContext(dut,as_num):
    if (enterConfigShell(dut) is False):
        return False

    devIntReturn = dut.DeviceInteract(command="no router bgp "+ as_num)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to exit BGP context"
    return True

def exitContext(dut):
    devIntReturn = dut.DeviceInteract(command="exit")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to exit current context"

    retStruct = dut.ConfigVtyShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit config terminal"

    retStruct = dut.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"
    return True

def configure_router_id(dut, as_num, router_id):
    if (enterRouterContext(dut, as_num) is False):
        return False

    LogOutput('info', "Configuring BGP router ID " + router_id)
    devIntReturn = dut.DeviceInteract(command="bgp router-id " + router_id)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set router-id failed"
    return True

def configure_network(dut, as_num, network):
    if (enterRouterContext(dut, as_num) is False):
        return False

    cmd = "network " + network
    devIntReturn = dut.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set network failed"
    return True

def configure_neighbor(dut, as_num1, network, as_num2):
    if (enterRouterContext(dut, as_num1) is False):
        return False

    cmd = "neighbor "+network+" remote-as "+as_num2
    devIntReturn = dut.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set neighbor config failed"
    return True

def configure_route_map_set_commands(dut, routemap):
    if (enterConfigShell(dut) is False):
        return False
    cmd = "route-map "+routemap+" permit 20"
    cmd1 = "set community 2:0 3:0 4:0"
    cmd3 = "set comm-list test delete"
    cmd4 = "set weight 4"
    cmd5 = "set aggregator as 1 8.0.0.1"
    cmd6 = "set atomic-aggregate"
    cmd7 = "set local-preference 22"
    devIntReturn = dut.DeviceInteract(command=cmd)
    devIntReturn = dut.DeviceInteract(command=cmd1)
    devIntReturn = dut.DeviceInteract(command=cmd3)
    devIntReturn = dut.DeviceInteract(command=cmd4)
    devIntReturn = dut.DeviceInteract(command=cmd5)
    devIntReturn = dut.DeviceInteract(command=cmd6)
    devIntReturn = dut.DeviceInteract(command=cmd7)
    return True


def verify_routemap_set(**kwargs):
    LogOutput('info',"\n\n########## Verifying route-map set commands CT##########\n")

    switch1 = kwargs.get('switch1', None)
    switch2 = kwargs.get('switch2', None)

    LogOutput('info',"Configuring no router bgp on SW2")
    result = enterNoRouterContext(switch2,AS_NUM2)
    assert result is True,"Failed to configure router Context on SW2"

    LogOutput('info',"Configuring route-map on SW1")
    result=configure_route_map_set_commands(switch1,"BGP_IN2")
    assert result is True, "Failed to configure route-map on SW1"

    set_community_str = "set community 2:0 3:0 4:0"
    set_community_flag = False
    set_commlist_str = "set comm-list test delete"
    set_commlist_flag = False
    set_weight_str = "set weight 4"
    set_weight_flag = False
    set_aggregator_str = "set aggregator as 1 8.0.0.1"
    set_aggregator_flag = False
    set_atomic_str = "set atomic-aggregate"
    set_atomic_flag = False
    set_localpref_str = "set local-preference"
    set_localpref_flag = False

    exitContext(switch1)
    dump = SwitchVtyshUtils.vtysh_cmd(switch1, "show running-config")
    lines = dump.split('\n')
    for line in lines:
        if set_community_str in line:
            set_community_flag = True
        elif set_commlist_str in line:
            set_commlist_flag = True
        elif set_weight_str in line:
            set_weight_flag = True
        elif set_aggregator_str in line:
            set_aggregator_flag = True
        elif set_atomic_str in line:
            set_atomic_flag = True
        elif set_localpref_str in line:
            set_localpref_flag = True

    assert set_community_flag == True, "Failed to configure 'set community'"
    LogOutput('info',"'set community' running succesfully")
    assert set_commlist_flag == True, "Failed to configure 'set comm-list'"
    LogOutput('info',"'set comm-list' running succesfully")
    assert set_weight_flag == True, "Failed to configure 'set weight'"
    LogOutput('info',"'set weight' running succesfully")
    assert set_aggregator_flag == True, "Failed to configure 'set aggregator as'"
    LogOutput('info',"'set aggregator as' running succesfully")
    assert set_atomic_flag == True, "Failed to configure 'set atomic'"
    LogOutput('info',"'set atomic' running succesfully")
    assert set_localpref_flag == True, "Failed to configure 'set local-preference'"
    LogOutput('info',"'set local-preference' running succesfully")




def configure(**kwargs):
    '''
     - Configures the IP address in SW1, SW2 and SW3
     - Creates router bgp instance on SW1 and SW2
     - Configures the router id
     - Configures the network range
     - Configure redistribute and neighbor
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
                                  addr=IP_ADDR1, mask=DEFAULT_PL,
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
                                  addr=IP_ADDR2, mask=DEFAULT_PL,
                                  config=True)
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to configure an IPv4 address on interface 1 of SW2"

class Test_bgp_redistribute_configuration:
    def setup_class(cls):
        Test_bgp_redistribute_configuration.testObj = \
            testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_bgp_redistribute_configuration.topoObj = \
            Test_bgp_redistribute_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_bgp_redistribute_configuration.topoObj.terminate_nodes()

    def test_configure(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        dut02Obj = self.topoObj.deviceObjGet(device="dut02")

        configure(switch1=dut01Obj, switch2=dut02Obj)

        verify_routemap_set(switch1=dut01Obj, switch2=dut02Obj)
