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
def test_vtysh_ct_led(topology, step):
    sw1 = topology.get("sw1")
    step("1-Initializing Led Table with dummy data")
    output = sw1("list subsystem", shell='vsctl')
    lines = output.split("\n")
    for line in lines:
        if '_uuid' in line:
            id_ = line.split(":")
            uuid = id_[1].strip()
            sw1("ovs-vsctl -- set Subsystem {uuid} "
                " leds=@led1 -- --id=@led1 create led "
                " id=base1 state=flashing "
                "status=ok".format(**locals()),
                shell='bash')
            break
    step("2-Verifying led command")
    sw1("configure terminal")
    sw1("led base1 on")
    sw1("exit")
    output = sw1("list led base1", shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if "state" in line:
            assert "on" in line
            break
    step("3-Verifying show system led command")
    output = sw1("show system led")
    lines = output.split('\n')
    for line in lines:
        if "base1" in line:
            assert "on" in line or \
                   "off" in line or \
                   "flashing" in line
            break
    step("4-Verifying no led command")
    sw1("configure terminal")
    sw1("no led base1")
    sw1("exit")
    output = sw1("list led base1", shell='vsctl')
    lines = output.split("\n")
    for line in lines:
        if "state" in line:
            assert "off" in line
            break
    step("5-Verifying led via show running config")
    sw1("configure terminal")
    sw1("led base1 on")
    sw1("exit")
    output = sw1("show running-config")
    assert "led base1 on" in output
