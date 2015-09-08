#!/usr/bin/python
# -*- coding: utf-8 -*-

# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

from opsvsi.docker import *
from opsvsi.opsvsitest import *

first_interface = '1'
second_interface = '2'
third_interface = '3'
error_str = \
    'Error: Invalid vlan id. Enter valid vlan id in the range of <1 to 4094> and should not be part of internal vlan'
max_vlan = '4095'
min_vlan = '0'


# Internal vlan test is performed in vlan CT #

class intervlanCLITest(OpsVsiTest):

    def setupNet(self):
        self.net = Mininet(
            topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                  sopts=self.getSwitchOpts()),
            switch=VsiOpenSwitch,
            host=OpsVsiHost,
            link=OpsVsiLink,
            controller=None,
            build=True,
            )

    def test_intervlan_input_test(self):
        '''
            Test invalid interface vlan input parameters.
        '''

        info('''
########## Test invalid vlan interface names ##########
''')
        s1 = self.net.switches[0]

        s1.cmdCLI('configure terminal')

        # Checking out of range Vlan inputs

        intf_cmd = 'interface vlan ' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str in ret, \
            'Adding Vlan id = %s validation failed' % max_vlan
        intf_cmd = 'interface vlan' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert 'Unknown command.' in ret, \
            'Adding Vlan id = %s validation failed' % max_vlan
        intf_cmd = 'interface vlan ' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str in ret, 'Adding Vlan id = 0 validation failed'
        intf_cmd = 'interface vlan' + min_vlan
        ret = s1.cmdCLI(min_vlan)
        assert 'Unknown command.' in ret, \
            'Adding Vlan id = 0 validation failed'
        info('### Vlanid outside range <1-4094> validation passed ###\n'
             )

        # Checking invalid interface vlan input parameters (vlanid: 2abc & abc2abc & abc#$)

        ret = s1.cmdCLI('interface vlan 2abc ')
        assert error_str in ret, 'Vlan id = 2abc validation failed'
        ret = s1.cmdCLI('interface vlan2abc')
        assert 'Unknown command.' in ret, \
            'Vlan id = 2abc validation failed'
        ret = s1.cmdCLI('interface vlan abc2abc ')
        assert error_str in ret, 'Vlan id = abc2abc validation failed'
        ret = s1.cmdCLI('interface vlanabc2abc')
        assert 'Unknown command.' in ret, \
            'Vlan id = abc2abc validation failed'
        ret = s1.cmdCLI('interface vlan abc#$ ')
        assert error_str in ret, 'Vlan id = abc#$ validation failed'
        ret = s1.cmdCLI('interface vlanabc#$')
        assert 'Unknown command.' in ret, \
            'Vlan id = abc#$ validation failed'
        info('### Successfully verified invalid vlanid (vlanid: 2abc & abc2abc & abc#$) ###\n'
             )

        # Deleting interface vlan outside range <1-4094>

        intf_cmd = 'no interface vlan ' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str in ret, \
            'Deleting vlan id = %s validation failed' % max_vlan
        intf_cmd = 'no interface vlan' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert 'Unknown command.' in ret, \
            'Deleting vlan id = %s validation failed' % max_vlan
        intf_cmd = 'no interface vlan ' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str in ret, \
            'Deleting vlan id = %s validation failed' % min_vlan
        intf_cmd = 'no interface vlan' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert 'Unknown command.' in ret, \
            'Deleting vlan id = %s validation failed' % min_vlan
        info('### Deleting out of range vlanid validation passed ###\n')

        # Cleanup

        s1.cmdCLI('exit')

    def test_vlan_interface_add_delete(self):
        '''
            Test add and delete vlan interface
        '''

        info('''
########## Add/Delete vlan interface and associate with VRF, bridge and DB ##########
''')

        s1 = self.net.switches[0]

        s1.cmdCLI('configure terminal')

        # Adding vlan interface and verify if VRF can see it

        intf_cmd = 'interface vlan ' + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('do show vrf')
        expected_output = 'vlan' + first_interface
        assert expected_output in ret, 'Failed to add vlan interface'
        info('### Add vlan interface and attach to VRF validation passed ###\n'
             )
        s1.cmdCLI('exit')

        # Verify we see the interface and port created in OVSDB and attached
        # to bridge with name same as Interface Name and type as internal

        intf_cmd = 'interface vlan ' + second_interface
        s1.cmdCLI(intf_cmd)

        # Verify interface name

        intf_cmd = '/usr/bin/ovs-vsctl get interface vlan%s name' \
            % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'vlan' + second_interface
        assert expected_output in listcmd, \
            'Failed to add interface to DB'

        # Verify interface type

        intf_cmd = '/usr/bin/ovs-vsctl get interface vlan%s type' \
            % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        assert 'internal' in listcmd, 'Failed to add interface to DB'

        # Verify port name

        intf_cmd = '/usr/bin/ovs-vsctl get port vlan%s name' \
            % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'vlan' + second_interface
        assert expected_output in listcmd, 'Failed to add port to DB'

        # verify interface uuid in port row

        intf_cmd = '/usr/bin/ovs-vsctl get interface vlan%s _uuid' \
            % second_interface
        uuid = s1.ovscmd(intf_cmd).strip()
        intf_cmd = '/usr/bin/ovs-vsctl get port vlan%s interfaces' \
            % second_interface
        portlist = s1.ovscmd(intf_cmd).strip()
        assert uuid in portlist, 'Failed to add port to DB'

        # Verify port in bridge

        intf_cmd = '/usr/bin/ovs-vsctl get port vlan%s _uuid' \
            % second_interface
        port_uuid = s1.ovscmd(intf_cmd).strip()
        intf_cmd = '/usr/bin/ovs-vsctl get bridge bridge_normal ports'
        portlist = s1.ovscmd(intf_cmd).strip()
        assert port_uuid in portlist, 'Failed to add port to DB'

        # Verify port in vrf

        intf_cmd = '/usr/bin/ovs-vsctl get port vlan%s _uuid' \
            % second_interface
        port_uuid = s1.ovscmd(intf_cmd).strip()
        intf_cmd = '/usr/bin/ovs-vsctl get vrf vrf_default ports'
        portlist = s1.ovscmd(intf_cmd).strip()
        assert port_uuid in portlist, 'Failed to add port to DB'
        info('### Adding interface and port to DB validation passed ### \n'
             )

        # Deleting interface vlan and verify if VRF can see it

        intf_cmd = 'no interface vlan ' + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('do show vrf')
        expected_output = 'vlan' + first_interface
        assert expected_output not in ret, \
            'Failed to delete vlan interface'
        info('### Deleting vlan interface and detach from VRF validation passed ###\n'
             )

        # Deleting non existing vlan interface

        intf_cmd = 'no interface vlan ' + first_interface
        ret = s1.cmdCLI(intf_cmd)
        assert 'Vlan interface does not exist. Cannot delete' in ret, \
            'Able to delete non existing vlan interface'
        info('### Deleting non existing vlan interface validation passed ###\n'
             )

        # Deleting vlan interface from OVSDB with name same as Interface Name

        intf_cmd = 'no interface vlan ' + second_interface
        s1.cmdCLI(intf_cmd)

        # Check for interface name in OVSDB

        intf_cmd = '/usr/bin/ovs-vsctl get interface vlan%s name' \
            % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'no row vlan%s in table Interface' \
            % second_interface
        assert expected_output in listcmd, \
            'Failed to delete vlan interface in DB'

        # Check for port name in OVSDB

        intf_cmd = '/usr/bin/ovs-vsctl get port vlan%s name' \
            % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'no row vlan%s in table Port' \
            % second_interface
        assert expected_output in listcmd, \
            'Failed to delete vlan interface in DB'
        info('### Deleting vlan interface from DB validation passed ### \n'
             )

        # Checking multiple interfaces add and delete

        intf_cmd = 'interface vlan ' + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('exit')
        intf_cmd = 'interface vlan ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('exit')
        intf_cmd = 'interface vlan ' + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('exit')
        intf_cmd = 'no interface vlan ' + second_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('do show vrf')
        expected_output = 'vlan' + second_interface
        assert expected_output not in ret, \
            'Multiple Interface delete failed'
        info('### Multiple Interface delete successfully ###\n')
        intf_cmd = 'no interface vlan ' + first_interface
        s1.cmdCLI(intf_cmd)
        intf_cmd = 'no interface vlan ' + third_interface
        s1.cmdCLI(intf_cmd)

        # Cleanup

        s1.cmdCLI('exit')

    def test_show_running_config(self):
        '''
            Test show running-config for vlan interface changes
        '''

        info('''
########## Testing show running-config output ##########
''')
        s1 = self.net.switches[0]

        # Modifying interface data to test show running-config

        s1.cmdCLI('configure terminal')
        intf_cmd = 'interface vlan ' + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('ipv6 address 2002::1/120')
        s1.cmdCLI('ip address 10.1.1.1/8')
        s1.cmdCLI('ip address 10.1.1.3/8 secondary')
        s1.cmdCLI('ipv6 address 2002::2/120 secondary')
        s1.cmdCLI('exit')
        intf_cmd = 'interface vlan ' + third_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI('do show running-config')
        output = ret.split('\n')

        # print output

        intf2 = 'interface vlan' + second_interface + ' '
        intf3 = 'interface vlan' + third_interface + ' '
        intf2_output = output.index(intf2)
        intf3_output = output.index(intf3)
        assert 'ip address 10.1.1.1/8' in output[intf2_output + 2] \
            and 'ip address 10.1.1.3/8 secondary' \
            in output[intf2_output + 3] and 'ipv6 address 2002::1/120' \
            in output[intf2_output + 4] \
            and 'ipv6 address 2002::2/120 secondary' \
            in output[intf2_output + 5], 'Show running config for ' \
            + intf2 + ' failed'
        info('### Show running config for ' + intf2 + ' passed ###\n')
        assert 'no shutdown' in output[intf3_output + 1], \
            'Show running config for ' + intf3 + ' failed'
        info('### Show running config for ' + intf3 + ' passed ###\n')

        # Cleanup

        intf_cmd = 'no interface vlan ' + second_interface
        s1.cmdCLI(intf_cmd)
        intf_cmd = 'no interface vlan ' + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI('exit')


class Test_vtysh_intervlan:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_intervlan.test = intervlanCLITest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_intervlan.test.net.stop()

    def test_intervlan_input_test(self):
        self.test.test_intervlan_input_test()

    def test_vlan_interface_add_delete(self):
        self.test.test_vlan_interface_add_delete()

    def test_show_running_config(self):
        self.test.test_show_running_config()

    def __del__(self):
        del self.test
