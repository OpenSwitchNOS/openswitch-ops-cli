# -*- coding: utf-8 -*-

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can rediTestribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is diTestributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; withoutputputputput even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, BoTeston, MA
# 02111-1307, USA.
from pytest import mark

TOPOLOGY = """
# +-------+
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def add_bgp_redistribute_ospf(sw1, step):
    step("Test to add bgp redistribute ospf configurations")
    redist_created = False
    sw1("router bgp 1")
    sw1("redistribute ospf")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute ospf" in line:
            redist_created = True
            break
    assert redist_created


def add_bgp_redistribute_connected(sw1, step):
    step("Test to add bgp redistribute connected configurations")
    redist_created = False
    sw1("configure terminal")
    sw1("router bgp 1")
    sw1("redistribute connected")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute connected" in line:
            redist_created = True
            break
    assert redist_created


def add_bgp_redistribute_static(sw1, step):
    step("Test to add bgp redistribute static configurations")
    redist_created = False
    sw1("router bgp 1")
    sw1("redistribute static")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute static" in line:
            redist_created = True
            break
    assert redist_created


def add_bgp_redistribute_static_route_map(sw1, step):
    step("Test to add bgp redistribute static route map configurations")
    redist_created = False
    sw1("route-map rm1 permit 1")
    sw1("router bgp 1")
    sw1("redistribute static route-map rm1")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute static route-map rm1" in line:
            redist_created = True
            break
    assert redist_created


def delete_bgp_redistribute_static_route_map(sw1, step):
    step("Test to delete bgp redistribute static route map configurations")
    redist_deleted = True
    sw1("router bgp 1")
    sw1("no redistribute static route-map rm1")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute static route-map rm1" in line:
            redist_deleted = False
            break
    assert redist_deleted


def delete_bgp_redistribute_static(sw1, step):
    step("Test to delete bgp redistribute static configurations")
    redist_deleted = True
    sw1("router bgp 1")
    sw1("no redistribute static")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute static" in line:
            redist_deleted = False
            break
    assert redist_deleted


def delete_bgp_redistribute_ospf(sw1, step):
    step("Test to delete bgp redistribute ospf configurations")
    redist_deleted = True
    sw1("router bgp 1")
    sw1("no redistribute ospf")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute ospf" in line:
            redist_deleted = False
            break
    assert redist_deleted


def delete_bgp_redistribute_connected(sw1, step):
    step("Test to delete bgp redistribute connected configurations")
    redist_deleted = True
    sw1("router bgp 1")
    sw1("no redistribute connected")
    dump = sw1("do show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "redistribute connected" in line:
            redist_deleted = False
            break
    assert redist_deleted

@mark.skipif(True, reason="skipped test case temporarly to avoid failure"
                          " as redistribute schema, cli and quagga support is being changed")
def test_vtysh_ct_bgp_redistribute(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    add_bgp_redistribute_connected(sw1, step)
    add_bgp_redistribute_ospf(sw1, step)
    add_bgp_redistribute_static(sw1, step)
    delete_bgp_redistribute_static(sw1, step)
    add_bgp_redistribute_static_route_map(sw1, step)
    delete_bgp_redistribute_static_route_map(sw1, step)
    delete_bgp_redistribute_ospf(sw1, step)
    delete_bgp_redistribute_connected(sw1, step)
