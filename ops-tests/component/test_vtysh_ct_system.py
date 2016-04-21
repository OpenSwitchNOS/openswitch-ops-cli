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
def test_vtysh_ct_system(topology, step):  # noqa
    step("1-Init System table with dummy data")
    # Add dummy data for LED in subsystem and led table for simulation.
    # Assume there would be only one entry in subsystem table
    sw1 = topology.get("sw1")
    output = sw1('list subsystem', shell='vsctl')
    output = output.split('\n')
    for line in output:
        if '_uuid' in line:
            id_ = line.split(':')
            uuid = id_[1].strip()
            sw1('ovs-vsctl -- set Subsystem {uuid} '
                ' leds=@led1 -- --id=@led1 create led '
                ' id=Led_base state=flashing status=ok'.format(**locals()),
                shell='bash')
            sw1('ovs-vsctl -- set Subsystem {uuid} '
                ' fans=@fan1 -- --id=@fan1 create fan '
                ' name=Fan_base speed=normal direction=f2b '
                ' rpm=9000 status=ok'.format(**locals()),
                shell='bash')
            sw1('ovs-vsctl -- set Subsystem {uuid} '
                ' power_supplies=@psu1 -- --id=@psu1 create '
                ' Power_supply name=Psu_base status=ok'.format(**locals()),
                shell='bash')
            sw1('ovs-vsctl -- set Subsystem {uuid} '
                ' temp_sensors=@tmp1 -- --id=@tmp1 create '
                ' Temp_sensor '
                ' name=Temp_base location=Chassis '
                ' temperature=20000 '
                ' status=normal fan_state=normal'.format(**locals()),
                shell='bash')
    step("2-Test to verify show system command")
    output = sw1('show system')
    counter = 0
    lines = output.split('\n')
    for line in lines:
        if 'OpenSwitch Version' in line:
            counter += 1
        if 'Manufacturer' in line:
            output = sw1('list Subsystem', shell='vsctl')
            lines_ = output.split('\n')
            manufacturer = None
            for line_ in lines_:
                if "other_info" in line_:
                    lines__ = line_.split(",")
                    manufacturer_value = lines__[9].split("=")
                    manufacturer_value = manufacturer_value[1].strip()
                    manufacturer = manufacturer_value.strip()
            if manufacturer is not None and manufacturer in line:
                counter += 1
        if "Interface Count" in line:
            output = sw1('list Subsystem', shell='vsctl')
            lines_ = output.split('\n')
            interface_count = None
            for line_ in lines_:
                if "other_info" in line_:
                    lines__ = line_.split(',')
                    interface_value = lines__[5].split('=')
                    interface_value = interface_value[1].strip()
                    interface_count = interface_value.strip().replace('"', "")
            if interface_count is not None and interface_count in line[:-32]:
                counter += 1
        if 'Max Interface Speed' in line:
            output = sw1('list Subsystem', shell='vsctl')
            lines_ = output.split('\n')
            interface_speed = None
            for line_ in lines_:
                if "other_info" in line_:
                    lines__ = line_.split(',')
                    interface_speed_value = lines__[12].split('=')
                    interface_speed_value = \
                        interface_speed_value[1].strip()
                    interface_speed = interface_speed_value.strip()
                    interface_speed = interface_speed.replace('"', "")
            if interface_speed is not None and interface_speed in line[42:]:
                counter += 1
        if "Led_base" in line:
            counter += 1
        if "Fan_base" in line:
            counter += 1
        if "Psu_base" in line:
            counter += 1
        if "Temp_base" in line:
            counter += 1
    assert counter is 8
