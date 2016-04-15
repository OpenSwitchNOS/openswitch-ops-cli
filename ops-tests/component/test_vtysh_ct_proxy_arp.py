# -*- coding: utf-8 -*-

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

from pytest import mark

TOPOLOGY = """
# +-------+
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def proxy_arp_on_l3_int(sw1):
    sw1("interface 1")
    sw1("ip proxy-arp")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' in output
    sw1("no ip proxy-arp")
    sw1("exit")
    output = sw1("do show running-config interface 1")
    assert 'ip proxy-arp' not in output
    output = sw1("do show interface 1")
    assert 'Proxy ARP is enabled' not in output


def proxy_arp_on_no_routing(sw1):
    sw1("interface 1")
    sw1("no routing")
    sw1("ip proxy-arp")
    sw1("exit")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' not in output


def proxy_arp_on_l3_vlan_int(sw1):
    sw1("interface vlan 1")
    sw1("ip proxy-arp")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' in output
    sw1("no ip proxy-arp")
    sw1("exit")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' not in output


def proxy_arp_on_split_child_int(sw1):
    sw1("interface 50")
    sw1("split \n y")
    sw1("interface 50-1")
    sw1("ip proxy-arp")
    output = sw1("do show interface 50-1")
    assert 'Proxy ARP is enabled' in output
    sw1("no ip proxy-arp")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' not in output
    sw1("ip proxy-arp")
    sw1("interface 50")
    sw1("no split \n y")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' not in output
    sw1("interface 51-1")
    sw1("ip proxy-arp")
    assert 'This is a QSFP child interface whose'
    'parent interface has not been split.' in output
    sw1("exit")


def proxy_arp_on_parent_int(sw1):
    sw1("interface 51")
    sw1("ip proxy-arp")
    output = sw1("do show interface 51")
    assert 'Proxy ARP is enabled' in output
    sw1("no ip proxy-arp")
    output = sw1("do show running-config")
    assert 'ip proxy-arp' not in output
    sw1("split \n yes")
    output = sw1("ip proxy-arp")
    assert 'This interface has been split. Operation not allowed' in output


@mark.skipif(True, reason="Skipping cause ip proxy arp is not present on "
                          "current image")
def test_vtysh_ct_proxy_arp(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    sw1("configure terminal")
    proxy_arp_on_l3_int(sw1)
    proxy_arp_on_no_routing(sw1)
    proxy_arp_on_l3_vlan_int(sw1)
    proxy_arp_on_parent_int(sw1)
    proxy_arp_on_split_child_int(sw1)
