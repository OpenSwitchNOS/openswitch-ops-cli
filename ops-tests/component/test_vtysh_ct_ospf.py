# -*- coding: utf-8 -*-
# (C) Copyright 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
##########################################################################

"""
OpenSwitch Test for switchd related configurations.
"""

# from pytest import set_trace
# from time import sleep

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""


def createrouterinstancetest(dut):
    dut("router ospf")
    out = dut("do show running-config")
    assert "router ospf" in out


def deleterouterinstancetest(dut):
    dut("no router ospf")
    out = dut("do show running-config")
    assert "router ospf" not in out


def setrouteridtest(dut):
    dut("router ospf")
    dut("router-id 1.1.1.1")
    cmdout = dut("do show ip ospf")
    assert '1.1.1.1' in cmdout


def unsetrouteridtest(dut):
    dut("router ospf")
    dut("no router-id")
    cmdout = dut("do show ip ospf")
    assert '1.1.1.1' not in cmdout


def setnetworkareaidtest(dut):
    dut("router ospf")
    dut("network 10.0.0.0/24 area 100")
    out = dut("do show running-config")
    assert "network 10.0.0.0/24 area 0.0.0.100" in out


def unsetnetworkareaidtest(dut):
    dut("router ospf")
    dut("no network 10.0.0.0/24 area 100")
    out = dut("do show running-config")
    assert "network 10.0.0.0/24 area 0.0.0.100" not in out


def runningconfigtest(dut):
    dut("router ospf")
    dut("router-id 1.2.3.4")
    dut("network 10.0.0.0/24 area 100")
    cmdout = dut("do show running-config")
    assert 'router-id 1.2.3.4' in cmdout
    assert 'network 10.0.0.0/24 area 0.0.0.100' in cmdout


def norunningconfigtest(dut):
    dut("router ospf")
    dut("no router-id")
    dut("no network 10.0.0.0/24 area 100")
    cmdout = dut("do show running-config")
    assert 'router-id 1.2.3.4' not in cmdout
    assert 'network 10.0.0.0/24 area 0.0.0.100' not in cmdout


def test_vtysh_ct_ospf(topology, step):
    ops1 = topology.get("ops1")
    assert ops1 is not None

    ops1("conf t")
    createrouterinstancetest(ops1)
    deleterouterinstancetest(ops1)
    setrouteridtest(ops1)
    unsetrouteridtest(ops1)
    setnetworkareaidtest(ops1)
    unsetnetworkareaidtest(ops1)
    runningconfigtest(ops1)
    norunningconfigtest(ops1)
