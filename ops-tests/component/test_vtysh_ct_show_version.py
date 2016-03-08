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
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def set_os_name(os_name, uuid, sw1):
    if os_name:
        sw1('set system {0} software_info:os_name="{1}"'
            ''.format(uuid, os_name), shell='vsctl')
    else:
        sw1('remove system {0} software_info os_name'
            ''.format(uuid), shell='vsctl')


def set_version(version, uuid, sw1):
    sw1('ovs-vsctl -- set system {0} switch_version="{1}"'
        ''.format(uuid, version), shell='bash')


def check_show_version(sw1, uuid, os_name="OpenSwitch", version="0.1.0"):
    set_os_name(os_name, uuid, sw1)
    set_version(version, uuid, sw1)
    assert "{0} {1}".format(os_name, version) in sw1('show version')


def test_vtysh_ct_show_version(topology, step):
    # old_switch_version = None
    # old_os_name = None
    uuid = None
    sw1 = topology.get("sw1")
    step("1-Setting up the initial values")
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if '_uuid' in line:
            uuid = line.split(':')[1].strip()
            break
    """
        elif 'switch_version' in line:
            old_switch_version = line.split(':')[1].strip()
        elif 'software_info' in line:
            kvs = line.split(':')[1].strip(' {}\r\n')
            if kvs:
                for kv in kvs.split(', '):
                    key, value = kv.split('=')
                    if 'os_name':
                        old_os_name = value
    """
    assert sw1 is not None
    assert uuid is not None
    step("2-Checking OS Name")
    sw1('list system', shell='vsctl')
    check_show_version(sw1, uuid, os_name="TestOS", version="0.1.0")
    step("3-OS Name with whitespace")
    check_show_version(sw1, uuid, os_name="TestVenter TestOS", version="0.1.0")
    step("4-Normal Version")
    check_show_version(sw1, uuid, os_name="OpenSwitch", version="1.1.1")
    step("5-String Version")
    check_show_version(sw1, uuid, os_name="OpenSwitch", version="string_ver")
