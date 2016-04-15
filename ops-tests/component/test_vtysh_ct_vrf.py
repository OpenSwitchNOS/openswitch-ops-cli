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

from pytest import mark

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


@mark.skipif(True, reason="Build image issue. Show running-config issue.")
def test_vrf_cli(topology, step):
    first_interface = '12'
    second_interface = '13'
    third_interface = '14'
    split_parent_1 = '49'
    split_parent_2 = '50'
    split_child_1 = '50-1'
    split_child_2 = '50-2'
    sw1 = topology.get('sw1')

    assert sw1 is not None

    step('### VRF add/delete validations ###')
    sw1('configure terminal')

    # Checking VRF name more than 32 characters
    ret = sw1('vrf thisisavrfnamewhichismorethan32characters')
    assert 'Non-default VRFs not supported' in ret

    # Adding another VRF
    ret = sw1('vrf thisisavrfnamewhichisexactly32c')
    assert 'Non-default VRFs not supported' in ret

    # Adding default VRF
    ret = sw1('vrf vrf_default')
    assert 'Default VRF already exists.' in ret

    # Deleting default VRF
    ret = sw1('no vrf vrf_default')
    assert 'Cannot delete default VRF.' in ret

    # Deleting VRF which does not exist
    ret = sw1('no vrf abcd')
    assert 'Non-default VRFs not supported' in ret

    step('### Attaching/detaching interface to/from vrf ###')
    # Attaching to default VRF
    sw1('interface ' + first_interface)
    ret = sw1("vrf attach vrf_default")
    assert 'Already attached to default VRF' in ret

    # Attaching to VRF which does not exist
    ret = sw1('vrf attach abcd')
    assert 'Non-default VRFs not supported' in ret

    # Attaching L2 interface to default VRF
    sw1('no routing')
    ret = sw1('vrf attach vrf_default')
    assert 'Interface ' + first_interface + ' is not L3.' in ret

    # Using routing to attach to default VRF
    sw1('routing')
    ret = sw1('do show vrf')
    assert '\t' + first_interface in ret

    # Detaching from default VRF
    ret = sw1("no vrf attach vrf_default")
    assert 'Cannot detach from default VRF.' in ret

    # Using no routing to check if detach passed
    sw1('no routing')
    ret = sw1('do show vrf')
    assert '\t' + first_interface not in ret

    # Checking multiple interfaces attach and delete
    sw1('interface ' + first_interface)
    sw1('routing')
    sw1('ip address 10.1.1.1/8')
    sw1('interface ' + second_interface)
    sw1('ip address 10.1.1.2/8')
    sw1('interface ' + third_interface)
    sw1('ip address 10.1.1.3/8')
    sw1('interface ' + second_interface)
    sw1('no routing')
    ret = sw1('do show vrf')
    assert '\t' + second_interface not in ret

    # Cleanup
    sw1('interface ' + first_interface)
    sw1('no routing')
    sw1('interface ' + third_interface)
    sw1('no routing')

    step('### VRF status up/no_internal_vlan validations ###')
    # ovs-vsctl command to create vlans
    sw1('ovs-vsctl set Subsystem base '
        'other_info:l3_port_requires_internal_vlan=1', shell='bash')

    # Configurig vlan range for a single vlan
    sw1('exit')
    sw1("vlan internal range 1024 1024 ascending")
    sw1('interface ' + first_interface)
    sw1('routing')
    sw1('ip address 10.1.1.1/8')
    sw1('interface ' + second_interface)
    sw1('routing')
    sw1('ip address 11.1.1.1/8')

    # Checking to see if up and no_internal_vlan cases are handled
    ret = sw1('do show vrf')
    ret = ret.replace(' ', '')
    assert '\t' + first_interface + 'up' in ret and \
        '\t' + second_interface + 'error:no_internal_vlan' in ret

    # Cleanup
    sw1('interface ' + first_interface)
    sw1('no routing')
    sw1('interface ' + second_interface)
    sw1('no routing')

    step('### Assign/remove IP address to/from interface ###')
    # Adding IP address to L2 interface
    sw1('interface ' + first_interface)
    ret = sw1('ip address 10.0.20.2/24')
    assert 'Interface ' + first_interface + ' is not L3.' in ret

    # Deleting IP address on an L3 interface which does not
    # have any IP address
    sw1('routing')
    ret = sw1('no ip address 10.0.30.2/24')
    assert 'No IP address configured on interface ' \
        + first_interface in ret

    # Configuring IP address on L3 interface
    sw1('ip address 10.0.20.2/24')
    ip = sw1('/usr/bin/ovs-vsctl get port ' + first_interface +
             ' ip4_address', shell='bash').strip()
    ip = ip.split('\n')
    assert ip[0] == '\"10.0.20.2/24\"'

    # Updating IP address on L3 interface
    sw1('ip address 10.0.20.3/24')
    ip = sw1('/usr/bin/ovs-vsctl get port ' + first_interface +
             ' ip4_address', shell='bash').strip()
    ip = ip.split('\n')
    assert ip[0] == '\"10.0.20.3/24\"'

    # Assigning existing primary IP address as primary
    ret = sw1("ip address 10.0.20.3/24")
    assert "IP address is already assigned to interface " + first_interface + \
        " as primary." in ret

    # Assigning existing secondary IP address as primary
    sw1("ip address 10.0.20.4/24 secondary")
    ret = sw1("ip address 10.0.20.4/24")
    assert "IP address is already assigned to interface " + first_interface + \
        " as secondary." in ret

    # Assigning existing primary IP address as secondary
    ret = sw1("ip address 10.0.20.3/24 secondary")
    assert "IP address is already assigned to interface " + first_interface + \
        " as primary." in ret

    # Assigning existing secondary IP address as secondary
    ret = sw1("ip address 10.0.20.4/24 secondary")
    assert "IP address is already assigned to interface " + first_interface + \
        " as secondary." in ret

    # Remove primary IP address before deleting secondary IP addresses
    ret = sw1("no ip address 10.0.20.3/24")
    assert "Delete all secondary IP addresses before deleting primary." in ret

    # Remove IP address on L3 interface by giving an IP address
    # that is not present
    sw1("no ip address 10.0.20.4/24 secondary")
    ret = sw1("no ip address 10.0.30.2/24")
    assert "IP address 10.0.30.2/24 not found." in ret

    # Remove IP address from L3 interface by giving correct IP address
    ret = sw1("no ip address 10.0.20.3/24")
    ip = sw1("get port " + first_interface +
             " ip4_address", shell='vsctl').strip()
    ip = ip.split('\n')
    assert ip[0] == '[]'

    # Deleting secondary IP address on an L3 interface which does not
    # have any secondary IP address
    ret = sw1("no ip address 10.0.30.2/24 secondary")
    assert "No secondary IP address configured on interface " + \
        first_interface in ret

    # Configuring multiple secondary IP addresses on L3 interface
    sw1('ip address 10.0.20.4/24 secondary')
    sw1('ip address 10.0.20.5/24 secondary')
    ip = sw1('get port ' + first_interface + ' ip4_address_secondary',
             shell='vsctl').strip()
    ip = ip.split('\n')
    assert ip[0] == '[\"10.0.20.4/24\", \"10.0.20.5/24\"]'

    # Deleting multiple secondary IP addresses on L3 interface
    sw1('no ip address 10.0.20.4/24 secondary')
    ip = sw1('get port ' + first_interface + ' ip4_address_secondary',
             shell='vsctl').strip()
    ip = ip.split('\n')
    assert ip[0] == '[\"10.0.20.5/24\"]'

    sw1('no ip address 10.0.20.5/24 secondary')
    ip = sw1('get port ' + first_interface + ' ip4_address_secondary',
             shell='vsctl').strip()
    ip = ip.split('\n')
    assert ip[0] == '[]'
    sw1('no routing')

    step('### Assign/remove IPv6 address to/from interface ###')
    # Adding IPv6 address to L2 interface
    sw1('interface ' + first_interface)
    ret = sw1('ipv6 address 2002::1/128')
    assert 'Interface ' + first_interface + ' is not L3.' in ret

    # Deleting IPv6 address on an L3 interface which does
    # not have any IPv6 address
    sw1('routing')
    ret = sw1('no ipv6 address 2002::1/128')
    assert 'No IPv6 address configured on interface ' + first_interface in ret

    # Configuring IPv6 address on L3 interface
    sw1('ipv6 address 2002::1/128')
    ipv6 = sw1('get port ' + first_interface + ' ip6_address',
               shell='vsctl').strip()
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '\"2002::1/128\"'

    # Updating IPv6 address on L3 interface
    sw1('ipv6 address 2001::1/128')
    ipv6 = sw1('get port ' + first_interface + ' ip6_address',
               shell='vsctl')
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '\"2001::1/128\"'

    # Assigning existing primary IPv6 address as primary
    ret = sw1("ipv6 address 2001::1/128")
    assert "IP address is already assigned to interface " + first_interface + \
        " as primary." in ret

    # Assigning existing secondary IPv6 address as primary
    sw1("ipv6 address 2001::2/128 secondary")
    ret = sw1("ipv6 address 2001::2/128")
    assert "IP address is already assigned to interface " + first_interface + \
        " as secondary." in ret

    # Assigning existing primary IPv6 address as secondary
    ret = sw1("ipv6 address 2001::1/128 secondary")
    assert "IP address is already assigned to interface " + first_interface + \
        " as primary." in ret

    # Assigning existing secondary IPv6 address as secondary
    ret = sw1("ipv6 address 2001::2/128 secondary")
    assert "IP address is already assigned to interface " + first_interface + \
        " as secondary." in ret

    # Remove primary IPv6 address before deleting secondary IP addresses
    ret = sw1("no ipv6 address 2001::1/128")
    assert "Delete all secondary IP addresses before deleting primary." in ret

    # Remove IPv6 address on L3 interface by giving an IPv6 address
    # that is not present
    sw1("no ipv6 address 2001::2/128 secondary")
    ret = sw1('no ipv6 address 2004::1/128')
    assert 'IPv6 address 2004::1/128 not found.' in ret

    # Removing IPv6 address from L3 interface giving correct IPv6 address
    ret = sw1('no ipv6 address 2001::1/128')
    ipv6 = sw1('get port ' + first_interface + ' ip6_address',
               shell='vsctl').strip()
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '[]'

    # Deleting secondary IPv6 address on an L3 interface which does not
    # have any secondary IPv6 address
    ret = sw1("no ipv6 address 2004::1/128 secondary")
    assert "No secondary IPv6 address configured on interface " + \
        first_interface in ret

    # Configuring multiple secondary IPv6 addresses on L3 interface
    sw1("ipv6 address 2001::2/128 secondary")
    sw1("ipv6 address 2001::3/128 secondary")
    ipv6 = sw1("get port " + first_interface + " ip6_address_secondary",
               shell='vsctl').strip()
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '[\"2001::2/128\", \"2001::3/128\"]'

    # Deleting multiple secondary IPv6 addresses on L3 interface
    sw1("no ipv6 address 2001::2/128 secondary")
    ipv6 = sw1("get port " + first_interface + " ip6_address_secondary",
               shell='vsctl').strip()
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '[\"2001::3/128\"]'

    sw1("no ipv6 address 2001::3/128 secondary")
    ipv6 = sw1("get port " + first_interface + " ip6_address_secondary",
               shell='vsctl').strip()
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '[]'
    sw1('no routing')

    step('### Testing routing/ no routing working ###')
    # Configuring IP, IPv6 addresses on L3 interface
    sw1('interface ' + first_interface)
    sw1("routing")
    sw1("ipv6 address 2002::1/128")
    sw1("ip address 10.1.1.1/8")
    ret = sw1("do show vrf")
    assert '\t' + first_interface in ret

    # Making L3 interface as L2
    sw1('no routing')
    ret = sw1("do show vrf")
    assert '\t' + first_interface not in ret

    # Checking if IP address removed
    ip = sw1('get port ' + first_interface + ' ip4_address',
             shell='vsctl').strip()
    ip = ip.split('\n')
    assert ip[0] == '[]'

    # Checking if IPv6 address removed
    ipv6 = sw1('get port ' + first_interface + ' ip6_address',
               shell='vsctl').strip()
    ipv6 = ipv6.split('\n')
    assert ipv6[0] == '[]'

    # Checking if no routing worked
    ret = sw1('ip address 10.1.1.1/8')
    assert 'Interface ' + first_interface + ' is not L3.' in ret

    step('### Testing show running-config output ###')
    # Modifying interface data to test show running-config
    sw1('interface ' + second_interface)
    sw1("routing")
    sw1("ipv6 address 2002::1/128")
    sw1("ip address 10.1.1.1/8")
    sw1("ip address 10.1.1.3/8 secondary")
    sw1("ipv6 address 2002::2/128 secondary")
    sw1('interface ' + third_interface)
    # sw1('lldp transmission')
    sw1('lldp transmit')
    sw1('exit')
    ret = sw1('do show running-config')
    output = ret.split('\n')

    intf1 = 'interface ' + first_interface
    intf2 = 'interface ' + second_interface
    intf3 = 'interface ' + third_interface + ' '
    intf1_output = output.index(intf1)
    intf2_output = output.index(intf2)
    intf3_output = output.index(intf3)
    assert 'no routing' in output[intf1_output + 1]

    assert 'ip address 10.1.1.1/8' in output[intf2_output + 1] \
        and 'ip address 10.1.1.3/8 secondary' \
        in output[intf2_output + 2] and 'ipv6 address 2002::1/128' \
        in output[intf2_output + 3] \
        and 'ipv6 address 2002::2/128 secondary' \
        in output[intf2_output + 4]

    assert 'no lldp reception' in output[intf3_output + 1] \
        and 'no routing' in output[intf3_output + 2]

    step('### Validate Layer 3 configurations on split interfaces ###')
    # Configure IP address on a parent interface that has been split
    sw1('interface ' + split_parent_1)

    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')
    ret = sw1('ip address 11.1.1.1/8')
    assert 'This interface has been split. Operation not allowed' in ret

    # Configure IP address on a parent interface that has not been split
    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('no split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')
    ret = sw1('ip address 11.1.1.1/8')
    ret_vrf = sw1('do show vrf')
    ret_ip = sw1('do show interface ' + split_parent_1)
    assert '\t' + split_parent_1 in ret_vrf and '11.1.1.1/8' in ret_ip

    # Split a parent interface that already has an IP address configured
    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')
    ret_vrf = sw1('do show vrf')
    ret_ip = sw1('do show interface ' + split_parent_1)
    assert '\t' + split_parent_1 not in ret_vrf and '11.1.1.1/8' not in ret_ip

    # Configure IP address on a child interface
    # whose parent has not been split
    sw1('interface ' + split_child_1)
    ret = sw1('ip address 12.1.1.1/8')
    assert 'This is a QSFP child interface whose parent interface has not ' \
        'been split. Operation not allowed' in ret

    # Configure IP address on a child interface
    # whose parent has been split
    sw1('interface ' + split_parent_2)

    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')
    sw1('interface ' + split_child_1)
    sw1('ip address 12.1.1.1/8')
    sw1('interface ' + split_child_2)
    sw1('ip address 13.1.1.1/8')
    ret_vrf = sw1('do show vrf')
    ret_ip1 = sw1('do show interface ' + split_child_1)
    ret_ip2 = sw1('do show interface ' + split_child_2)
    assert '\t' + split_child_1 in ret_vrf and '\t' + split_child_2 in \
        ret_vrf and '12.1.1.1/8' in ret_ip1 and '13.1.1.1/8' in ret_ip2

    # No Split a parent interface that has
    # child interfaces with Layer 3 configurations
    sw1('interface ' + split_parent_2)

    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('no split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')
    ret_vrf = sw1('do show vrf')
    ret_ip1 = sw1('do show interface ' + split_child_1)
    ret_ip2 = sw1('do show interface ' + split_child_2)
    assert '\t' + split_child_1 not in ret_vrf and '\t' + split_child_2 \
        not in ret_vrf and '12.1.1.1/8' not in ret_ip1 and '13.1.1.1/8' \
        not in ret_ip2

    # Check "no routing" case to see if port
    # row is removed on split
    sw1('interface ' + split_parent_2)
    sw1('no routing')
    ret_port = sw1('get port ' + split_parent_2 + ' name', shell='vsctl')
    assert split_parent_2 in ret_port

    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')
    ret_port = sw1('get port ' + split_parent_2 + ' name', shell='vsctl')
    assert 'ovs-vsctl: no row \"' + split_parent_2 + '\" in table Port' \
        in ret_port
