#!/usr/bin/python

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

from opsvsi.docker import *
from opsvsi.opsvsitest import *

first_interface = "1"
second_interface = "2"
third_interface = "3"
error_str_range = "Error : Vlanid outside valid vlan range <1-4094>"
error_str_invalid_vlan = "Error : Invalid vlan input"
max_vlan = "4095"
min_vlan = "0"

# Internal vlan test is performed in vlan CT #


class intervlanCLITest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        intervlan_topo = SingleSwitchTopo(
            k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(intervlan_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_intervlan_input_test(self):
        '''
            Test invalid interface vlan input parameters.
        '''
        info('\n########## Test invalid vlan interface names ##########\n')
        s1 = self.net.switches[0]

        s1.cmdCLI("configure terminal")

        # Checking out of range Vlan inputs
        intf_cmd = 'interface vlan ' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Adding Vlan id = %s validation failed' % max_vlan
        intf_cmd = 'interface vlan' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Adding Vlan id = %s validation failed' % max_vlan
        intf_cmd = 'interface vlan ' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Adding Vlan id = 0 validation failed'
        intf_cmd = 'interface vlan' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Adding Vlan id = 0 validation failed'
        info('### Vlanid outside range <1-4094> validation passed ###\n')

        # Checking invalid interface vlan input parameters (vlanid: 2abc &
        # abc2abc & abc#$)
        ret = s1.cmdCLI("interface vlan 2abc ")
        assert error_str_invalid_vlan in ret, \
            'Vlan id = 2abc validation failed'
        ret = s1.cmdCLI("interface vlan2abc")
        assert error_str_invalid_vlan in ret, \
            'Vlan id = 2abc validation failed'
        ret = s1.cmdCLI("interface vlan abc2abc ")
        assert error_str_invalid_vlan in ret, \
            'Vlan id = abc2abc validation failed'
        ret = s1.cmdCLI("interface vlanabc2abc")
        assert error_str_invalid_vlan in ret, \
            'Vlan id = abc2abc validation failed'
        ret = s1.cmdCLI("interface vlan abc#$ ")
        assert 'Unknown command.' in ret, \
            'Vlan id = abc#$ validation failed'
        ret = s1.cmdCLI("interface vlanabc#$")
        assert 'Unknown command.' in ret, \
            'Vlan id = abc#$ validation failed'
        info('### Successfully verified invalid vlanid'
             '(vlanid: 2abc & abc2abc & abc#$) ###\n')

        # Deleting interface vlan outside range <1-4094>
        intf_cmd = 'no interface vlan ' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Deleting vlan id = %s validation failed' % max_vlan
        intf_cmd = 'no interface vlan' + max_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Deleting vlan id = %s validation failed' % max_vlan
        intf_cmd = 'no interface vlan ' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Deleting vlan id = %s validation failed' % min_vlan
        intf_cmd = 'no interface vlan' + min_vlan
        ret = s1.cmdCLI(intf_cmd)
        assert error_str_range in ret, \
            'Deleting vlan id = %s validation failed' % min_vlan
        info('### Deleting out of range vlanid validation passed ###\n')

        # Cleanup
        s1.cmdCLI("exit")

    def test_vlan_interface_add_delete(self):
        '''
            Test add and delete vlan interface
        '''
        info('\n########## Add/Delete vlan interface and associate'
             ' with VRF, bridge and DB ##########\n')

        s1 = self.net.switches[0]

        s1.cmdCLI("configure terminal")

        # Adding vlan interface and verify if VRF can see it
        intf_cmd = "interface vlan " + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("do show vrf")
        expected_output = 'vlan' + first_interface
        assert expected_output in ret, 'Failed to add vlan interface'
        info('### Add vlan interface and attach '
             'to VRF validation passed ###\n')
        s1.cmdCLI("exit")

        # Verify we see the interface and port created in OVSDB and attached
        # to bridge with name same as Interface Name and type as internal
        intf_cmd = "interface vlan " + second_interface
        s1.cmdCLI(intf_cmd)
        # Verify interface name
        intf_cmd = \
            "/usr/bin/ovs-vsctl get interface vlan%s name" % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'vlan' + second_interface
        assert expected_output in listcmd, 'Failed to add interface to DB'
        # Verify interface type
        intf_cmd = \
            "/usr/bin/ovs-vsctl get interface vlan%s type" % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        assert 'internal' in listcmd, 'Failed to add interface to DB'
        # Verify port name
        intf_cmd = \
            "/usr/bin/ovs-vsctl get port vlan%s name" % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'vlan' + second_interface
        assert expected_output in listcmd, 'Failed to add port to DB'
        # verify interface uuid in port row
        intf_cmd = \
            "/usr/bin/ovs-vsctl get interface vlan%s _uuid" % second_interface
        uuid = s1.ovscmd(intf_cmd).strip()
        intf_cmd = \
            "/usr/bin/ovs-vsctl get port vlan%s interfaces" % second_interface
        portlist = s1.ovscmd(intf_cmd).strip()
        assert uuid in portlist, 'Failed to add port to DB'
        # Verify port in bridge
        intf_cmd = \
            "/usr/bin/ovs-vsctl get port vlan%s _uuid" % second_interface
        port_uuid = s1.ovscmd(intf_cmd).strip()
        intf_cmd = "/usr/bin/ovs-vsctl get bridge bridge_normal ports"
        portlist = s1.ovscmd(intf_cmd).strip()
        assert port_uuid in portlist, 'Failed to add port to DB'
        # Verify port in vrf
        intf_cmd = \
            "/usr/bin/ovs-vsctl get port vlan%s _uuid" % second_interface
        port_uuid = s1.ovscmd(intf_cmd).strip()
        intf_cmd = "/usr/bin/ovs-vsctl get vrf vrf_default ports"
        portlist = s1.ovscmd(intf_cmd).strip()
        assert port_uuid in portlist, 'Failed to add port to DB'
        info('### Adding interface and port to DB validation passed ### \n')

        # Deleting interface vlan and verify if VRF can see it
        intf_cmd = "no interface vlan " + first_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("do show vrf")
        expected_output = 'vlan' + first_interface
        assert expected_output not in ret, 'Failed to delete vlan interface'
        info('### Deleting vlan interface and detach '
             'from VRF validation passed ###\n')

        # Deleting non existing vlan interface
        intf_cmd = "no interface vlan " + first_interface
        ret = s1.cmdCLI(intf_cmd)
        assert 'Vlan interface does not exist. Cannot delete' in ret, \
            'Able to delete non existing vlan interface'
        info('### Deleting non existing vlan interface '
             'validation passed ###\n')

        # Deleting vlan interface from OVSDB with name same as Interface Name
        intf_cmd = "no interface vlan " + second_interface
        s1.cmdCLI(intf_cmd)
        # Check for interface name in OVSDB
        intf_cmd = \
            "/usr/bin/ovs-vsctl get interface vlan%s name" % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = \
            'no row vlan%s in table Interface' % second_interface
        assert expected_output in listcmd, \
            'Failed to delete vlan interface in DB'
        # Check for port name in OVSDB
        intf_cmd = \
            "/usr/bin/ovs-vsctl get port vlan%s name" % second_interface
        listcmd = s1.ovscmd(intf_cmd).strip()
        expected_output = 'no row vlan%s in table Port' % second_interface
        assert expected_output in listcmd, \
            'Failed to delete vlan interface in DB'
        info('### Deleting vlan interface from DB validation passed ### \n')

        # Checking multiple interfaces add and delete
        intf_cmd = "interface vlan " + first_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("exit")
        intf_cmd = "interface vlan " + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("exit")
        intf_cmd = "interface vlan " + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("exit")
        intf_cmd = "no interface vlan " + second_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("do show vrf")
        expected_output = 'vlan' + second_interface
        assert expected_output not in ret, 'Multiple Interface delete failed'
        info('### Multiple Interface delete successfully ###\n')
        intf_cmd = "no interface vlan " + first_interface
        s1.cmdCLI(intf_cmd)
        intf_cmd = "no interface vlan " + third_interface
        s1.cmdCLI(intf_cmd)

        # Cleanup
        s1.cmdCLI("exit")

    def test_show_running_config(self):
        '''
            Test show running-config for vlan interface changes
        '''
        info('\n########## Testing show running-config output ##########\n')
        s1 = self.net.switches[0]

        # Modifying interface data to test show running-config
        s1.cmdCLI("configure terminal")
        intf_cmd = "interface vlan " + second_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("ipv6 address 2002::1/120")
        s1.cmdCLI("ip address 10.1.1.1/8")
        s1.cmdCLI("ip address 10.1.1.3/8 secondary")
        s1.cmdCLI("ipv6 address 2002::2/120 secondary")
        s1.cmdCLI("exit")
        intf_cmd = "interface vlan " + third_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("do show running-config")
        intf2 = "interface vlan" + second_interface
        intf3 = "interface vlan" + third_interface
        assert 'ip address 10.1.1.1/8' in ret and \
               'ip address 10.1.1.3/8 secondary' in ret and \
               'ipv6 address 2002::1/120' in ret and \
               'ipv6 address 2002::2/120 secondary' in ret, \
            'Show running config for ' + intf2 + ' failed'
        info('### Show running config for ' + intf2 + ' passed ###\n')
        assert 'no shutdown' in ret, 'Show running ' \
               'config for ' + intf3 + ' failed'
        info('### Show running config for ' + intf3 + ' passed ###\n')

        # Cleanup
        intf_cmd = "no interface vlan " + second_interface
        s1.cmdCLI(intf_cmd)
        intf_cmd = "no interface vlan " + third_interface
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("exit")


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
