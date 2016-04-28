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
from pytest import mark

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""

@mark.skipif(True, reason="Disabling due to random gate job failures")
def test_vtysh_ct_loopback_intf_cli(topology, step):
    ops1 = topology.get('ops1')
    assert ops1 is not None

    ops1("config t")
    ops1("interface loopback 1")

    out = ops1("get interface lo1 name", shell="vsctl")
    assert "lo1" in out

    out = ops1("get port lo1 name", shell="vsctl")
    assert "lo1" in out

    ops1("ipv6 address 10:10::10:10/24")

    out = ops1("get port lo1 ip6_address", shell="vsctl")
    assert "10:10::10:10/24" in out

    ops1("ip address 192.168.1.5/24")

    out = ops1("get port lo1 ip4_address", shell="vsctl")
    assert "192.168.1.5/24" in out

    ops1("exit")
    ops1("no interface loopback 1")

    out = ops1("get interface lo1 name", shell="vsctl")
    if len(out) == 3:
        out = out[1]
    assert "ovs-vsctl: no row \"lo1\" in table Interface" in out

    ops1("int loopback 2")
    out = ops1("ip address 255.255.255.255/24")
    assert "Invalid IP address." in out

    ops1("int loopback 1")
    ops1("ip address 192.168.1.5/24")
    ops1("int loopback 2")
    out = ops1("ip address 192.168.1.5/24")
    assert "An interface with the same IP address or subnet or an overlapping"\
           " network\n192.168.1.5/24 already exists." in out

    out = ops1("int loopback 21474836501")
    assert "% Unknown command." in out

    ops1("no int loopback 1")
    out = ops1("no int loopback 1")
    assert "Loopback interface does not exist." in out

    out = ops1("int loopback 0")
    assert "% Unknown command." in out

    out = ops1("no int loopback 6")
    assert "Loopback interface does not exist." in out
