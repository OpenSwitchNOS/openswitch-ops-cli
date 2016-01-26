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


class VLANCliTest(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        vlan_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(vlan_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def createVlan(self):
        info('''
########## Test to create VLAN ##########
''')
        vlan_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan 2' in line:
                vlan_created = True
        assert (vlan_created is True), 'Test to create VLAN - FAILED!'
        return True

    def showVlanSummary(self):
        s1 = self.net.switches[0]
        info('''
########## Test "show vlan summary" command ##########
''')
        vlan_summary_present = False
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 12')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 123')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 1234')
        s1.cmdCLI('exit')
        out = s1.cmdCLI('do show vlan summary')
        lines = out.split('\n')
        for line in lines:
            if 'Number of existing VLANs: 5' in line:
                vlan_summary_present = True
        assert (vlan_summary_present is True), \
            'Test "show vlan summary" command - FAILED!'
        return True

    def deleteVlan(self):
        s1 = self.net.switches[0]
        info('''
########## Test to delete VLAN ##########
''')
        vlan_deleted = True
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 99')
        s1.cmdCLI('exit')
        s1.cmdCLI('no vlan 99')
        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN99' in line:
                vlan_deleted = False
        assert (vlan_deleted is True), 'Test to delete VLAN - FAILED!'
        return True

    def addAccessVlanToInterface(self):
        info('''
########## Test "vlan access" command ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 21')
        out = s1.cmdCLI('vlan access 2')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test "vlan access" command - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 2')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 1' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN2' in line and '21' in line:
                success += 1

        assert 'success == 2', 'Test "vlan access" command - FAILED!'

        vlan_access_cmd_found = False
        s1.cmdCLI('no vlan access')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 2' in line:
                vlan_access_cmd_found = True
        assert (vlan_access_cmd_found is False), \
            'Test "vlan access" command - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 21')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan access 2')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test "vlan access" command - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addTrunkVlanToInterface(self):
        info('''
########## Test to add VLAN to interface ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 12')
        s1.cmdCLI('exit')
        # Split the parent interface to enable L2/L3 configurations
        # on child interfaces
        s1.cmdCLI('interface 52')
        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        s1.cmdCLI('interface 52-1')
        out = s1.cmdCLI('vlan trunk allowed 2')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan trunk allowed 2')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 2' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN2' in line and '52-1' in line:
                success += 1
        assert success == 2, 'Test to add VLAN to interface - FAILED!'

        vlan_trunk_allowed_cmd_found = True
        s1.cmdCLI('no vlan trunk allowed 2')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 2' in line:
                vlan_trunk_allowed_cmd_found = False

        assert (vlan_trunk_allowed_cmd_found is True), \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-1')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan trunk allowed 1')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addTrunkNativeVlanToInterface(self):
        info('''
########## Test to add trunk native to interface ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 77')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-2')
        out = s1.cmdCLI('vlan trunk native 2')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan trunk native 2')
        s1.cmdCLI('vlan trunk allowed 12')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 2' in line:
                success += 1
            if 'vlan trunk allowed 12' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN2' in line and '52-2' in line:
                success += 1
            if 'VLAN12' in line and '52-2' in line:
                success += 1

        assert success == 4, \
            'Test to add trunk native to interface - FAILED!'

        vlan_trunk_native_cmd_found = False
        s1.cmdCLI('no vlan trunk native')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native' in line:
                vlan_trunk_native_cmd_found = True
        assert (vlan_trunk_native_cmd_found is False), \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-2')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan trunk native 1')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addTrunkNativeTagVlanToInterface(self):
        info('''
########## Test add trunk native tag vlan to interface ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1789')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 88')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-3')
        out = s1.cmdCLI('vlan trunk native tag')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan trunk native 1789')
        s1.cmdCLI('vlan trunk allowed 88')
        s1.cmdCLI('vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1789' in line:
                success += 1
            if 'vlan trunk allowed 88' in line:
                success += 1
            if 'vlan trunk native tag' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN1789' in line and '52-3' in line:
                success += 1
            if 'VLAN88' in line and '52-3' in line:
                success += 1

        assert success == 5, \
            'Test add trunk native tag vlan to interface - FAILED!'

        vlan_trunk_native_tag_present = False
        s1.cmdCLI('no vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native tag' in line:
                return True
        assert (vlan_trunk_native_tag_present is False), \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-3')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan trunk native tag')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addAccessVlanToLAG(self):
        info('''
########## Test to add access vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 21')
        out = s1.cmdCLI('vlan access 2')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add access vlan to LAG - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 2')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 2' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN2' in line and 'lag21' in line:
                success += 1

        assert success == 2, 'Test to add access vlan to LAG - FAILED!'
        vlan_access_cmd_present = False
        s1.cmdCLI('no vlan access')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 2' in line:
                vlan_access_cmd_present = True

        assert (vlan_access_cmd_present is False), \
            'Test to add access vlan to LAG - FAILED!'
        return True

    def addTrunkVlanToLAG(self):
        info('''
########## Test to add trunk vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2345')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 55')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 31')
        out = s1.cmdCLI('vlan trunk allowed 55')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add trunk vlan to LAG - FAILED'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan trunk allowed 55')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 55' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN55' in line and 'lag31' in line:
                success += 1
        assert success == 2, 'Test to add trunk vlan to LAG - FAILED!'
        vlan_trunk_allowed_cmd_present = False
        s1.cmdCLI('no vlan trunk allowed 55')
        s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 55' in line:
                vlan_trunk_allowed_cmd_present = True
        assert (vlan_trunk_allowed_cmd_present is False), \
            'Test to add trunk vlan to LAG - FAILED!'
        return True

    def addTrunkNativeVlanToLAG(self):
        info('''
########## Test to add trunk native vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1234')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 66')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 41')
        out = s1.cmdCLI('vlan trunk native 1234')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add trunk native vlan to LAG - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan trunk native 1234')
        s1.cmdCLI('vlan trunk allowed 66')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1234' in line:
                success += 1
            if 'vlan trunk allowed 66' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN1234' in line and 'lag41' in line:
                success += 1
            if 'VLAN66' in line and 'lag41' in line:
                success += 1

        s1.cmdCLI('no vlan trunk native')
        out = s1.cmdCLI('do show running-config')

        assert success == 4, \
            'Test to add trunk native vlan to LAG - FAILED!'
        return True

    def addTrunkNativeTagVlanToLAG(self):
        info('''
########## Test to add trunk native tag vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1567')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 44')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 51')
        out = s1.cmdCLI('vlan trunk native tag')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add trunk native tag vlan to LAG - FAILED!'

        s1.cmdCLI('no routing')

        s1.cmdCLI('vlan trunk native 1567')
        s1.cmdCLI('vlan trunk allowed 44')
        s1.cmdCLI('vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1567' in line:
                success += 1
            if 'vlan trunk allowed 44' in line:
                success += 1
            if 'vlan trunk native tag' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'VLAN1567' in line and 'lag51' in line:
                success += 1
            if 'VLAN44' in line and 'lag51' in line:
                success += 1
        assert success == 5, \
            'Test to add trunk native tag vlan to LAG - FAILED!'

        vlan_trunk_native_tag_present = False
        s1.cmdCLI('no vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native tag' in line:
                vlan_trunk_native_tag_present = True
        assert (vlan_trunk_native_tag_present is False), \
            'Test to add trunk native tag vlan to LAG - FAILED!'
        return True

    def vlanCommands(self):
        info('''
########## Test to check VLAN commands ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2')
        s1.cmdCLI('no shutdown')
        out = s1.cmd('ovs-vsctl list vlan VLAN2')
        lines = out.split('\n')
        success = 0
        for line in lines:
            if 'admin' in line:
                if 'up' in line:
                    success += 1

        assert success == 1, 'Test to check VLAN commands - FAILED!'
        return True

    def internalVlanChecks(self):
        info('\n########## Test to check internal '
             'vlan validations ##########\n')
        s1 = self.net.switches[0]
        # Internal VLANs are not assigned in VSI by default.
        # To test functionality, we use below command
        # to generate internal VLANs for L3 interfaces.
        s1.ovscmd('/usr/bin/ovs-vsctl set Subsystem '
                  'base other_info:l3_port_requires_internal_vlan=1')

        s1.cmdCLI('conf t')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('ip address 1.1.1.1/8')
        s1.cmdCLI('exit')

        ret = s1.cmdCLI('vlan 1024')
        assert 'VLAN1024 is used as an internal VLAN. ' \
               'No further configuration allowed.' in ret, \
               'Test to prevent internal vlan configuration - FAILED!'
        info('### Test to prevent internal vlan configuration - PASSED ###\n')

        ret = s1.cmdCLI('no vlan 1024')
        assert 'VLAN1024 is used as an internal VLAN. ' \
               'Deletion not allowed.' in ret, \
               'Test to prevent internal vlan deletion - FAILED!'
        info('### Test to prevent internal vlan deletion - PASSED ###\n')
        return True

    def display_vlan_id_in_numerical_order(self):
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 10')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('vlan 9')
        s1.cmdCLI('vlan 15')
        s1.cmdCLI('vlan 7')
        out = s1.cmdCLI('do show vlan')
        list_interface = out.split("\n")
        result_sortted = []
        result_orig = []
        for x in list_interface:
            test = re.match('\d+\s+VLAN\d+\s+[down|up]', x)
            if test:
                test_number = test.group(0)
                number = re.match('\d+', test_number).group(0)
                result_orig.append(int(number))
                result_sortted.append(int(number))

        result_sortted.sort()

        assert result_orig == result_sortted, 'Test to \
                  display vlan-id in numerical order -FAILED'
        return True

    def noVlanTrunkAllowed(self):
        info('''
########## Test to check no vlan trunk allowed ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('int 1')
        s1.cmdCLI('no routing')
        out1 = s1.cmdCLI('do show running config')
        s1.cmdCLI('no vlan trunk allowed 100')
        out2 = s1.cmdCLI('do show running config')
        assert out1 in out2, \
            'Test to remove vlan trunk - FAILED!'
        return True


class Test_vlan_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_vlan_cli.test = VLANCliTest()

    def test_createVlan(self):
        if self.test.createVlan():
            info('''
########## Test to create VLAN - SUCCESS! ##########
''')

    def test_showVlanSummary(self):
        if self.test.showVlanSummary():
            info('''
########## Test "show vlan summary" command - SUCCESS! ##########
''')

    def test_deleteVlan(self):
        if self.test.deleteVlan():
            info('''
########## Test to delete VLAN - SUCCESS! ##########
''')

    def test_addAccessVlanToInterface(self):
        if self.test.addAccessVlanToInterface():
            info('''
########## Test "vlan access" command - SUCCESS! ##########
''')


    def test_addTrunkVlanToInterface(self):
        if self.test.addTrunkVlanToInterface():
            info('''
########## Test to add VLAN to interface - SUCCESS! ##########
''')

    def test_addTrunkNativeVlanToInterface(self):
        if self.test.addTrunkNativeVlanToInterface():
            info('''
########## Test to add trunk native to interface - SUCCESS! ##########
''')

    def test_addTrunkNativeTagVlanToInterface(self):
        if self.test.addTrunkNativeTagVlanToInterface():
            info('''
########## Test to add trunk native to interface - SUCCESS! ##########
''')

    def test_addAccessVlanToLAG(self):
        if self.test.addAccessVlanToLAG():
            info('''
########## Test add trunk native tag vlan to interface - SUCCESS! ##########
''')

    def test_addTrunkVlanToLAG(self):
        if self.test.addTrunkVlanToLAG():
            info('''
########## Test to add access vlan to LAG - SUCCESS! ##########
''')

    def test_addTrunkNativeVlanToLAG(self):
        if self.test.addTrunkNativeVlanToLAG():
            info('''
########## Test to add trunk native vlan to LAG - SUCCESS! ##########
''')

    def test_addTrunkNativeTagVlanToLAG(self):
        if self.test.addTrunkNativeTagVlanToLAG():
            info('''
########## Test to add trunk native vlan to LAG - SUCCESS!  ##########
''')

    def test_vlanCommands(self):
        if self.test.vlanCommands():
            info('''
########## Test to add trunk native tag vlan to LAG - SUCCESS! ##########
''')

    def test_internalVlanChecks(self):
        if self.test.internalVlanChecks():
            info('''
########## Test to check internal vlan validations - SUCCESS! ##########
''')

    def test_display_vlan_id_in_numerical_order(self):
        if self.test.display_vlan_id_in_numerical_order():
            info('''
########## Test to verify that vlan id is displaying in numerical order - SUCCESS! ##########\n''')


    def test_noVlanTrunkAllowed(self):
        if self.test.noVlanTrunkAllowed():
            info('''
########## Test to check no vlan trunk allowed - SUCCESS! ##########
''')

    def teardown_class(cls):
        Test_vlan_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
