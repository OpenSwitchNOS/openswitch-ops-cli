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


TOPOLOGY = """
# +-------+
# |       |     +-------+
# |  hs1  <----->  sw1  |
# |       |     +-------+
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
[type=host name="Host 1"] hs1

# Links
hs1:if01 -- sw1:if01
"""


def test_vtysh_ct_db_intf(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    step("1-Creating database")
    for i in range(1, 49):
        output = sw1("ovs-vsctl list interface"
                     " {i}".format(**locals()),
                     shell='bash')
        assert "_uuid" in output
    sw1("configure terminal")
    for i in range(1, 49):
        sw1("interface {i}".format(**locals()))
        sw1("no shutdown")
        sw1("exit")
    step("2-Testing what the database retrieves")
    output = sw1("do show running-config")
    output = output.split("\n")
    for i in range(1, 49):
        line_check = "interface {i}".format(**locals())
        return_ = False
        for line in output:
            if line_check in line:
                return_ = True
                break
        assert return_
