# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def test_source_interface_configuration(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    step('Validation of source IP address to all the defined protocols')
    sw1('configure terminal')
    out = sw1("ip source-interface all address 255.255.255.255")
    assert 'Broadcast, multicast and loopback addresses are not allowed.' \
        in out

    out = sw1("ip source-interface all address 224.0.0.0")
    assert 'Broadcast, multicast and loopback addresses are not allowed.' \
        in out

    out = sw1("ip source-interface all address 127.0.0.1")
    assert 'Broadcast, multicast and loopback addresses are not allowed.' \
        in out

    step('Validation of source IP address to tftp protocol')
    sw1("ip source-interface tftp address 255.255.255.255")
    assert 'Broadcast, multicast and loopback addresses are not allowed.' \
        in out

    sw1("ip source-interface tftp address 224.0.0.0")
    assert 'Broadcast, multicast and loopback addresses are not allowed.' \
        in out

    sw1("ip source-interface tftp address 127.0.0.1")
    assert 'Broadcast, multicast and loopback addresses are not allowed.' \
        in out

    step('Set source IP address to all the defined protocols')
    sw1("ip source-interface all address 1.1.1.1")
    out = sw1("do show ip source-interface")
    assert '1.1.1.1' in out

    step('Set source IP address to tftp protocol')
    sw1("ip source-interface tftp address 1.1.1.1")
    out = sw1("do show ip source-interface tftp")
    assert '1.1.1.1' in out

    step('Set source IP interface to all the defined protocols')
    sw1("ip source-interface all interface 2")
    out = sw1("do show ip source-interface")
    assert '2' in out

    step('Set source IP interface to tftp protocol')
    sw1("ip source-interface tftp interface 2")
    out = sw1("do show ip source-interface tftp")
    assert '2' in out

    step('Unset source IP to all the defined protocols')
    sw1("no ip source-interface all")
    out = sw1("do show ip source-interface")
    assert '(null)' in out

    step('Unset source IP to tftp protocols')
    sw1("no ip source-interface tftp")
    out = sw1("do show ip source-interface tftp")
    assert '(null)' in out
