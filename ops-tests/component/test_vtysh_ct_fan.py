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

@mark.skipif(True, reason="Disabling due to gate job failures")
def test_vtysh_ct_fan(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    step("1-Initializing Fan Table with dummy data")
    output = sw1("list subsystem", shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if "_uuid" in line:
            id_ = line.split(":")
            uuid = id_[1].strip()
            sw1("ovs-vsctl -- set Subsystem {uuid} "
                " fans=@fan1 -- --id=@fan1 create Fan "
                " name=base-FAN-1L direction=f2b "
                " speed=normal "
                " status=ok rpm=9000".format(**locals()),
                shell='bash')
            break
    step("2-verifying show system fan command")
    output = sw1("show system fan")
    lines = output.split('\n')
    counter = 0
    for line in lines:
        if "base-FAN-1L" in line:
            counter += 1
        if "front-to-back" in line:
            counter += 1
        if "normal" in line:
            counter += 1
        if "ok" in line:
            counter += 1
        if "9000" in line:
            counter += 1
    assert counter is 5
    step("3-Verifying fan-speed command")
    sw1("configure terminal")
    sw1("fan-speed slow")
    output = sw1("do show system fan")
    sw1("exit")
    assert 'Fan speed override is set to : slow' in output
    step("4-Verifying show running config for fan speed")
    output = sw1("show running-config")
    assert 'fan-speed slow' in output
    step("5-Verifying no fan speed command")
    sw1("configure terminal")
    sw1("no fan-speed")
    output = sw1("do show system fan")
    sw1("exit")
    assert "Fan speed override is not configured" in output
