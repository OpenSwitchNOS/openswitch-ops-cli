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

import re

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""


'''
sort in alphanumeric order
'''
def alphanumeric_sort(l):
    convert = lambda text: int(text) if text.isdigit() else text
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
    return sorted(l, key = alphanum_key)



def test_vtysh_ct_loopback_intf_cli(topology, step):
    ops1 = topology.get('ops1')
    assert ops1 is not None

    ops1("config t")
    ops1("interface loopback 1")

    out = ops1("get interface loopback1 name", shell="vsctl")
    assert "loopback1" in out

    out = ops1("get port loopback1 name", shell="vsctl")
    assert "loopback1" in out

    ops1("ipv6 address 10:10::10:10/24")

    out = ops1("get port loopback1 ip6_address", shell="vsctl")
    assert "10:10::10:10/24" in out

    ops1("ip address 192.168.1.5/24")

    out = ops1("get port loopback1 ip4_address", shell="vsctl")
    assert "192.168.1.5/24" in out

    ops1("exit")
    ops1("no interface loopback 1")

    out = ops1("get interface loopback1 name", shell="vsctl")
    if len(out) == 3:
        out = out[1]
    assert "ovs-vsctl: no row \"loopback1\" in table Interface" in out

    ops1("int loopback 2")
    out = ops1("ip address 255.255.255.255/24")
    assert "Invalid IP address." in out

    ops1("int loopback 1")
    ops1("ip address 192.168.1.5/24")
    ops1("int loopback 2")
    out = ops1("ip address 192.168.1.5/24")
    assert "Overlapping networks observed for \"192.168.1.5/24\". "\
           "Please configure non overlapping networks." in out

    out = ops1("int loopback 21474836501")
    assert "% Unknown command." in out

    ops1("no int loopback 1")
    out = ops1("no int loopback 1")
    assert "Loopback interface does not exist." in out

    out = ops1("int loopback 0")
    assert "% Unknown command." in out

    out = ops1("no int loopback 6")
    assert "Loopback interface does not exist." in out

    # verify that result of show int loopback is in sorted order
    ops1("interface loopback 44")
    ops1("interface loopback 4")
    ops1("interface loopback 33")
    ops1("interface loopback 7")
    ops1("interface loopback 33")
    ops1("end")
    out = ops1("show interface loopback")
    lines = out.split('\n')
    interface_lines = [line for line in lines if "Interface" in line]

    ''' collect interface names '''
    interface_names = []
    for line in interface_lines:
       interface_names.append(line.split()[1])

    assert interface_names == alphanumeric_sort(interface_names)
