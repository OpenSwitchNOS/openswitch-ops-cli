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


def test_vtysh_ct_tempsensor(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    step("1-Initializing the temperature sensor with dummy data")
    output = sw1("list subsystem", shell="vsctl")
    output = output.split("\n")
    for line in output:
        if '_uuid' in line:
            id_ = line.split(":")
            uuid = id_[1].strip()
            sw1("ovs-vsctl -- set Subsystem {uuid} "
                " temp_sensors=@fan1 -- --id=@fan1 "
                " create Temp_sensor name=base-1 "
                " location=Faceplate_side_of_switch_chip_U16 "
                " status=normal fan-state=normal min=0 "
                " max=21000 temperature=20500".format(**locals()),
                shell="bash")
            break
    step("2-Verifying the system temperature via show system")
    output = sw1("show system temperature detail")
    output = output.split("\n")
    counter = 0
    for line in output:
        if "base-1" in line:
            counter += 1
        if "Faceplate_side_of_switch_chip_U16" in line:
            counter += 1
        if "normal" in line:
            counter += 1
    assert counter is 3
