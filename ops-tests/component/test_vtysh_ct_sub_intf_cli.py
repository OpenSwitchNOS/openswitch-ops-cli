# -*- coding: utf-8 -*-
#
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

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def test_subintf_cli(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    # creating a sub interface and verifying in port and interface table
    sw1('configure terminal')
    sw1("interface 4.2")
    sw1('no shutdown')

    step("### Verify the interface is created with same name for L3 port ###")
    out = sw1("ovs-vsctl get interface 4.2 name", shell='bash')
    assert "4.2" in out

    out = sw1("ovs-vsctl get port 4.2 name", shell='bash')
    assert "4.2" in out

    # configuring the ip address and verifying ip address in port table
    sw1("interface 4.3")
    sw1("ip address 192.168.1.2/24")
    sw1('no shutdown')

    out = sw1("ovs-vsctl get port 4.3 ip4_address", shell='bash')
    assert '192.168.1.2/24' in out

    # verifying in assigning Invalid Ip address
    sw1("interface 4.5")
    out = sw1("ip address 0.0.0.0/24")
    sw1('no shutdown')
    assert 'Invalid IP address' in out

    # verifying in assigning Invalid Ip address
    sw1("interface 4.5")
    out = sw1("ip address 255.255.255.255/24")
    sw1('no shutdown')
    assert 'Invalid IP address' in out

    # configuring the ipv6 address and verifying in port table
    sw1("interface 4.4")
    sw1("ipv6 address 10:10::10:10/24")
    sw1('no shutdown')
    out = sw1("ovs-vsctl get port 4.4 ip6_address", shell='bash')
    assert "10:10::10:10/24" in out

    # verifying same vlan for different subinterface
    sw1("interface 4.3")
    sw1("encapsulation dot1Q 100")

    sw1("interface 4.8")
    out = sw1("encapsulation dot1Q 100")
    assert "Encapsulation VLAN is already configured on interface 4.3." in out

    sw1("interface 4.8")
    out = sw1("ip address 192.168.1.2/24")
    sw1('no shutdown')
    assert "Overlapping networks observed for \"192.168.1.2/24\". "\
           "Please configure non overlapping networks." in out

    sw1("interface 4.8")
    sw1("no encapsulation dot1Q 100")
    out = sw1("do show running-config")
    assert "encapsulation dot1Q 100" in out

    sw1("interface 4")
    sw1('no routing')
    sw1('no shutdown')

    out = sw1("int 12.24545435612431562")
    assert "Invalid input" in out

    out = sw1("no int 12.2")
    assert "Interface does not exist" in out

    out = sw1("int 4.9")
    assert "Parent interface is not L3" in out

    sw1("no int 4.2")
    out = sw1("no int 4.2")
    assert "Interface does not exist" in out

    out = sw1("int 100.1025")
    assert "Parent interface does not exist" in out

    out = sw1("ovs-vsctl get interface 4.2 name", shell='bash')
    assert "no row \"4.2\" in table Interface" in out

    out = sw1("do show interface bridge_normal subinterface brief")
    assert "Unknown command" in out

    out = sw1("do show interface vlan12 subinterface brief")
    assert "Unknown command" in out

    out = sw1("do show interface 4.8 subinterface")
    assert "Unknown command" in out

    out = sw1("do show interface 44 subinterface")
    assert "No sub-interfaces configured for interface 44" in out
