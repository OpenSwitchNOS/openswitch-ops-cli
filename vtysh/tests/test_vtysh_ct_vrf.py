#!/usr/bin/python

# (c) Copyright 2015-2016 Hewlett Packard Enterprise Development LP
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

from opsvsi.docker import *
from opsvsi.opsvsitest import *

first_interface = '12'
second_interface = '13'
third_interface = '14'
split_parent_1 = '49'
split_parent_2 = '50'
split_child_1 = '50-1'
split_child_2 = '50-2'


class vrfCLITest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        infra_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(infra_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_vrf_add_delete(self):
        '''
            Test VRF add and delete validations
        '''

        info("\n########## VRF add/delete validations ##########\n")
        s1 = self.net.switches[0]

        # OPS_TODO: When multiple VRF support is added, change the script
        # to include required validations.

        s1.cmdCLI("configure terminal")

        # Checking VRF name more than 32 characters

        ret = s1.cmdCLI('vrf thisisavrfnamewhichismorethan32characters')
        assert 'Non-default VRFs not supported' in ret, \
               'VRF name validation failed'
        info('### VRF name validation passed ###\n')

        # Adding another VRF

        ret = s1.cmdCLI('vrf thisisavrfnamewhichisexactly32c')
        assert 'Non-default VRFs not supported' in ret, \
               'VRF add validation failed'
        info('### VRF add validation passed ###\n')

        # Adding default VRF

        ret = s1.cmdCLI('vrf vrf_default')
        assert 'Default VRF already exists.' in ret, \
               'Default VRF add validation failed'
        info('### Default VRF add validation passed ###\n')

        # Deleting default VRF

        ret = s1.cmdCLI('no vrf vrf_default')
        assert 'Cannot delete default VRF.' in ret, \
               'VRF delete validation failed'
        info('### VRF delete validation passed ###\n')

        # Deleting VRF which does not exist

        ret = s1.cmdCLI('no vrf abcd')
        assert 'Non-default VRFs not supported' in ret, \
               'VRF lookup validation failed'
        info('### VRF lookup validation passed ###\n')

        # Cleanup

        s1.cmdCLI('exit')

    def test_interface(self):
        '''
            Test attaching/detaching interface to/from VRF
        '''

        info("\n########## Attaching/detaching interface "
             "to/from vrf ##########\n")
        s1 = self.net.switches[0]

        # Attaching to default VRF

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("vrf attach vrf_default")
        assert 'Already attached to default VRF' in ret, \
               'Adding default VRF validation failed'
        info('### Adding default VRF validation passed ###\n')

        # Attaching to VRF which does not exist

        ret = s1.cmdCLI("vrf attach abcd")
        assert 'Non-default VRFs not supported' in ret, \
               'VRF lookup validation failed'
        info('### VRF lookup validation passed### \n')

        # Attaching L2 interface to default VRF

        s1.cmdCLI('no routing')
        ret = s1.cmdCLI('vrf attach vrf_default')
        expected_output = 'Interface ' + first_interface \
            + ' is not L3.'
        assert expected_output in ret, 'L2 Interface validation failed'
        info('### L2 Interface validation passed ###\n')

        # Using routing to attach to default VRF

        s1.cmdCLI('routing')
        ret = s1.cmdCLI('do show vrf')
        expected_output = '\t' + first_interface
        assert expected_output in ret, 'Interface attach failed'
        info('### Interface attached successfully ###\n')

        # Detaching from default VRF

        ret = s1.cmdCLI("no vrf attach vrf_default")
        assert 'Cannot detach from default VRF.' in ret, \
               'VRF detach interface validation failed'
        info('### VRF detach interface validation passed ###\n')

        # Using no routing to check if detach passed

        s1.cmdCLI('no routing')
        ret = s1.cmdCLI('do show vrf')
        expected_output = '\t' + first_interface
        assert expected_output not in ret, 'Interface detach failed'
        info('### Interface detached successfully ###\n')
        s1.cmdCLI('exit')

        # Checking multiple interfaces attach and delete

        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('routing')
        s1.cmdCLI('ip address 10.1.1.1/8')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('ip address 10.1.1.2/8')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('ip address 10.1.1.3/8')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')
        ret = s1.cmdCLI('do show vrf')
        expected_output = '\t' + second_interface
        assert expected_output not in ret, \
            'Multiple Interface detach failed'
        info('### Multiple Interface detached successfully ###\n')

        # Cleanup

        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')

        s1.cmdCLI('exit')

    def test_vrf_no_internal_vlan(self):
        '''
            Test VRF status for up/no internal vlan
        '''

        info("\n########## VRF status up/no_internal_vlan validations"
             " ##########\n")
        s1 = self.net.switches[0]

        # ovs-vsctl command to create vlans
        s1.cmd("ovs-vsctl set Subsystem base "
               "other_info:l3_port_requires_internal_vlan=1")

        # Configurig vlan range for a single vlan
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("vlan internal range 1024 1024 ascending")
        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('routing')
        s1.cmdCLI('ip address 10.1.1.1/8')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('routing')
        s1.cmdCLI('ip address 11.1.1.1/8')
        s1.cmdCLI('exit')

        # Checking to see if up and no_internal_vlan cases are handled
        ret = s1.cmdCLI('do show vrf')
        ret = ret.replace(' ', '')
        expected_output = '\t' + first_interface + 'up'
        assert expected_output in ret, 'Interface status up failed'
        info('### Interface status up passed ###\n')
        expected_output = '\t' + second_interface \
            + 'error:no_internal_vlan'
        assert expected_output in ret, \
            'Interface status no_internal_vlan failed'
        info('### Interface status no_internal_vlan passed ###\n')

        # Cleanup

        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')

        s1.cmdCLI('exit')

    def test_ip(self):
        '''
            Test configuration of IP address for port
        '''

        info("\n########## Assign/remove IP address "
             "to/from interface ##########\n")
        s1 = self.net.switches[0]

        # Adding IP address to L2 interface

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('ip address 10.0.20.2/24')
        expected_output = 'Interface ' + first_interface \
            + ' is not L3.'
        assert expected_output in ret, 'IP address validation failed'
        info('### IP address validation passed ###\n')

        # Deleting IP address on an L3 interface which does not
        # have any IP address

        s1.cmdCLI('routing')
        ret = s1.cmdCLI('no ip address 10.0.30.2/24')
        expected_output = \
            'No IP address configured on interface ' \
            + first_interface
        assert expected_output in ret, \
            'IP address presence validation failed'
        info('### IP address presence validation passed ###\n')

        # Configuring IP address on L3 interface

        s1.cmdCLI('ip address 10.0.20.2/24')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip4_address'
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '10.0.20.2/24', 'IP address configuration failed'
        info('### IP address configured successfully ###\n')

        # Updating IP address on L3 interface

        s1.cmdCLI('ip address 10.0.20.3/24')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip4_address'
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '10.0.20.3/24', 'IP address update failed'
        info('### IP address updated successfully ###\n')

        # Assigning existing primary IP address as primary

        ret = s1.cmdCLI("ip address 10.0.20.3/24")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as primary."
        assert expected_output in ret, 'IP address duplicate check failed'

        # Assigning existing secondary IP address as primary

        s1.cmdCLI("ip address 10.0.20.4/24 secondary")
        ret = s1.cmdCLI("ip address 10.0.20.4/24")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as secondary."
        assert expected_output in ret, 'IP address duplicate check failed'

        # Assigning existing primary IP address as secondary

        ret = s1.cmdCLI("ip address 10.0.20.3/24 secondary")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as primary."
        assert expected_output in ret, 'IP address duplicate check failed'

        # Assigning existing secondary IP address as secondary

        ret = s1.cmdCLI("ip address 10.0.20.4/24 secondary")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as secondary."
        assert expected_output in ret, 'IP address duplicate check failed'
        info('### IP address duplicate checked successfully ###\n')

        # Remove primary IP address before deleting secondary IP addresses

        ret = s1.cmdCLI("no ip address 10.0.20.3/24")
        assert "Delete all secondary IP addresses before " \
               "deleting primary." in ret, \
               'IP address delete primary validation failed'
        info('### IP address delete primary validation passed ###\n')

        # Remove IP address on L3 interface by giving an IP address
        # that is not present

        s1.cmdCLI("no ip address 10.0.20.4/24 secondary")
        ret = s1.cmdCLI("no ip address 10.0.30.2/24")
        assert "IP address 10.0.30.2/24 not found." in ret, \
               'IP address delete validation failed'
        info('### IP address delete validation passed ###\n')

        # Remove IP address from L3 interface by giving correct IP address

        ret = s1.cmdCLI("no ip address 10.0.20.3/24")
        intf_cmd = "/usr/bin/ovs-vsctl get port " + first_interface \
                   + " ip4_address"
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '[]', 'IP address remove failed'
        info('### IP address removed successfully ###\n')

        # Deleting secondary IP address on an L3 interface which does not
        # have any secondary IP address

        ret = s1.cmdCLI("no ip address 10.0.30.2/24 secondary")
        expected_output = "No secondary IP address configured " \
                          "on interface " + first_interface
        assert expected_output in ret, \
            'Secondary IP address presence validation failed'
        info('### Secondary IP address presence validation passed ###\n')

        # Configuring multiple secondary IP addresses on L3 interface

        s1.cmdCLI('ip address 10.0.20.4/24 secondary')
        s1.cmdCLI('ip address 10.0.20.5/24 secondary')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip4_address_secondary'
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '[10.0.20.4/24, 10.0.20.5/24]', \
            'Secondary IP address add failed'
        info('### Secondary IP address added successfully ###\n')

        # Deleting multiple secondary IP addresses on L3 interface

        s1.cmdCLI('no ip address 10.0.20.4/24 secondary')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip4_address_secondary'
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '[10.0.20.5/24]', \
            'Secondary IP address delete failed'
        info('### Secondary IP address deleted successfully ###\n')
        s1.cmdCLI('no ip address 10.0.20.5/24 secondary')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip4_address_secondary'
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '[]', 'Secondary IP address remove failed'
        info('### Secondary IP address removed successfully ###\n')
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')

        # Cleanup

        s1.cmdCLI('exit')

    def test_ipv6(self):
        '''
            Test configuration of IPv6 address for port
        '''

        info("\n########## Assign/remove IPv6 address "
             "to/from interface ##########\n")
        s1 = self.net.switches[0]

        # Adding IPv6 address to L2 interface

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('ipv6 address 2002::1/128')
        expected_output = 'Interface ' + first_interface \
            + ' is not L3.'
        assert expected_output in ret, 'IPv6 address validation failed'
        info('### IPv6 address validation passed ###\n')

        # Deleting IPv6 address on an L3 interface which does
        # not have any IPv6 address

        s1.cmdCLI('routing')
        ret = s1.cmdCLI('no ipv6 address 2002::1/128')
        expected_output = \
            'No IPv6 address configured on interface ' \
            + first_interface
        assert expected_output in ret, \
            'IPv6 address presence validation failed'
        info('### IPv6 address presence validation passed ###\n')

        # Configuring IPv6 address on L3 interface

        s1.cmdCLI('ipv6 address 2002::1/128')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip6_address'
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '2002::1/128', \
            'IPv6 address configuration failed'
        info('### IPv6 address configured successfully ###\n')

        # Updating IPv6 address on L3 interface

        s1.cmdCLI('ipv6 address 2001::1/128')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip6_address'
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '2001::1/128', 'IPv6 address update failed'
        info('### IPv6 address updated successfully ###\n')

        # Assigning existing primary IPv6 address as primary

        ret = s1.cmdCLI("ipv6 address 2001::1/128")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as primary."
        assert expected_output in ret, \
            'IPv6 address duplicate check failed'

        # Assigning existing secondary IPv6 address as primary

        s1.cmdCLI("ipv6 address 2001::2/128 secondary")
        ret = s1.cmdCLI("ipv6 address 2001::2/128")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as secondary."
        assert expected_output in ret, \
            'IPv6 address duplicate check failed'

        # Assigning existing primary IPv6 address as secondary

        ret = s1.cmdCLI("ipv6 address 2001::1/128 secondary")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as primary."
        assert expected_output in ret, \
            'IPv6 address duplicate check failed'

        # Assigning existing secondary IPv6 address as secondary

        ret = s1.cmdCLI("ipv6 address 2001::2/128 secondary")
        expected_output = "IP address is already assigned to " \
                          "interface " + first_interface + " as secondary."
        assert expected_output in ret, \
            'IPv6 address duplicate check failed'
        info('### IPv6 address duplicate checked successfully ###\n')

        # Remove primary IPv6 address before deleting secondary IP addresses

        ret = s1.cmdCLI("no ipv6 address 2001::1/128")
        assert "Delete all secondary IP addresses before " \
               "deleting primary." in ret, \
               'IP address delete primary validation failed'
        info('### IPv6 address delete primary validation passed ###\n')

        # Remove IPv6 address on L3 interface by giving an IPv6 address
        # that is not present

        s1.cmdCLI("no ipv6 address 2001::2/128 secondary")
        ret = s1.cmdCLI('no ipv6 address 2004::1/128')
        assert 'IPv6 address 2004::1/128 not found.' in ret, \
            'IPv6 address delete validation failed'
        info('### IPv6 address delete validation passed ###\n')

        # Removing IPv6 address from L3 interface giving correct IPv6 address

        ret = s1.cmdCLI('no ipv6 address 2001::1/128')
        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip6_address'
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '[]', 'IPv6 address remove failed'
        info('### IPv6 address removed successfully ###\n')

        # Deleting secondary IPv6 address on an L3 interface which does not
        # have any secondary IPv6 address

        ret = s1.cmdCLI("no ipv6 address 2004::1/128 secondary")
        expected_output = "No secondary IPv6 address configured on " \
                          "interface " + first_interface
        assert expected_output in ret, \
            'Secondary IPv6 address presence validation failed'
        info('### Secondary IPv6 address presence validation passed ###\n')

        # Configuring multiple secondary IPv6 addresses on L3 interface

        s1.cmdCLI("ipv6 address 2001::2/128 secondary")
        s1.cmdCLI("ipv6 address 2001::3/128 secondary")
        intf_cmd = "/usr/bin/ovs-vsctl get port " + first_interface \
                   + " ip6_address_secondary"
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '[2001::2/128, 2001::3/128]', \
            'Secondary IPv6 address add failed'
        info('### Secondary IPv6 address added successfully ###\n')

        # Deleting multiple secondary IPv6 addresses on L3 interface

        s1.cmdCLI("no ipv6 address 2001::2/128 secondary")
        intf_cmd = "/usr/bin/ovs-vsctl get port " + first_interface \
                   + " ip6_address_secondary"
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '[2001::3/128]', \
            'Secondary IPv6 address delete failed'
        info('### Secondary IPv6 address deleted successfully ###\n')

        s1.cmdCLI("no ipv6 address 2001::3/128 secondary")
        intf_cmd = "/usr/bin/ovs-vsctl get port " + first_interface \
                   + " ip6_address_secondary"
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '[]', 'Secondary IPv6 address remove failed'
        info('### Secondary IPv6 address removed successfully ###\n')
        s1.cmdCLI('no routing')
        s1.cmdCLI('exit')

        # Cleanup

        s1.cmdCLI('exit')

    def test_toggle_l2_l3(self):
        '''
            Test routing / no routing commands for port
        '''

        info("\n########## Testing routing/ no routing "
             "working ##########\n")
        s1 = self.net.switches[0]

        # Configuring IP, IPv6 addresses on L3 interface

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface ' + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("routing")
        s1.cmdCLI("ipv6 address 2002::1/128")
        s1.cmdCLI("ip address 10.1.1.1/8")
        ret = s1.cmdCLI("do show vrf")
        expected_output = '\t' + first_interface
        assert expected_output in ret, \
            'Interface is not L3. "routing" failed'
        info('### Interface is L3. "routing" passed ###\n')

        # Making L3 interface as L2

        s1.cmdCLI('no routing')
        ret = s1.cmdCLI('do show vrf')
        expected_output = '\t' + first_interface
        assert expected_output not in ret, 'Show vrf validation failed'
        info('### Show vrf validation passed ###\n')

        # Checking if IP address removed

        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip4_address'
        ip = s1.ovscmd(intf_cmd).strip()
        assert ip == '[]', 'IP address remove failed'
        info('### IP address removed successfully ###\n')

        # Checking if IPv6 address removed

        intf_cmd = '/usr/bin/ovs-vsctl get port ' + first_interface \
            + ' ip6_address'
        ipv6 = s1.ovscmd(intf_cmd).strip()
        assert ipv6 == '[]', 'IPv6 address remove failed'
        info('### IPv6 address removed successfully ###\n')

        # Checking if no routing worked

        ret = s1.cmdCLI('ip address 10.1.1.1/8')
        expected_output = 'Interface ' + first_interface \
            + ' is not L3.'
        assert expected_output in ret, 'Attach to bridge failed'
        info('### Attached to bridge successfully ###\n')

        # Cleanup

        s1.cmdCLI('exit')

    def test_show_running_config(self):
        '''
            Test show running-config for vrf changes
        '''

        info("\n########## Testing show running-config "
             "output ##########\n")
        s1 = self.net.switches[0]

        # Modifying interface data to test show running-config

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("routing")
        s1.cmdCLI("ipv6 address 2002::1/128")
        s1.cmdCLI("ip address 10.1.1.1/8")
        s1.cmdCLI("ip address 10.1.1.3/8 secondary")
        s1.cmdCLI("ipv6 address 2002::2/128 secondary")
        s1.cmdCLI("exit")
        intf_cmd = "interface " + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('lldp transmit')
        s1.cmdCLI('exit')
        ret = s1.cmdCLI('do show running-config')
        output = ret.split('\n')

        intf1 = 'interface ' + first_interface
        intf2 = 'interface ' + second_interface
        intf3 = 'interface ' + third_interface + ' '
        intf1_output = output.index(intf1)
        intf2_output = output.index(intf2)
        intf3_output = output.index(intf3)

        assert 'no routing' in output[intf1_output + 1], \
            'Show running config for ' + intf1 + ' failed'
        info('### Show running config for ' + intf1 + ' passed ###\n')

        assert 'ip address 10.1.1.1/8' in output[intf2_output + 1] \
            and 'ip address 10.1.1.3/8 secondary' \
            in output[intf2_output + 2] and 'ipv6 address 2002::1/128' \
            in output[intf2_output + 3] \
            and 'ipv6 address 2002::2/128 secondary' \
            in output[intf2_output + 4], 'Show running config for ' \
            + intf2 + ' failed'
        info('### Show running config for ' + intf2 + ' passed ###\n')

        assert 'no lldp reception' in output[intf3_output + 1] \
            and 'no routing' in output[intf3_output + 2], \
            'Show running config for ' + intf3 + ' failed'
        info('### Show running config for ' + intf3 + ' passed ###\n')

        # Cleanup

        s1.cmdCLI('exit')

    def test_split_interfaces(self):
        '''
            Test Layer 3 operations on split interfaces
        '''

        info("\n########## Validate Layer 3 configurations "
             "on split interfaces ##########\n")
        s1 = self.net.switches[0]

        # Configure IP address on a parent interface that has been split

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface ' + split_parent_1
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        ret = s1.cmdCLI('ip address 11.1.1.1/8')
        assert 'This interface has been split. ' \
               'Operation not allowed' in ret, \
               'Split parent interface validation failed'
        info('### Split parent interface validation passed ###\n')

        # Configure IP address on a parent interface that has not been split

        s1.cmdCLI('no split', False)
        s1.cmdCLI('y')
        s1.cmdCLI('ip address 11.1.1.1/8')
        ret_vrf = s1.cmdCLI('do show vrf')
        expected_output = '\t' + split_parent_1
        intf_cmd = 'do show interface ' + split_parent_1
        ret_ip = s1.cmdCLI(intf_cmd)
        assert expected_output in ret_vrf \
            and '11.1.1.1/8' in ret_ip, \
            'Split parent interface configuration failed'
        info('### Split parent interface configured successfully ###\n')

        # Split a parent interface that already has an IP address configured

        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        ret_vrf = s1.cmdCLI('do show vrf')
        unexpected_output = '\t' + split_parent_1
        intf_cmd = 'do show interface ' + split_parent_1
        ret_ip = s1.cmdCLI(intf_cmd)
        assert unexpected_output not in ret_vrf and \
            '11.1.1.1/8' not in ret_ip, \
            'Split parent interface configuration removal failed'
        info('### Split parent interface layer 3 '
             'configuration removed successfully ###\n')
        s1.cmdCLI('exit')

        # Configure IP address on a child interface
        # whose parent has not been split

        intf_cmd = 'interface ' + split_child_1
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('ip address 12.1.1.1/8')
        assert 'This is a QSFP child interface whose ' \
               'parent interface has not been split. ' \
               'Operation not allowed' in ret, \
               'Split child interface validation failed'
        info('### Split child interface validation passed ###\n')
        s1.cmdCLI('exit')

        # Configure IP address on a child interface
        # whose parent has been split

        intf_cmd = 'interface ' + split_parent_2
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + split_child_1
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('ip address 12.1.1.1/8')
        s1.cmdCLI('exit')
        intf_cmd = 'interface ' + split_child_2
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('ip address 13.1.1.1/8')
        s1.cmdCLI('exit')
        ret_vrf = s1.cmdCLI('do show vrf')
        intf_cmd = 'do show interface ' + split_child_1
        ret_ip1 = s1.cmdCLI(intf_cmd)
        intf_cmd = 'do show interface ' + split_child_2
        ret_ip2 = s1.cmdCLI(intf_cmd)
        expected_output_1 = '\t' + split_child_1
        expected_output_2 = '\t' + split_child_2
        assert expected_output_1 in ret_vrf and \
            expected_output_2 in ret_vrf and \
            '12.1.1.1/8' in ret_ip1 and '13.1.1.1/8' in ret_ip2, \
            'Split child interface configuration failed'
        info('### Split child interface configured successfully ###\n')

        # No Split a parent interface that has
        # child interfaces with Layer 3 configurations

        intf_cmd = 'interface ' + split_parent_2
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no split', False)
        s1.cmdCLI('y')
        s1.cmdCLI('exit')
        ret_vrf = s1.cmdCLI('do show vrf')
        intf_cmd = 'do show interface ' + split_child_1
        ret_ip1 = s1.cmdCLI(intf_cmd)
        intf_cmd = 'do show interface ' + split_child_2
        ret_ip2 = s1.cmdCLI(intf_cmd)
        unexpected_output_1 = '\t' + split_child_1
        unexpected_output_2 = '\t' + split_child_2

        assert unexpected_output_1 not in ret_vrf and \
            unexpected_output_2 not in ret_vrf and \
            '12.1.1.1/8' not in ret_ip1 and \
            '13.1.1.1/8' not in ret_ip2, \
            'Split child interface configuration removal failed'
        info('### Split child interface layer 3 '
             'configurations removed successfully ###\n')

        # Check "no routing" case to see if port
        # row is removed on split
        intf_cmd = 'interface ' + split_parent_2
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('no routing')
        ovs_get_port_cmd = 'ovs-vsctl get port ' + split_parent_2 + ' name'
        ret_port = s1.ovscmd(ovs_get_port_cmd)
        assert split_parent_2 in ret_port, \
            'No routing command failed on split parent'
        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        ret_port = s1.ovscmd(ovs_get_port_cmd)
        expected_output = 'ovs-vsctl: no row ' + split_parent_2 + \
                          ' in table Port'
        assert expected_output in ret_port, \
            'Port row deletion failed on split command'
        info('### Port row deleted successfully on split command ###\n')

        # Cleanup

        s1.cmdCLI('exit')


class Test_vtysh_vrf:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_vrf.test = vrfCLITest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_vrf.test.net.stop()

    def test_vrf_add_delete(self):
        self.test.test_vrf_add_delete()

    def test_interface(self):
        self.test.test_interface()

    def test_vrf_no_internal_vlan(self):
        self.test.test_vrf_no_internal_vlan()

    def test_ip(self):
        self.test.test_ip()

    def test_ipv6(self):
        self.test.test_ipv6()

    def test_toggle_l2_l3(self):
        self.test.test_toggle_l2_l3()

    def test_show_running_config(self):
        self.test.test_show_running_config()

    def test_split_interfaces(self):
        self.test.test_split_interfaces()

    def __del__(self):
        del self.test
